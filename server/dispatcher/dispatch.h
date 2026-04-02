#pragma once

// ------------------------------------------------
// dispatch.h
// 프로토콜별 처리 방식(dispatch_type) 정의
//
// 역할:
//   internal_protocol → dispatch_type 매핑
//   Router가 이 타입을 보고 처리 경로를 결정한다
//
// 처리 경로:
//   sync  → 즉시 처리 후 응답 (epoll 스레드에서 직접)
//   async → JobQueue → Worker Thread 로 전달
//   event → EventBus publish → subscriber 처리
// ------------------------------------------------

#include "common/protocol.h"

// ------------------------------------------------
// dispatch_type
// 각 프로토콜의 처리 방식 분류
// ------------------------------------------------
enum class dispatch_type
{
    sync,    // 즉시 처리 (빠른 응답, blocking 없음)
    async,   // 비동기 처리 (JobQueue → Worker)
    event,   // 이벤트 처리 (EventBus 경유)
    unknown  // 알 수 없는 처리 방식
};

// ------------------------------------------------
// dispatch_type_to_string
// dispatch_type → 문자열 변환 (로그 출력용)
// ------------------------------------------------
inline std::string dispatch_type_to_string(dispatch_type dt)
{
    switch (dt)
    {
        case dispatch_type::sync:    return "sync";
        case dispatch_type::async:   return "async";
        case dispatch_type::event:   return "event";
        default:                     return "unknown";
    }
}

// ------------------------------------------------
// get_dispatch_type
// internal_protocol → dispatch_type 결정 함수
//
// 프로토콜 종류에 따라 처리 경로를 반환한다
// Router는 이 함수의 반환값으로 분기를 결정한다
// ------------------------------------------------
dispatch_type get_dispatch_type(internal_protocol protocol);