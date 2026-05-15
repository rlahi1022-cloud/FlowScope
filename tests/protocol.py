"""FlowScope wire protocol helpers + server registry.

Wire format (see common/packet.h):
    [ 4-byte big-endian length ][ JSON body ]

The JSON request shape (see server/router/router.cpp):
    {"cmd": "<command>", "target": "<server1..4 | omitted>", "data": {...}}

Pure standard library — no third-party dependencies.
"""
from __future__ import annotations

import json
import struct

HEADER_SIZE = 4                       # common/packet.h : HEADER_SIZE
MAX_BODY_SIZE = 1024 * 1024           # common/packet.h : MAX_BODY_SIZE (1 MiB)

# ---------------------------------------------------------------------------
# Server registry
#
#   name    : logical name used throughout the tests
#   port    : TCP port the server listens on
#   src     : source directory (relative to repo root) holding its CMakeLists
#   binary  : executable name produced by that CMakeLists
#   arch    : human-readable architecture label (for the benchmark table)
# ---------------------------------------------------------------------------
SERVERS = {
    "central": {
        "port": 9000, "src": "server", "binary": "flowscope_server",
        "arch": "API Gateway (forwarding)",
    },
    "server1": {
        "port": 9001, "src": "server1", "binary": "flowscope_server1",
        "arch": "Thread-per-Connection",
    },
    "server2": {
        "port": 9002, "src": "server2", "binary": "flowscope_server2",
        "arch": "epoll + direct write",
    },
    "server3": {
        "port": 9003, "src": "server3", "binary": "flowscope_server3",
        "arch": "EventBus (pub/sub)",
    },
    "server4": {
        "port": 9004, "src": "server4", "binary": "server4",
        "arch": "Hybrid (epoll + dispatcher + eventbus)",
    },
}

# the four architectures that the portfolio compares (central is the gateway)
SUB_SERVERS = ["server1", "server2", "server3", "server4"]


def encode(obj: dict) -> bytes:
    """dict -> framed packet bytes (4-byte BE length prefix + JSON body)."""
    body = json.dumps(obj).encode("utf-8")
    if len(body) > MAX_BODY_SIZE:
        raise ValueError(f"body too large: {len(body)} > {MAX_BODY_SIZE}")
    return struct.pack(">I", len(body)) + body


def decode_length(header: bytes) -> int:
    """4-byte big-endian header -> body length."""
    return struct.unpack(">I", header)[0]
