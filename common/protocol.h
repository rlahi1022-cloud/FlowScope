#pragma once

// ------------------------------------------------
// protocol.h
// 서버 / 클라이언트 공통 프로토콜 정의
//
// 책임 범위:
//   - internal_protocol enum 정의
//   - target_server enum 정의
//   - JSON "cmd" 문자열 <-> enum 변환 함수
//   - JSON "target" 문자열 <-> enum 변환 함수
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

    // ui_event 계열 - 클라이언트 UI 동작 전달
    ui_btn_click,       // 버튼 클릭 이벤트
    ui_server_select,   // 서버 선택 변경
    ui_connect,         // 서버 연결 요청
    ui_disconnect,      // 서버 연결 해제
    ui_chat_msg,        // 채팅 메시지 전송
    ui_flow_start,      // 흐름 처리 시작 요청
    ui_flow_stop,       // 흐름 처리 중지 요청

    // 분기 불가
    unknown         // cmd 매핑 실패 시 반환
};

// ------------------------------------------------
// target_server
// 클라이언트 JSON "target" 필드를
// 포워딩 대상 서버로 변환하는 enum
//
// none    : 중앙서버 자체 처리
// server1 : Thread-per-Connection 서버 (port 9001)
// server2 : epoll + worker + DB 서버   (port 9002)
// server3 : EventBus 기반 서버         (port 9003)
// server4 : Hybrid 서버                (port 9004)
// ------------------------------------------------
enum class target_server
{
    none,    // 중앙서버 자체 처리
    server1, // port 9001
    server2, // port 9002
    server3, // port 9003
    server4, // port 9004
    unknown  // 매핑 실패
};

// ------------------------------------------------
// string_to_protocol
// JSON "cmd" 문자열을 internal_protocol 로 변환한다.
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
    if (cmd == "ui_btn_click")     return internal_protocol::ui_btn_click;
    if (cmd == "ui_server_select") return internal_protocol::ui_server_select;
    if (cmd == "ui_connect")       return internal_protocol::ui_connect;
    if (cmd == "ui_disconnect")    return internal_protocol::ui_disconnect;
    if (cmd == "ui_chat_msg")      return internal_protocol::ui_chat_msg;
    if (cmd == "ui_flow_start")    return internal_protocol::ui_flow_start;
    if (cmd == "ui_flow_stop")     return internal_protocol::ui_flow_stop;
    return internal_protocol::unknown;
}

// ------------------------------------------------
// protocol_to_string
// internal_protocol enum 을 문자열로 변환한다.
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
        case internal_protocol::ai_keyword:     return "ai_keyword";
        case internal_protocol::ui_btn_click:     return "ui_btn_click";
        case internal_protocol::ui_server_select: return "ui_server_select";
        case internal_protocol::ui_connect:       return "ui_connect";
        case internal_protocol::ui_disconnect:    return "ui_disconnect";
        case internal_protocol::ui_chat_msg:      return "ui_chat_msg";
        case internal_protocol::ui_flow_start:    return "ui_flow_start";
        case internal_protocol::ui_flow_stop:     return "ui_flow_stop";
        default:                                  return "unknown";
    }
}

// ------------------------------------------------
// string_to_target
// JSON "target" 문자열을 target_server enum 으로 변환한다.
// target 필드 없으면 none (중앙서버 자체 처리)
// ------------------------------------------------
inline target_server string_to_target(const std::string& target)
{
    if (target == "server1") return target_server::server1;
    if (target == "server2") return target_server::server2;
    if (target == "server3") return target_server::server3;
    if (target == "server4") return target_server::server4;
    if (target.empty())      return target_server::none;
    return target_server::unknown;
}

// ------------------------------------------------
// target_to_port
// target_server enum 을 포트 번호로 변환한다.
// ------------------------------------------------
inline int target_to_port(target_server target)
{
    switch (target)
    {
        case target_server::server1: return 9001;
        case target_server::server2: return 9002;
        case target_server::server3: return 9003;
        case target_server::server4: return 9004;
        default:                     return -1;
    }
}

// ------------------------------------------------
// target_to_string
// target_server enum 을 문자열로 변환한다. (로그용)
// ------------------------------------------------
inline std::string target_to_string(target_server target)
{
    switch (target)
    {
        case target_server::none:    return "none";
        case target_server::server1: return "server1";
        case target_server::server2: return "server2";
        case target_server::server3: return "server3";
        case target_server::server4: return "server4";
        default:                     return "unknown";
    }
}