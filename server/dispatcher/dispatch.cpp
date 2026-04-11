// ------------------------------------------------
// dispatch.cpp
// 프로토콜별 처리 방식 결정 구현
// ------------------------------------------------

#include "dispatcher/dispatch.h"

// ------------------------------------------------
// get_dispatch_type
// internal_protocol → dispatch_type 반환
//
// sync  : epoll 스레드에서 즉시 처리 가능한 프로토콜
// async : 무거운 흐름 처리, JobQueue → Worker
// event : AI 등 EventBus 경유 처리
// ------------------------------------------------
dispatch_type get_dispatch_type(internal_protocol protocol)
{
    switch (protocol)
    {
        // -------------------------
        // sync: 즉시 처리 (빠른 응답)
        // -------------------------
        case internal_protocol::ping:
        case internal_protocol::echo:
        case internal_protocol::debug_status:
            return dispatch_type::sync;

        // -------------------------
        // async: 흐름 처리 (JobQueue → Worker)
        // -------------------------
        case internal_protocol::flow_start:
        case internal_protocol::flow_step1:
        case internal_protocol::flow_step2:
        case internal_protocol::flow_end:
            return dispatch_type::async;

        // -------------------------
        // event: EventBus 경유 처리
        // -------------------------
        case internal_protocol::ai_keyword:
            return dispatch_type::event;

        // -------------------------
        // sync: UI 이벤트 (즉시 응답)
        // -------------------------
        case internal_protocol::ui_btn_click:
        case internal_protocol::ui_server_select:
        case internal_protocol::ui_connect:
        case internal_protocol::ui_disconnect:
        case internal_protocol::ui_chat_msg:
        case internal_protocol::ui_flow_start:
        case internal_protocol::ui_flow_stop:
            return dispatch_type::sync;

        // -------------------------
        // unknown: 처리 불가
        // -------------------------
        default:
            return dispatch_type::unknown;
    }
}