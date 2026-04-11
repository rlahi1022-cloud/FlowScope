#pragma once

// ------------------------------------------------
// uieventservice.h
// 클라이언트 UI 이벤트 비즈니스 로직 처리 서비스
//
// 흐름:
//   uieventhandler → uieventservice.process() → eventbus → 클라이언트 응답
//
// 역할:
//   - 클라이언트 UI 동작(버튼 클릭, 서버 선택, 연결, 채팅 등)을 수신
//   - 해당 동작에 대한 서버 반응을 JSON 응답으로 구성
//   - 응답에 flow_step 정보를 포함하여 클라이언트 흐름도 업데이트 유도
// ------------------------------------------------

#include "protocol.h"
#include "packetclass.h"
#include <string>

class uieventservice
{
public:
    // packet을 받아 UI 이벤트에 대한 응답을 처리한다
    void process(const packet& pkt);

private:
    // JSON에서 특정 문자열 필드 추출
    std::string parse_field(const std::string& json, const std::string& field);

    // 버튼 클릭 응답 JSON 생성
    std::string build_btn_click_response(const std::string& body,
                                          const std::string& traceid);

    // 서버 선택 응답 JSON 생성
    std::string build_server_select_response(const std::string& body,
                                              const std::string& traceid);

    // 연결 응답 JSON 생성
    std::string build_connect_response(const std::string& body,
                                        const std::string& traceid);

    // 연결 해제 응답 JSON 생성
    std::string build_disconnect_response(const std::string& traceid);

    // 채팅 메시지 응답 JSON 생성
    std::string build_chat_response(const std::string& body,
                                     const std::string& traceid);

    // 흐름 시작 응답 JSON 생성
    std::string build_flow_start_response(const std::string& body,
                                           const std::string& traceid);

    // 흐름 중지 응답 JSON 생성
    std::string build_flow_stop_response(const std::string& traceid);
};
