"""Pytest fixtures for the FlowScope test suite.

On the first test that needs them, all five servers are built (CMake) and
started for the whole session, then stopped at the end.

If all five ports are already listening, those processes are reused as-is —
handy while iterating with the servers running in another terminal.

Build location is tests/_build by default; override with FLOWSCOPE_BUILD_DIR
(see serverctl.py).
"""
from __future__ import annotations

import pytest

import protocol as proto
import serverctl
from client import FlowScopeClient


@pytest.fixture(scope="session")
def servers():
    """Yield {name: {"port": int, ...}} for the five running servers."""
    ports = {name: info["port"] for name, info in proto.SERVERS.items()}

    if all(serverctl.port_open(p) for p in ports.values()):
        # external servers already running — reuse them, don't manage lifecycle
        yield {name: {"port": port, "external": True} for name, port in ports.items()}
        return

    binaries = serverctl.build_all(log=lambda _msg: None)
    handles = serverctl.start_servers(list(proto.SERVERS), binaries)
    try:
        yield {
            name: {"port": h.port, "pid": h.pid, "external": False}
            for name, h in handles.items()
        }
    finally:
        serverctl.stop_servers(handles)


@pytest.fixture
def connect(servers):
    """Factory: connect("server2") -> FlowScopeClient, auto-closed after the test."""
    opened: list[FlowScopeClient] = []

    def _connect(name: str = "central", timeout: float = 3.0) -> FlowScopeClient:
        client = FlowScopeClient(servers[name]["port"], timeout=timeout).connect()
        opened.append(client)
        return client

    yield _connect
    for client in opened:
        client.close()
