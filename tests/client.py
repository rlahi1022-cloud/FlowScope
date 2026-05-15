"""Synchronous TCP client for FlowScope servers.

Speaks the length-prefixed JSON protocol so tests (and the benchmark) can drive
any of the five servers exactly like the MFC client would. Standard library only.
"""
from __future__ import annotations

import socket
import struct
import time

import protocol as proto


class Timeout(Exception):
    """Raised when an expected response did not arrive within the deadline."""


class FlowScopeClient:
    def __init__(self, port: int, host: str = "127.0.0.1", timeout: float = 3.0):
        self.host = host
        self.port = port
        self.timeout = timeout
        self._sock: socket.socket | None = None
        self._buf = bytearray()

    # -- lifecycle -----------------------------------------------------------
    def connect(self) -> "FlowScopeClient":
        self._sock = socket.create_connection((self.host, self.port), timeout=self.timeout)
        self._sock.settimeout(self.timeout)
        self._sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        return self

    def close(self) -> None:
        if self._sock is not None:
            try:
                self._sock.close()
            finally:
                self._sock = None

    def __enter__(self):
        return self.connect()

    def __exit__(self, *exc):
        self.close()

    # -- send / receive ------------------------------------------------------
    def send(self, obj: dict) -> None:
        assert self._sock is not None, "client is not connected"
        self._sock.sendall(proto.encode(obj))

    def _fill(self, deadline: float) -> None:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise Timeout("no complete response in time")
        self._sock.settimeout(remaining)
        try:
            chunk = self._sock.recv(65536)
        except socket.timeout as e:
            raise Timeout("socket recv timed out") from e
        finally:
            if self._sock is not None:
                self._sock.settimeout(self.timeout)
        if not chunk:
            raise ConnectionError("server closed the connection")
        self._buf.extend(chunk)

    def recv(self, timeout: float | None = None) -> dict:
        """Read exactly one framed JSON response."""
        import json
        deadline = time.monotonic() + (self.timeout if timeout is None else timeout)
        while True:
            if len(self._buf) >= proto.HEADER_SIZE:
                length = proto.decode_length(self._buf[:proto.HEADER_SIZE])
                end = proto.HEADER_SIZE + length
                if len(self._buf) >= end:
                    body = bytes(self._buf[proto.HEADER_SIZE:end])
                    del self._buf[:end]
                    return json.loads(body.decode("utf-8"))
            self._fill(deadline)

    def request(self, obj: dict, timeout: float | None = None) -> dict:
        """Send a request and return its response."""
        self.send(obj)
        return self.recv(timeout=timeout)

    def expect_no_response(self, within: float = 0.8) -> None:
        """Assert the server sends nothing back (e.g. for unknown cmd / target)."""
        try:
            resp = self.recv(timeout=within)
        except Timeout:
            return
        raise AssertionError(f"expected no response, but got {resp!r}")
