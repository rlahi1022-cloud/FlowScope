#pragma once

// ------------------------------------------------
// uieventservice4.h
// server4 UI 이벤트 서비스
//
// 책임:
//   - 7가지 UI 이벤트 처리:
//     ui_btn_click, ui_server_select, ui_connect, ui_disconnect,
//     ui_chat_msg, ui_flow_start, ui_flow_stop
//   - 각각에 대해 flow_step 포함한 응답 JSON 생성
//   - eventbus4 "response" 토픽에 publish
//
// 응답 형식 (모두 동일):
//   {
//     "cmd": "{cmd}_response",
//     "flow_step": N,
//     "server": "server4",
//     "traceid": "TRC-XXXXXXXX",
//     "data": {...}
//   }
// ------------------------------------------------

#include "common/packetclass.h"
#include <string>

// ------------------------------------------------
// uieventservice4
// ------------------------------------------------
class uieventservice4 {
public:
    // ------------------------------------------------
    // process
    // packet4를 받아 UI 이벤트 처리
    // 완료 후 eventbus4에 "response" publish
    // ------------------------------------------------
    void process(const packet4& pkt);

private:
    // ------------------------------------------------
    // build_ui_response
    // UI 이벤트 응답 JSON 생성
    // flow_step은 랜덤 또는 시퀀셜 (1~10)
    // ------------------------------------------------
    std::string build_ui_response(const packet4& pkt);

    // ------------------------------------------------
    // publish_response
    // 응답을 eventbus4 "response" 토픽에 publish
    // 형식: "fd:traceid:json"
    // ------------------------------------------------
    void publish_response(int fd,
                         const std::string& traceid,
                         const std::string& json);

    // ------------------------------------------------
    // get_response_cmd
    // 원본 cmd에 "_response" suffix 추가
    // ------------------------------------------------
    std::string get_response_cmd(const std::string& cmd);
};
