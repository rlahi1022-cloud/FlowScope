// ------------------------------------------------
// echoservice.cpp
// echo 비즈니스 로직 구현부
// 처리 결과를 eventbus topic "response"로 publish
// ------------------------------------------------

#include "service/echo/echoservice.h"
#include "eventbus/eventbus.h"
#include "common/logger.h"

void echoservice::process(const packet& pkt)
{
    log_event("echoservice",
              "처리 시작 | proto=" + protocol_to_string(pkt.ctx.proto),
              pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "echohandler", "echoservice",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    std::string response;

    switch (pkt.ctx.proto)
    {
        case internal_protocol::echo:
            response = build_echo_response(pkt.ctx.jsonbody, pkt.ctx.traceid);
            break;

        case internal_protocol::ping:
            response = build_ping_response(pkt.ctx.traceid);
            break;

        default:
            log_event("echoservice",
                      "알 수 없는 프로토콜 - 처리 중단",
                      pkt.ctx.traceid);
            return;
    }

    log_event("echoservice",
              "처리 완료 | response=" + response,
              pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "echoservice", "eventbus",
             "topic=response");

    std::string payload = std::to_string(pkt.fd)
                          + ":" + pkt.ctx.traceid
                          + ":" + response;
    eventbus::instance().publish("response", payload);
}

std::string echoservice::build_echo_response(const std::string& body,
                                              const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"echo_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"data\":" + body;
    json += "}";
    return json;
}

std::string echoservice::build_ping_response(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"pong\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"data\":\"pong\"";
    json += "}";
    return json;
}