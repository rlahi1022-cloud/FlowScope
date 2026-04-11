#pragma once

// ------------------------------------------------
// uieventhandler3.h
// server3 UI event 핸들러
//
// 역할:
//   - 7개의 ui_* 커맨드를 처리
//   - ui_btn_click, ui_server_select, ui_connect, ui_disconnect,
//     ui_chat_msg, ui_flow_start, ui_flow_stop
//
// 흐름:
//   eventbus3.publish("request", payload)
//   → uieventhandler3.handle(payload)
//   → uieventservice3.process()
//   → eventbus3.publish("response", response_payload)
// ------------------------------------------------

#include "service/handler/basehandler3.h"
#include "service/uievent/uieventservice3.h"

// ------------------------------------------------
// uieventhandler3
// ui_* 요청을 uieventservice3로 위임
// ------------------------------------------------
class uieventhandler3 : public basehandler3
{
public:
    // 생성자
    uieventhandler3();

    // ------------------------------------------------
    // handle
    // payload를 받아 ui_* 이벤트 처리
    // payload 형식: "fd:traceid:jsonbody"
    // ------------------------------------------------
    void handle(const std::string& payload) override;

private:
    // ui event 비즈니스 로직 처리 객체
    uieventservice3 service;
};
