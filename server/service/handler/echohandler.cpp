// ------------------------------------------------
// echohandler.cpp
// echohandler 구현부
// internal_protocol → handlermap → echoservice 흐름
// ------------------------------------------------

#include "service/handler/echohandler.h"
#include "common/logger.h"

// ------------------------------------------------
// 생성자
// internal_protocol enum 값을 int로 캐스팅하여
// handlermap에 처리 함수를 등록한다
// ------------------------------------------------
echohandler::echohandler()
{
    // echo 처리 함수 등록
    handlermap[static_cast<int>(internal_protocol::echo)] =
        [this](const packet& pkt)
        {
            log_event("echohandler",
                      "echo 처리 함수 호출",
                      pkt.traceid);
            log_flow(pkt.traceid, "echohandler", "echoservice",
                     "proto=echo");
            service.process(pkt);
        };

    // ping 처리 함수 등록
    handlermap[static_cast<int>(internal_protocol::ping)] =
        [this](const packet& pkt)
        {
            log_event("echohandler",
                      "ping 처리 함수 호출",
                      pkt.traceid);
            log_flow(pkt.traceid, "echohandler", "echoservice",
                     "proto=ping");
            service.process(pkt);
        };
}

// ------------------------------------------------
// handle
// packet의 proto 값으로 handlermap 조회
// 등록된 처리 함수가 있으면 호출, 없으면 unknown 처리
// ------------------------------------------------
void echohandler::handle(const packet& pkt)
{
    log_event("echohandler",
              "요청 수신 | proto=" + protocol_to_string(pkt.protocol),
              pkt.traceid);
    log_flow(pkt.traceid, "router", "echohandler",
             "proto=" + protocol_to_string(pkt.protocol));

    int key = static_cast<int>(pkt.protocol);
    auto it = handlermap.find(key);

    if (it == handlermap.end())
    {
        // 매핑되지 않은 프로토콜: unknown 처리
        log_event("echohandler",
                  "처리 불가 | proto=" + protocol_to_string(pkt.protocol),
                  pkt.traceid);
        return;
    }

    // 등록된 처리 함수 호출
    it->second(pkt);
}