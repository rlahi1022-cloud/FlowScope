// ------------------------------------------------
// echohandler2.cpp
// server2 echo/ping 핸들러 구현부
// ------------------------------------------------

#include "service/handler/echohandler2.h"
#include "common/logger.h"

// ------------------------------------------------
// 생성자
// internal_protocol → 처리 함수 매핑 등록
// ------------------------------------------------
echohandler2::echohandler2()
{
    // echo 처리 함수 등록
    handlermap[static_cast<int>(internal_protocol::echo)] =
        [this](const packet2& pkt)
        {
            log_event("echohandler2", "echo 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "echohandler2", "echoservice2",
                     "proto=echo");
            service.process(pkt);
        };

    // ping 처리 함수 등록
    handlermap[static_cast<int>(internal_protocol::ping)] =
        [this](const packet2& pkt)
        {
            log_event("echohandler2", "ping 처리 함수 호출",
                      "", pkt.ctx.traceid);
            log_flow(pkt.ctx.traceid, "echohandler2", "echoservice2",
                     "proto=ping");
            service.process(pkt);
        };
}

// ------------------------------------------------
// handle
// handlermap에서 proto 기반 처리 함수 호출
// ------------------------------------------------
void echohandler2::handle(const packet2& pkt)
{
    log_event("echohandler2",
              "요청 수신 | proto=" + protocol_to_string(pkt.ctx.proto),
              "", pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "router2", "echohandler2",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    int key = static_cast<int>(pkt.ctx.proto);
    auto it = handlermap.find(key);

    if (it == handlermap.end())
    {
        log_event("echohandler2",
                  "처리 불가 | proto=" + protocol_to_string(pkt.ctx.proto),
                  "", pkt.ctx.traceid);
        return;
    }

    it->second(pkt);
}