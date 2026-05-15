"""Architecture equivalence — the core of the suite.

FlowScope implements one protocol on four different server architectures
(plus the central forwarding gateway). These tests run the SAME checks against
every server and assert identical behaviour, proving the four structures are
functionally equivalent despite their very different internals.
"""
import threading

import pytest

import protocol as proto
from client import FlowScopeClient

ALL_SERVERS = list(proto.SERVERS)        # central, server1, server2, server3, server4


@pytest.fixture(params=ALL_SERVERS)
def server_name(request):
    """Parametrize a test across every server."""
    return request.param


def test_echo(server_name, servers):
    with FlowScopeClient(servers[server_name]["port"]) as c:
        resp = c.request({"cmd": "echo", "data": {"msg": "hello"}})
    assert resp["cmd"] == "echo_response"
    assert resp["traceid"]
    # echo returns the original request body verbatim under "data"
    assert resp["data"] == {"cmd": "echo", "data": {"msg": "hello"}}


def test_ping(server_name, servers):
    with FlowScopeClient(servers[server_name]["port"]) as c:
        resp = c.request({"cmd": "ping"})
    assert resp["cmd"] == "pong"
    assert resp["data"] == "pong"
    assert resp["traceid"]


def test_unknown_cmd_is_silently_dropped(server_name, servers):
    # every server drops an unrecognised cmd without replying, and stays alive
    with FlowScopeClient(servers[server_name]["port"]) as c:
        c.send({"cmd": "no_such_cmd_at_all"})
        c.expect_no_response(within=0.8)


def test_multiple_requests_on_one_connection(server_name, servers):
    with FlowScopeClient(servers[server_name]["port"]) as c:
        for i in range(10):
            resp = c.request({"cmd": "echo", "data": {"i": i}})
            assert resp["data"]["data"]["i"] == i


def test_large_payload_roundtrip(server_name, servers):
    blob = "x" * 200_000          # 200 KB, well under the 1 MiB frame limit
    with FlowScopeClient(servers[server_name]["port"], timeout=6) as c:
        resp = c.request({"cmd": "echo", "data": {"blob": blob}})
    assert resp["data"]["data"]["blob"] == blob


def test_concurrent_connections(server_name, servers):
    port = servers[server_name]["port"]
    n = 30
    results = [None] * n

    def worker(i):
        try:
            with FlowScopeClient(port, timeout=6) as c:
                resp = c.request({"cmd": "echo", "data": {"i": i}})
                results[i] = resp["data"]["data"]["i"]
        except Exception as e:           # noqa: BLE001 - surface as test failure
            results[i] = repr(e)

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    assert results == list(range(n))


def _normalise(resp: dict) -> dict:
    # traceid comes from a per-server generator and sub-servers add a "server"
    # tag; neither changes functional behaviour, so drop them before comparing.
    return {k: v for k, v in resp.items() if k not in ("traceid", "server")}


@pytest.mark.parametrize("request_body,expected", [
    (
        {"cmd": "echo", "data": {"k": "v", "n": 42}},
        {"cmd": "echo_response", "data": {"cmd": "echo", "data": {"k": "v", "n": 42}}},
    ),
    (
        {"cmd": "ping"},
        {"cmd": "pong", "data": "pong"},
    ),
])
def test_all_architectures_return_identical_responses(servers, request_body, expected):
    """The same request produces a functionally identical response on all five
    servers — this is the headline guarantee the project claims."""
    responses = {}
    for name in ALL_SERVERS:
        with FlowScopeClient(servers[name]["port"]) as c:
            responses[name] = _normalise(c.request(request_body))
    for name, resp in responses.items():
        assert resp == expected, f"{name} diverged from the others: {resp}"
