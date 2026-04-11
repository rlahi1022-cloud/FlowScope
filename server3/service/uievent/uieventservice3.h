#pragma once

// ------------------------------------------------
// uieventservice3.h
// server3 UI event 비즈니스 로직 서비스
//
// 역할:
//   - 7개의 ui_* 커맨드 처리
//   - ui_btn_click, ui_server_select, ui_connect, ui_disconnect,
//     ui_chat_msg, ui_flow_start, ui_flow_stop
//
// server3 특징:
//   - EventBus 경유: handler 요청 수신 → 처리 → EventBus publish
//   - 모든 응답에 "flow_step" 정수 필드 포함
//   - 모든 응답에 "server":"server3" 필드 포함
//
// 흐름:
//   uieventhandler3 → uieventservice3.process(payload)
//                  → eventbus3.publish("response", response_payload)
// ------------------------------------------------

#include <string>

// ------------------------------------------------
// uieventservice3
// UI 이벤트를 처리하고 EventBus "response" 토픽으로 publish
// ------------------------------------------------
class uieventservice3
{
public:
    // ------------------------------------------------
    // process
    // payload를 받아 ui_* 이벤트를 처리하고
    // EventBus "response" 토픽으로 publish
    // payload 형식: "fd:traceid:jsonbody"
    //
    // 모든 응답:
    //   - "flow_step" 정수 필드 포함
    //   - "server":"server3" 필드 포함
    // ------------------------------------------------
    void process(const std::string& payload);

private:
    // payload 파싱
    bool parse_payload(const std::string& payload,
                      int& fd,
                      std::string& traceid,
                      std::string& jsonbody);

    // JSON에서 "cmd" 필드 추출
    std::string parse_cmd(const std::string& jsonbody);

    // ui_btn_click 처리
    std::string handle_ui_btn_click(const std::string& traceid);

    // ui_server_select 처리
    std::string handle_ui_server_select(const std::string& traceid);

    // ui_connect 처리
    std::string handle_ui_connect(const std::string& traceid);

    // ui_disconnect 처리
    std::string handle_ui_disconnect(const std::string& traceid);

    // ui_chat_msg 처리
    std::string handle_ui_chat_msg(const std::string& traceid);

    // ui_flow_start 처리
    std::string handle_ui_flow_start(const std::string& traceid);

    // ui_flow_stop 처리
    std::string handle_ui_flow_stop(const std::string& traceid);
};
