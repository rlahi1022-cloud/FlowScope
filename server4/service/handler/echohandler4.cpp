// ------------------------------------------------
// echohandler4.cpp
// server4 echo/ping/debug_status 핸들러 구현부
// ------------------------------------------------

#include "service/handler/echohandler4.h"
#include "common/logger.h"

// ------------------------------------------------
// 생성자
// internal_protocol → 처리 함수 매핑 등록
// ------------------------------------------------
echohandler4::echohandler4()
{
    // echo 처리 함수 등록
    handlermap[static_cast<int>(internal_protocol::echo)] =
        [this](const packet4& pkt)
        {
            log_event("echohandler4", "echo 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "echohandler4", "echoservice4",
                     "proto=echo");
            service.process(pkt);
        };

    // ping 처리 함수 등록
    handlermap[static_cast<int>(internal_protocol::ping)] =
        [this](const packet4& pkt)
        {
            log_event("echohandler4", "ping 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "echohandler4", "echoservice4",
                     "proto=ping");
            service.process(pkt);
        };

    // debug_status 처리 함수 등록
    handlermap[static_cast<int>(internal_protocol::debug_status)] =
        [this](const packet4& pkt)
        {
            log_event("echohandler4", "debug_status 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "echohandler4", "echoservice4",
                     "proto=debug_status");
            service.process(pkt);
        };
}

// ------------------------------------------------
// handle
// handlermap에서 proto 기반 처리 함수 호출
// ------------------------------------------------
void echohandler4::handle(const packet4& pkt)
{
    log_event("echohandler4",
              "요청 수신 | proto=" + protocol_to_string(pkt.ctx.proto),
              "", pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "router4", "echohandler4",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    int key = static_cast<int>(pkt.ctx.proto);
    auto it = handlermap.find(key);

    if (it == handlermap.end())
    {
        log_event("echohandler4",
                  "처리 불가 | proto=" + protocol_to_string(pkt.ctx.proto),
                  "", pkt.ctx.traceid);
        return;
    }

    it->second(pkt);
}
