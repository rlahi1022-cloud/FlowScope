// ------------------------------------------------
// router.cpp
// router 구현부
// target 필드 기반 포워딩 + 로컬 처리 분기
// ------------------------------------------------

#include "router/router.h"
#include "service/handler/echohandler.h"
#include "logger.h"
#include "traceid.h"

// ------------------------------------------------
// 생성자
// routemap에 internal_protocol → 핸들러 등록
// ------------------------------------------------
router::router()
{
    auto handler = std::make_shared<echohandler>();
    routemap[static_cast<int>(internal_protocol::echo)] = handler;
    routemap[static_cast<int>(internal_protocol::ping)] = handler;

    log_event("router", "라우팅 테이블 초기화 완료 | echo, ping 등록");
}

// ------------------------------------------------
// route
// 1. target 파싱 → 포워딩 or 로컬 분기
// 2. 로컬: cmd → internal_protocol → 핸들러 호출
// 3. 포워딩: forwarder.forward() 호출
// ------------------------------------------------
void router::route(int fd, const std::string& jsonbody)
{
    std::string traceid = generate_traceid();

    // target 파싱
    std::string target_str = parse_target(jsonbody);
    target_server target   = string_to_target(target_str);

    log_event("router",
              "요청 수신 | fd=" + std::to_string(fd) +
              " | target=" + target_to_string(target),
              traceid);
    log_flow(traceid, "epollserver", "router",
             "target=" + target_to_string(target));

    // 포워딩 대상이면 forwarder로 위임
    if (target != target_server::none && target != target_server::unknown)
    {
        log_event("router",
                  "포워딩 분기 | target=" + target_to_string(target),
                  traceid);
        forwarder_.forward(fd, target, jsonbody, traceid);
        return;
    }

    // unknown target 처리
    if (target == target_server::unknown)
    {
        log_event("router",
                  "알 수 없는 target | 처리 중단 | target=" + target_str,
                  traceid);
        return;
    }

    // 로컬 처리: cmd 파싱
    std::string cmd = parse_cmd(jsonbody);

    log_event("router",
              "로컬 처리 | fd=" + std::to_string(fd) +
              " | cmd=" + cmd,
              traceid);
    log_flow(traceid, "router", "handler", "cmd=" + cmd);

    internal_protocol proto = string_to_protocol(cmd);

    if (proto == internal_protocol::unknown)
    {
        log_event("router",
                  "알 수 없는 cmd | 처리 중단 | cmd=" + cmd,
                  traceid);
        return;
    }

    int key = static_cast<int>(proto);
    auto it = routemap.find(key);
    if (it == routemap.end())
    {
        log_event("router",
                  "핸들러 없음 | proto=" + protocol_to_string(proto),
                  traceid);
        return;
    }

    packet pkt;
    pkt.fd           = fd;
    pkt.ctx.proto    = proto;
    pkt.ctx.jsonbody = jsonbody;
    pkt.ctx.traceid  = traceid;
    pkt.ctx.cmd      = cmd;

    log_event("router",
              "핸들러 호출 | proto=" + protocol_to_string(proto),
              traceid);
    log_flow(traceid, "router", "echohandler",
             "proto=" + protocol_to_string(proto));

    it->second->handle(pkt);
}

// ------------------------------------------------
// parse_cmd
// JSON에서 "cmd" 필드 추출
// ------------------------------------------------
std::string router::parse_cmd(const std::string& jsonbody)
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

// ------------------------------------------------
// parse_target
// JSON에서 "target" 필드 추출
// 없으면 빈 문자열 반환 → none으로 처리
// ------------------------------------------------
std::string router::parse_target(const std::string& jsonbody)
{
    std::string key = "\"target\"";
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