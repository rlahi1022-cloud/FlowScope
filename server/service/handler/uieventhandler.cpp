// ------------------------------------------------
// uieventhandler.cpp
// uieventhandler 구현부
// ------------------------------------------------

#include "service/handler/uieventhandler.h"
#include "common/logger.h"

uieventhandler::uieventhandler()
{
    // 모든 UI 이벤트 프로토콜을 동일한 service.process()로 위임
    // uieventservice 내부에서 proto별 분기 처리

    auto register_ui = [this](internal_protocol proto)
    {
        handlermap[static_cast<int>(proto)] =
            [this, proto](const packet& pkt)
            {
                log_event("uieventhandler",
                          protocol_to_string(proto) + " 처리 함수 호출",
                          pkt.ctx.traceid);
                log_flow(pkt.ctx.traceid, "uieventhandler", "uieventservice",
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

    log_event("uieventhandler", "UI 이벤트 핸들러 초기화 완료 | 7개 프로토콜 등록");
}

void uieventhandler::handle(const packet& pkt)
{
    log_event("uieventhandler",
              "요청 수신 | proto=" + protocol_to_string(pkt.ctx.proto),
              pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "router", "uieventhandler",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    int key = static_cast<int>(pkt.ctx.proto);
    auto it = handlermap.find(key);

    if (it == handlermap.end())
    {
        log_event("uieventhandler",
                  "처리 불가 | proto=" + protocol_to_string(pkt.ctx.proto),
                  pkt.ctx.traceid);
        return;
    }

    it->second(pkt);
}
