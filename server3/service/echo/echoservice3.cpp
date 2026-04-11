// ------------------------------------------------
// echoservice3.cpp
// server3 echo 비즈니스 로직 구현부
//
// server3 핵심 특징:
//   EventBus 경유: 처리 완료 후 eventbus.publish("response", ...)
//   응답 수신: main.cpp의 response 토픽 구독자가 fd로 write
// ------------------------------------------------

#include "service/echo/echoservice3.h"
#include "eventbus/eventbus3.h"
#include "common/logger.h"
#include "common/protocol.h"

// ------------------------------------------------
// process
// payload 파싱 → echo/ping 처리 → EventBus publish("response", ...)
// ------------------------------------------------
void echoservice3::process(const std::string& payload)
{
    int fd;
    std::string traceid;
    std::string jsonbody;

    if (!parse_payload(payload, fd, traceid, jsonbody))
    {
        log_event("echoservice3",
                  "payload 파싱 실패 | 처리 중단",
                  "payload=" + payload);
        return;
    }

    std::string cmd = parse_cmd(jsonbody);
    internal_protocol proto = string_to_protocol(cmd);

    log_event("echoservice3",
              "처리 시작 | proto=" + protocol_to_string(proto),
              "", traceid);
    log_flow(traceid, "echohandler3", "echoservice3",
             "proto=" + protocol_to_string(proto));

    std::string response;

    switch (proto)
    {
        case internal_protocol::echo:
            response = build_echo_response(jsonbody, traceid);
            break;

        case internal_protocol::ping:
            response = build_ping_response(traceid);
            break;

        default:
            log_event("echoservice3",
                      "알 수 없는 프로토콜 - 처리 중단",
                      "", traceid);
            return;
    }

    log_event("echoservice3",
              "처리 완료 - EventBus publish | fd=" + std::to_string(fd),
              "response=" + response.substr(0, 100) + "...", traceid);
    log_flow(traceid, "echoservice3", "eventbus3",
             "topic=response | fd=" + std::to_string(fd));

    // EventBus publish: payload = "fd:traceid:json"
    std::string response_payload = std::to_string(fd) + ":" + traceid + ":" + response;
    eventbus3::instance().publish("response", response_payload);
}

// ------------------------------------------------
// parse_payload
// payload 파싱: "fd:traceid:jsonbody"
// ------------------------------------------------
bool echoservice3::parse_payload(const std::string& payload,
                                  int& fd,
                                  std::string& traceid,
                                  std::string& jsonbody)
{
    // 첫 번째 ':' 찾기 (fd와 traceid 구분)
    size_t pos1 = payload.find(':');
    if (pos1 == std::string::npos) return false;

    // fd 파싱
    try {
        fd = std::stoi(payload.substr(0, pos1));
    } catch (...) {
        return false;
    }

    // 두 번째 ':' 찾기 (traceid와 jsonbody 구분)
    size_t pos2 = payload.find(':', pos1 + 1);
    if (pos2 == std::string::npos) return false;

    traceid = payload.substr(pos1 + 1, pos2 - pos1 - 1);
    jsonbody = payload.substr(pos2 + 1);

    return true;
}

// ------------------------------------------------
// build_echo_response
// echo 응답 JSON 생성
// ------------------------------------------------
std::string echoservice3::build_echo_response(const std::string& jsonbody,
                                               const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"echo_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server3\",";
    json += "\"data\":" + jsonbody;
    json += "}";
    return json;
}

// ------------------------------------------------
// build_ping_response
// ping 응답 JSON 생성
// ------------------------------------------------
std::string echoservice3::build_ping_response(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"pong\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server3\",";
    json += "\"data\":\"pong\"";
    json += "}";
    return json;
}

// ------------------------------------------------
// parse_cmd
// JSON에서 "cmd" 필드 추출
// ------------------------------------------------
std::string echoservice3::parse_cmd(const std::string& jsonbody)
{
    std::string key = "\"cmd\"";
    size_t pos = jsonbody.find(key);
    if (pos == std::string::npos) return "";

    pos = jsonbody.find(':', pos + key.size());
    if (pos == std::string::npos) return "";

    ++pos;
    while (pos < jsonbody.size() && jsonbody[pos] == ' ') ++pos;
    if (pos >= jsonbody.size() || jsonbody[pos] != '"') return "";
    ++pos;

    size_t end = jsonbody.find('"', pos);
    if (end == std::string::npos) return "";

    return jsonbody.substr(pos, end - pos);
}
