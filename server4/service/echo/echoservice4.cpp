// ------------------------------------------------
// echoservice4.cpp
// server4 echo/ping/debug_status 서비스 구현부
// ------------------------------------------------

#include "service/echo/echoservice4.h"
#include "common/logger.h"
#include "eventbus/eventbus4.h"

// ------------------------------------------------
// process
// proto에 따라 echo/ping/debug_status 응답을 생성하고
// eventbus4 "response" 토픽에 publish
// ------------------------------------------------
void echoservice4::process(const packet4& pkt)
{
    log_event("echoservice4",
              "처리 시작 | proto=" + protocol_to_string(pkt.ctx.proto),
              "", pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "echohandler4", "echoservice4",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    std::string response;

    switch (pkt.ctx.proto)
    {
        case internal_protocol::echo:
            response = build_echo_response(pkt.ctx.jsonbody,
                                          pkt.ctx.traceid);
            break;

        case internal_protocol::ping:
            response = build_ping_response(pkt.ctx.traceid);
            break;

        case internal_protocol::debug_status:
            response = build_debug_status_response(pkt.ctx.traceid);
            break;

        default:
            log_event("echoservice4",
                      "알 수 없는 프로토콜 - 처리 중단",
                      "", pkt.ctx.traceid);
            return;
    }

    log_event("echoservice4",
              "처리 완료 - eventbus4에 publish | fd=" + std::to_string(pkt.fd),
              "response=" + response, pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "echoservice4", "eventbus4 (response)",
             "fd=" + std::to_string(pkt.fd));

    // EventBus 거쳐서 publish (main.cpp의 response subscriber가 처리)
    publish_response(pkt.fd, pkt.ctx.traceid, response);
}

// ------------------------------------------------
// build_echo_response
// echo 응답 JSON 생성
// ------------------------------------------------
std::string echoservice4::build_echo_response(const std::string& body,
                                              const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"echo_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server4\",";
    json += "\"data\":" + body;
    json += "}";
    return json;
}

// ------------------------------------------------
// build_ping_response
// ping 응답 JSON 생성
// ------------------------------------------------
std::string echoservice4::build_ping_response(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"pong\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server4\",";
    json += "\"data\":\"pong\"";
    json += "}";
    return json;
}

// ------------------------------------------------
// build_debug_status_response
// debug_status 응답 JSON 생성
// ------------------------------------------------
std::string echoservice4::build_debug_status_response(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"debug_status_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server4\",";
    json += "\"data\":{";
    json += "\"mode\":\"hybrid\",";
    json += "\"components\":[\"epoll\",\"dispatcher\",\"eventbus\",\"jobqueue\",\"workerpool\"]";
    json += "}";
    json += "}";
    return json;
}

// ------------------------------------------------
// publish_response
// 응답을 eventbus4 "response" 토픽에 publish
// 형식: "fd:traceid:json"
// ------------------------------------------------
void echoservice4::publish_response(int fd,
                                    const std::string& traceid,
                                    const std::string& json)
{
    std::string data = std::to_string(fd) + ":" + traceid + ":" + json;
    eventbus4::instance().publish("response", data);

    log_event("echoservice4",
              "eventbus4 publish 완료 | topic=response",
              "fd=" + std::to_string(fd), traceid);
}
