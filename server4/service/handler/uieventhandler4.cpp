// ------------------------------------------------
// uieventhandler4.cpp
// server4 UI 이벤트 핸들러 구현부
// ------------------------------------------------

#include "service/handler/uieventhandler4.h"
#include "common/logger.h"

// ------------------------------------------------
// 생성자
// UI 이벤트별 처리 함수 등록
// ------------------------------------------------
uieventhandler4::uieventhandler4()
{
    // ui_btn_click
    handlermap[static_cast<int>(internal_protocol::ui_btn_click)] =
        [this](const packet4& pkt)
        {
            log_event("uieventhandler4", "ui_btn_click 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "uieventhandler4", "uieventservice4",
                     "proto=ui_btn_click");
            service.process(pkt);
        };

    // ui_server_select
    handlermap[static_cast<int>(internal_protocol::ui_server_select)] =
        [this](const packet4& pkt)
        {
            log_event("uieventhandler4", "ui_server_select 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "uieventhandler4", "uieventservice4",
                     "proto=ui_server_select");
            service.process(pkt);
        };

    // ui_connect
    handlermap[static_cast<int>(internal_protocol::ui_connect)] =
        [this](const packet4& pkt)
        {
            log_event("uieventhandler4", "ui_connect 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "uieventhandler4", "uieventservice4",
                     "proto=ui_connect");
            service.process(pkt);
        };

    // ui_disconnect
    handlermap[static_cast<int>(internal_protocol::ui_disconnect)] =
        [this](const packet4& pkt)
        {
            log_event("uieventhandler4", "ui_disconnect 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "uieventhandler4", "uieventservice4",
                     "proto=ui_disconnect");
            service.process(pkt);
        };

    // ui_chat_msg
    handlermap[static_cast<int>(internal_protocol::ui_chat_msg)] =
        [this](const packet4& pkt)
        {
            log_event("uieventhandler4", "ui_chat_msg 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "uieventhandler4", "uieventservice4",
                     "proto=ui_chat_msg");
            service.process(pkt);
        };

    // ui_flow_start
    handlermap[static_cast<int>(internal_protocol::ui_flow_start)] =
        [this](const packet4& pkt)
        {
            log_event("uieventhandler4", "ui_flow_start 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "uieventhandler4", "uieventservice4",
                     "proto=ui_flow_start");
            service.process(pkt);
        };

    // ui_flow_stop
    handlermap[static_cast<int>(internal_protocol::ui_flow_stop)] =
        [this](const packet4& pkt)
        {
            log_event("uieventhandler4", "ui_flow_stop 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "uieventhandler4", "uieventservice4",
                     "proto=ui_flow_stop");
            service.process(pkt);
        };
}

// ------------------------------------------------
// handle
// handlermap에서 proto 기반 처리 함수 호출
// ------------------------------------------------
void uieventhandler4::handle(const packet4& pkt)
{
    log_event("uieventhandler4",
              "요청 수신 | proto=" + protocol_to_string(pkt.ctx.proto),
              "", pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "router4", "uieventhandler4",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    int key = static_cast<int>(pkt.ctx.proto);
    auto it = handlermap.find(key);

    if (it == handlermap.end())
    {
        log_event("uieventhandler4",
                  "처리 불가 | proto=" + protocol_to_string(pkt.ctx.proto),
                  "", pkt.ctx.traceid);
        return;
    }

    it->second(pkt);
}
