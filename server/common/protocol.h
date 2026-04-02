#pragma once

// ------------------------------------------------
// protocol.h
// 서버 내부 프로토콜 정의
//
// 클라이언트 JSON cmd 문자열 → internal_protocol enum 변환
// 모든 내부 처리 흐름은 enum 기반으로 통일
// ------------------------------------------------

#include <string>
#include <cstdint>

// ------------------------------------------------
// 패킷 헤더 크기 (4바이트 빅엔디언 길이 헤더)
// ------------------------------------------------
static constexpr int HEADER_SIZE   = 4;

// ------------------------------------------------
// 최대 패킷 바디 크기 (1MB 제한)
// ------------------------------------------------
static constexpr int MAX_BODY_SIZE = 1024 * 1024;

// ------------------------------------------------
// internal_protocol
// 클라이언트 JSON cmd → 서버 내부 처리 분기용 enum
//
// sync  계열 : 즉시 처리, 빠른 응답
// async 계열 : 흐름 처리, JobQueue → Worker
// event 계열 : EventBus 경유, 별도 처리
// ------------------------------------------------
enum class internal_protocol
{
    // -------------------------
    // sync 계열 (즉시 처리)
    // -------------------------
    echo,           // 에코 응답
    ping,           // 핑퐁 응답
    debug_status,   // 서버 상태 디버그 출력

    // -------------------------
    // async 계열 (흐름 처리)
    // -------------------------
    flow_start,     // 흐름 시작
    flow_step1,     // 흐름 단계 1
    flow_step2,     // 흐름 단계 2
    flow_end,       // 흐름 종료

    // -------------------------
    // event 계열 (EventBus 경유)
    // -------------------------
    ai_keyword,     // AI 처리 요청

    // -------------------------
    // 기타
    // -------------------------
    unknown         // 알 수 없는 프로토콜
};

// ------------------------------------------------
// packet
// epoll 수신 후 router → handler로 전달되는 단위
//
// fd       : 요청을 보낸 클라이언트 소켓 fd
// protocol : string_to_protocol()로 변환된 내부 프로토콜
// jsonbody : 원본 JSON 문자열 (서비스 레이어에서 파싱)
// traceid  : 요청 추적 ID (log_flow에서 사용)
// ------------------------------------------------
struct packet
{
    int               fd;        // 클라이언트 소켓 fd
    internal_protocol protocol;  // 내부 프로토콜 enum
    std::string       jsonbody;  // 원본 JSON 바디
    std::string       traceid;   // 요청 추적 ID
};

// ------------------------------------------------
// string_to_protocol
// JSON cmd 문자열 → internal_protocol 변환
// 매칭되지 않으면 unknown 반환
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
// internal_protocol → 문자열 변환 (로그 출력용)
// ------------------------------------------------
inline std::string protocol_to_string(internal_protocol p)
{
    switch (p)
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