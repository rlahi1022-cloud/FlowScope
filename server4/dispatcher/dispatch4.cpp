// ------------------------------------------------
// dispatch4.cpp
// server4 디스패처 구현부
// ------------------------------------------------

#include "dispatcher/dispatch4.h"

// ------------------------------------------------
// determine
// proto를 분석해 처리 경로 결정
// ------------------------------------------------
dispatch_type dispatch4::determine(internal_protocol proto)
{
    if (is_sync(proto))
        return dispatch_type::sync;

    if (is_async(proto))
        return dispatch_type::async;

    if (is_event(proto))
        return dispatch_type::event;

    return dispatch_type::unknown;
}

// ------------------------------------------------
// is_sync
// sync 경로인지 확인 (echo, ping, debug_status, ui_*)
// ------------------------------------------------
bool dispatch4::is_sync(internal_protocol proto)
{
    return proto == internal_protocol::echo
        || proto == internal_protocol::ping
        || proto == internal_protocol::debug_status
        || proto == internal_protocol::ui_btn_click
        || proto == internal_protocol::ui_server_select
        || proto == internal_protocol::ui_connect
        || proto == internal_protocol::ui_disconnect
        || proto == internal_protocol::ui_chat_msg
        || proto == internal_protocol::ui_flow_start
        || proto == internal_protocol::ui_flow_stop;
}

// ------------------------------------------------
// is_async
// async 경로인지 확인 (flow_*)
// ------------------------------------------------
bool dispatch4::is_async(internal_protocol proto)
{
    return proto == internal_protocol::flow_start
        || proto == internal_protocol::flow_step1
        || proto == internal_protocol::flow_step2
        || proto == internal_protocol::flow_end;
}

// ------------------------------------------------
// is_event
// event 경로인지 확인 (ai_keyword)
// ------------------------------------------------
bool dispatch4::is_event(internal_protocol proto)
{
    return proto == internal_protocol::ai_keyword;
}
