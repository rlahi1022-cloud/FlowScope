#pragma once

// ------------------------------------------------
// internalprotocol.h
// 서버 전용 내부 처리 분기 정의
//
// 책임 범위:
//   - internal_protocol -> 서버 처리 계열 분류
//   - 서버 아키텍처별 처리 경로를 결정하는 분기 기준
//
// include 위치:
//   server/router, server/dispatcher (서버 전용)
//
// 관련 파일:
//   공통 프로토콜 enum  -> common/protocol.h
//   서버 패킷 클래스    -> server/common/packetclass.h
// ------------------------------------------------

// protocol.h는 flowScope/common/ 에 위치
// CMake include path: ${CMAKE_CURRENT_SOURCE_DIR}/../common
#include "protocol.h"

// ------------------------------------------------
// processingtype
// 서버가 요청을 어떤 경로로 처리할지 결정하는 분류값
//
// sync  : epoll 스레드에서 즉시 처리 후 응답
// async : JobQueue 에 적재 -> Worker Thread 에서 처리
// event : EventBus 에 publish -> subscribe 콜백에서 처리
// ------------------------------------------------
enum class processingtype
{
    sync,   // 즉시 처리 (echo, ping, debug_status)
    async,  // Worker Thread 경유 (flow_*)
    event,  // EventBus 경유 (ai_keyword)
    none    // 분기 불가 (unknown)
};

// ------------------------------------------------
// get_processingtype
// internal_protocol 을 processingtype 으로 변환한다.
// router/dispatcher 에서 처리 경로 결정 시 호출한다.
// ------------------------------------------------
inline processingtype get_processingtype(internal_protocol proto)
{
    switch (proto)
    {
        case internal_protocol::echo:
        case internal_protocol::ping:
        case internal_protocol::debug_status:
            return processingtype::sync;

        case internal_protocol::flow_start:
        case internal_protocol::flow_step1:
        case internal_protocol::flow_step2:
        case internal_protocol::flow_end:
            return processingtype::async;

        case internal_protocol::ai_keyword:
            return processingtype::event;

        // UI 이벤트 계열 - 즉시 응답
        case internal_protocol::ui_btn_click:
        case internal_protocol::ui_server_select:
        case internal_protocol::ui_connect:
        case internal_protocol::ui_disconnect:
        case internal_protocol::ui_chat_msg:
        case internal_protocol::ui_flow_start:
        case internal_protocol::ui_flow_stop:
            return processingtype::sync;

        default:
            return processingtype::none;
    }
}

// ------------------------------------------------
// processingtype_to_string
// processingtype -> 문자열 변환 (로그 출력용)
// ------------------------------------------------
inline std::string processingtype_to_string(processingtype pt)
{
    switch (pt)
    {
        case processingtype::sync:  return "sync";
        case processingtype::async: return "async";
        case processingtype::event: return "event";
        default:                    return "none";
    }
}