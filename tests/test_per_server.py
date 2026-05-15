"""Per-architecture characteristics — behaviour that legitimately differs.

The equivalence suite proves the servers agree on what matters. These tests
pin down the intentional differences, so a regression in any one server is
caught precisely.
"""
import pytest

import protocol as proto
from client import FlowScopeClient


def test_central_does_not_tag_responses(servers):
    # the central gateway handles local requests without a "server" tag
    with FlowScopeClient(servers["central"]["port"]) as c:
        resp = c.request({"cmd": "ping"})
    assert "server" not in resp


@pytest.mark.parametrize("name", proto.SUB_SERVERS)
def test_sub_servers_tag_themselves(servers, name):
    # server1..4 each stamp their own name into every response
    with FlowScopeClient(servers[name]["port"]) as c:
        resp = c.request({"cmd": "echo", "data": {}})
    assert resp.get("server") == name


@pytest.mark.parametrize("name", list(proto.SERVERS))
def test_every_server_assigns_a_traceid(servers, name):
    with FlowScopeClient(servers[name]["port"]) as c:
        resp = c.request({"cmd": "ping"})
    assert isinstance(resp.get("traceid"), str) and resp["traceid"]


@pytest.mark.parametrize("name", list(proto.SERVERS))
def test_ui_button_event_contract(servers, name):
    # UI event payloads differ in detail per server, but the contract is shared:
    # a "*_response" cmd, an integer flow_step, and an "ok" status.
    with FlowScopeClient(servers[name]["port"]) as c:
        resp = c.request({"cmd": "ui_btn_click", "data": {"button": "send"}})
    assert resp["cmd"] == "ui_btn_click_response"
    assert isinstance(resp["flow_step"], int)
    assert resp["data"]["status"] == "ok"


@pytest.mark.parametrize("name", list(proto.SERVERS))
def test_ui_chat_event_contract(servers, name):
    with FlowScopeClient(servers[name]["port"]) as c:
        resp = c.request({"cmd": "ui_chat_msg", "data": {"message": "ping-from-test"}})
    assert resp["cmd"] == "ui_chat_response"
    assert isinstance(resp["flow_step"], int)
    assert resp["data"]["status"] == "ok"
