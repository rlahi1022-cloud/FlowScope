"""The central server as an API gateway: target-based forwarding.

A request with a "target" field is forwarded by the central server to that
sub-server over TCP; the sub-server's response is relayed back to the client.
A request with no target is handled by the central server itself.
"""
import pytest

import protocol as proto
from client import FlowScopeClient


@pytest.mark.parametrize("target", proto.SUB_SERVERS)
def test_echo_is_forwarded_to_target(servers, target):
    with FlowScopeClient(servers["central"]["port"], timeout=5) as c:
        resp = c.request({"cmd": "echo", "target": target, "data": {"msg": "fwd"}})
    assert resp["cmd"] == "echo_response"
    # the response carries the target server's own tag — proof it really went there
    assert resp["server"] == target
    assert resp["data"]["data"] == {"msg": "fwd"}


@pytest.mark.parametrize("target", proto.SUB_SERVERS)
def test_ping_is_forwarded_to_target(servers, target):
    with FlowScopeClient(servers["central"]["port"], timeout=5) as c:
        resp = c.request({"cmd": "ping", "target": target})
    assert resp["cmd"] == "pong"
    assert resp["server"] == target


def test_request_without_target_is_handled_locally(servers):
    with FlowScopeClient(servers["central"]["port"]) as c:
        resp = c.request({"cmd": "ping"})
    assert resp["cmd"] == "pong"
    assert "server" not in resp        # handled by the central server itself


def test_unknown_target_is_dropped(servers):
    # an unrecognised target is logged and dropped — no response, connection stays open
    with FlowScopeClient(servers["central"]["port"]) as c:
        c.send({"cmd": "echo", "target": "server99", "data": {}})
        c.expect_no_response(within=0.8)
