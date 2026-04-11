#pragma once

// ------------------------------------------------
// uieventservice2.h
// server2 UI 이벤트 비즈니스 로직 서비스
//
// server2 특징:
//   - EventBus 없음 → fd로 직접 write
//   - 7개 UI 프로토콜 처리
//   - 클라이언트 응답에 flow_step 포함 (흐름도 업데이트용)
// ------------------------------------------------

#include "common/packetclass.h"
#include <string>

// ------------------------------------------------
// uieventservice2
// UI 이벤트 응답을 처리하고 fd로 직접 전송
// ------------------------------------------------
class uieventservice2
{
public:
    // ------------------------------------------------
    // process
    // packet2를 받아 UI 응답을 생성하고
    // fd로 직접 write한다
    // ------------------------------------------------
    void process(const packet2& pkt);

private:
    // UI 응답 JSON 생성
    std::string build_ui_response(const packet2& pkt);

    // JSON에서 특정 문자열 필드 추출
    std::string parse_field(const std::string& json,
                            const std::string& field);

    // UI cmd → response cmd 변환
    std::string get_response_cmd(const std::string& cmd);

    // fd로 4byte 헤더 + body 전송
    void send_response(int fd, const std::string& body);
};
