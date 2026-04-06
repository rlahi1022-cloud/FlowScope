#pragma once

// ------------------------------------------------
// protocol.h
// 서버 / 클라이언트 공통 프로토콜 정의
//
// 책임 범위:
//   - internal_protocol enum 정의
//   - JSON "cmd" 문자열 <-> enum 변환 함수
//
// include 위치:
//   서버 : server/ 전 계층에서 include
//   클라 : client/ 전 계층에서 include
//
// 관련 파일:
//   네트워크 상수          -> common/packet.h
//   서버 내부 처리 분기    -> server/common/internalprotocol.h
// ------------------------------------------------

#include <string>

// ------------------------------------------------
// internal_protocol
//
// 클라이언트에서 수신한 JSON "cmd" 필드를
// 서버 내부에서 분기 처리하기 위한 enum 이다.
//
// sync  계열 : epoll 스레드에서 즉시 처리
//              (echo, ping, debug_status)
// async 계열 : JobQueue -> Worker Thread 경유
//              (flow_start, flow_step1, flow_step2, flow_end)
// event 계열 : EventBus publish -> subscribe 경유
//              (ai_keyword)
// ------------------------------------------------
enum class internal_protocol
{
    // sync 계열 - 즉시 응답
    echo,           // 수신 데이터를 그대로 되돌린다
    ping,           // 서버 생존 확인
    debug_status,   // 서버 내부 상태 반환

    // async 계열 - Worker Thread 에서 처리
    flow_start,     // 흐름 처리 시작
    flow_step1,     // 흐름 처리 1단계
    flow_step2,     // 흐름 처리 2단계
    flow_end,       // 흐름 처리 종료

    // event 계열 - EventBus 경유
    ai_keyword,     // AI 서버(PC3)로 전달할 키워드

    // 분기 불가
    unknown         // cmd 매핑 실패 시 반환
};

// ------------------------------------------------
// string_to_protocol
// JSON "cmd" 문자열을 internal_protocol 로 변환한다.
// 매핑되지 않는 문자열은 unknown 을 반환한다.
// ------------------------------------------------
inline internal_protocol string_to_protocol(const std::string& cmd)
{
    if (cmd == "echo")         return internal_protocol::echo;
    if (cmd == "ping")         return internal_protocol::ping;
    if (cmd == "debug_status") return internal_protocol::debug_status;
    if (cmd == "flow_start")   return internal_protocol::flow_start;
    if (cmd == "flow_step1")   return internal_protocol::flow_step1;
    if (cmd == "flow_step2")   return internal_protocol::flow_step2;
    if (cmd == "flow_end")     return internal_protocol::flow_end;
    if (cmd == "ai_keyword")   return internal_protocol::ai_keyword;
    return internal_protocol::unknown;
}

// ------------------------------------------------
// protocol_to_string
// internal_protocol enum 을 문자열로 변환한다.
// 로그 출력 및 JSON 응답 생성 시 사용한다.
// ------------------------------------------------
inline std::string protocol_to_string(internal_protocol proto)
{
    switch (proto)
    {
        case internal_protocol::echo:         return "echo";
        case internal_protocol::ping:         return "ping";
        case internal_protocol::debug_status: return "debug_status";
        case internal_protocol::flow_start:   return "flow_start";
        case internal_protocol::flow_step1:   return "flow_step1";
        case internal_protocol::flow_step2:   return "flow_step2";
        case internal_protocol::flow_end:     return "flow_end";
        case internal_protocol::ai_keyword:   return "ai_keyword";
        default:                              return "unknown";
    }
}