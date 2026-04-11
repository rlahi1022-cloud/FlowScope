#pragma once

// ------------------------------------------------
// uieventhandler4.h
// server4 UI 이벤트 (ui_*) 핸들러
//
// 책임:
//   - 7가지 UI 이벤트 처리:
//     ui_btn_click, ui_server_select, ui_connect, ui_disconnect,
//     ui_chat_msg, ui_flow_start, ui_flow_stop
//   - 각각 uieventservice4로 위임
//
// 응답 형식:
//   { "cmd": "ui_btn_click_response", "flow_step": N, "server": "server4", ... }
// ------------------------------------------------

#include "service/handler/basehandler4.h"
#include "service/uievent/uieventservice4.h"
#include "common/protocol.h"
#include <functional>
#include <unordered_map>

// ------------------------------------------------
// uieventhandler4
// UI 이벤트를 uieventservice4로 위임
// ------------------------------------------------
class uieventhandler4 : public basehandler4
{
public:
    // 생성자: handlermap 초기화
    uieventhandler4();

    // ------------------------------------------------
    // handle
    // packet4를 받아 handlermap에서 처리 함수 호출
    // ------------------------------------------------
    void handle(const packet4& pkt) override;

private:
    // internal_protocol → 처리 함수 매핑
    std::unordered_map<int, std::function<void(const packet4&)>> handlermap;

    // UI 이벤트 비즈니스 로직 처리 객체
    uieventservice4 service;
};
