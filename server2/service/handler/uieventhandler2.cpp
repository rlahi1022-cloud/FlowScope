// ------------------------------------------------
// uieventhandler2.cpp
// server2 UI 이벤트 핸들러 구현부
// ------------------------------------------------

#include "service/handler/uieventhandler2.h"
#include "common/logger.h"

// ------------------------------------------------
// 생성자
// 7개 UI 프로토콜 → 처리 함수 매핑 등록
// ------------------------------------------------
uieventhandler2::uieventhandler2()
{
    auto register_ui = [this](internal_protocol proto) {
        handlermap[static_cast<int>(proto)] =
            [this, proto](const packet2& pkt)
            {
                log_event("uieventhandler2",
                          protocol_to_string(proto) + " 처리 함수 호출",
                          "", pkt.ctx.traceid);
                log_flow(pkt.ctx.traceid, "uieventhandler2", "uieventservice2",
                         "proto=" + protocol_to_string(proto));
                service.process(pkt);
            };
    };

    register_ui(internal_protocol::ui_btn_click);
    register_ui(internal_protocol::ui_server_select);
    register_ui(internal_protocol::ui_connect);
    register_ui(internal_protocol::ui_disconnect);
    register_ui(internal_protocol::ui_chat_msg);
    register_ui(internal_protocol::ui_flow_start);
    register_ui(internal_protocol::ui_flow_stop);
}

// ------------------------------------------------
// handle
// handlermap에서 proto 기반 처리 함수 호출
// ------------------------------------------------
void uieventhandler2::handle(const packet2& pkt)
{
    log_event("uieventhandler2",
              "요청 수신 | proto=" + protocol_to_string(pkt.ctx.proto),
              "", pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "router2", "uieventhandler2",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    int key = static_cast<int>(pkt.ctx.proto);
    auto it = handlermap.find(key);

    if (it == handlermap.end())
    {
        log_event("uieventhandler2",
                  "처리 불가 | proto=" + protocol_to_string(pkt.ctx.proto),
                  "", pkt.ctx.traceid);
        return;
    }

    it->second(pkt);
}
