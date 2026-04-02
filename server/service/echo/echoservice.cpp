// ------------------------------------------------
// echoservice.cpp
// echo 비즈니스 로직 구현부
// 처리 결과를 eventbus topic "response"로 publish
// ------------------------------------------------

#include "service/echo/echoservice.h"
#include "eventbus/eventbus.h"
#include "common/logger.h"

// ------------------------------------------------
// process
// internal_protocol에 따라 echo 또는 ping 처리
// 결과는 eventbus publish로만 전달 (직접 호출 금지)
// ------------------------------------------------
void echoservice::process(const packet& pkt)
{
    log_event("echoservice",
              "처리 시작 | proto=" + protocol_to_string(pkt.protocol),
              pkt.traceid);
    log_flow(pkt.traceid, "echohandler", "echoservice",
             "proto=" + protocol_to_string(pkt.protocol));

    std::string response;

    switch (pkt.protocol)
    {
        case internal_protocol::echo:
            response = build_echo_response(pkt.jsonbody, pkt.traceid);
            break;

        case internal_protocol::ping:
            response = build_ping_response(pkt.traceid);
            break;

        default:
            // unknown 타입은 처리하지 않는다
            log_event("echoservice",
                      "알 수 없는 프로토콜 - 처리 중단",
                      pkt.traceid);
            return;
    }

    log_event("echoservice",
              "처리 완료 | response=" + response,
              pkt.traceid);
    log_flow(pkt.traceid, "echoservice", "eventbus",
             "topic=response");

    // eventbus를 통해 결과 전달 (직접 fd write 금지)
    // topic "response": 응답 데이터 전달용 채널
    // data 형식: "fd:traceid:json_body"
    std::string payload = std::to_string(pkt.fd)
                          + ":" + pkt.traceid
                          + ":" + response;
    eventbus::instance().publish("response", payload);
}

// ------------------------------------------------
// build_echo_response
// 받은 body를 그대로 포함한 echo 응답 JSON 생성
// ------------------------------------------------
std::string echoservice::build_echo_response(const std::string& body,
                                              const std::string& traceid)
{
    // 수동 JSON 생성 (외부 라이브러리 의존성 없음)
    std::string json;
    json += "{";
    json += "\"cmd\":\"echo_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"data\":" + body;
    json += "}";
    return json;
}

// ------------------------------------------------
// build_ping_response
// ping에 대한 pong 응답 JSON 생성
// ------------------------------------------------
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