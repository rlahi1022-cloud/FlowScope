// ------------------------------------------------
// router3.cpp
// server3 라우터 구현부
// ------------------------------------------------

#include "router/router3.h"
#include "eventbus/eventbus3.h"
#include "common/logger.h"
#include "common/traceid.h"

// ------------------------------------------------
// 생성자
// ------------------------------------------------
router3::router3()
{
    log_event("router3", "라우터 초기화 완료");
}

// ------------------------------------------------
// route
// 1. cmd 파싱 → proto 변환
// 2. traceid 발급
// 3. packet3 생성 후 EventBus publish("request", payload)
// ------------------------------------------------
void router3::route(int fd, const std::string& jsonbody)
{
    std::string traceid = generate_traceid();
    std::string cmd     = parse_cmd(jsonbody);

    log_event("router3",
              "요청 수신 | fd=" + std::to_string(fd) +
              " | cmd=" + cmd,
              "", traceid);
    log_flow(traceid, "epollserver3", "router3", "cmd=" + cmd);

    internal_protocol proto = string_to_protocol(cmd);

    if (proto == internal_protocol::unknown)
    {
        log_event("router3",
                  "알 수 없는 cmd | EventBus publish 중단 | cmd=" + cmd,
                  "", traceid);
        return;
    }

    // packet3 구성
    requestcontext ctx;
    ctx.traceid  = traceid;
    ctx.cmd      = cmd;
    ctx.proto    = proto;
    ctx.jsonbody = jsonbody;

    packet3 pkt(fd, ctx);

    log_event("router3",
              "EventBus publish | topic=request | proto=" + protocol_to_string(proto),
              "", traceid);
    log_flow(traceid, "router3", "eventbus3",
             "topic=request | proto=" + protocol_to_string(proto));

    // EventBus publish: payload = "fd:traceid:jsonbody"
    std::string payload = std::to_string(fd) + ":" + traceid + ":" + jsonbody;
    eventbus3::instance().publish("request", payload);
}

// ------------------------------------------------
// parse_cmd
// JSON에서 "cmd" 필드 추출
// ------------------------------------------------
std::string router3::parse_cmd(const std::string& jsonbody)
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
