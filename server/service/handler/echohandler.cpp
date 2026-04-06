// ------------------------------------------------
// echohandler.cpp
// echohandler 구현부
// ------------------------------------------------

#include "service/handler/echohandler.h"
#include "common/logger.h"

echohandler::echohandler()
{
    handlermap[static_cast<int>(internal_protocol::echo)] =
        [this](const packet& pkt)
        {
            log_event("echohandler", "echo 처리 함수 호출", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "echohandler", "echoservice", "proto=echo");
            service.process(pkt);
        };

    handlermap[static_cast<int>(internal_protocol::ping)] =
        [this](const packet& pkt)
        {
            log_event("echohandler", "ping 처리 함수 호출", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "echohandler", "echoservice", "proto=ping");
            service.process(pkt);
        };
}

void echohandler::handle(const packet& pkt)
{
    log_event("echohandler",
              "요청 수신 | proto=" + protocol_to_string(pkt.ctx.proto),
              pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "router", "echohandler",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    int key = static_cast<int>(pkt.ctx.proto);
    auto it = handlermap.find(key);

    if (it == handlermap.end())
    {
        log_event("echohandler",
                  "처리 불가 | proto=" + protocol_to_string(pkt.ctx.proto),
                  pkt.ctx.traceid);
        return;
    }

    it->second(pkt);
}