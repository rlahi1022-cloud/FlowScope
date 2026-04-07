// ------------------------------------------------
// router2.cpp
// server2 라우터 구현부
// ------------------------------------------------

#include "router/router2.h"
#include "service/handler/echohandler2.h"
#include "common/logger.h"
#include "common/traceid.h"

// ------------------------------------------------
// 생성자
// routemap에 internal_protocol → 핸들러 등록
// ------------------------------------------------
router2::router2()
{
    auto handler = std::make_shared<echohandler2>();

    // echo / ping → echohandler2 로 라우팅
    routemap[static_cast<int>(internal_protocol::echo)] = handler;
    routemap[static_cast<int>(internal_protocol::ping)] = handler;

    log_event("router2", "라우팅 테이블 초기화 완료 | echo, ping 등록");
}

// ------------------------------------------------
// route
// 1. cmd 파싱 → proto 변환
// 2. traceid 발급
// 3. packet2 생성 → 핸들러 호출
// ------------------------------------------------
void router2::route(int fd, const std::string& jsonbody)
{
    std::string traceid = generate_traceid();
    std::string cmd     = parse_cmd(jsonbody);

    log_event("router2",
              "요청 수신 | fd=" + std::to_string(fd) +
              " | cmd=" + cmd,
              "", traceid);
    log_flow(traceid, "epollserver2", "router2", "cmd=" + cmd);

    internal_protocol proto = string_to_protocol(cmd);

    if (proto == internal_protocol::unknown)
    {
        log_event("router2",
                  "알 수 없는 cmd | 처리 중단 | cmd=" + cmd,
                  "", traceid);
        return;
    }

    int key = static_cast<int>(proto);
    auto it = routemap.find(key);

    if (it == routemap.end())
    {
        log_event("router2",
                  "핸들러 없음 | proto=" + protocol_to_string(proto),
                  "", traceid);
        return;
    }

    // packet2 구성
    requestcontext ctx;
    ctx.traceid  = traceid;
    ctx.cmd      = cmd;
    ctx.proto    = proto;
    ctx.jsonbody = jsonbody;

    packet2 pkt(fd, ctx);

    log_event("router2",
              "핸들러 호출 | proto=" + protocol_to_string(proto),
              "", traceid);
    log_flow(traceid, "router2", "echohandler2",
             "proto=" + protocol_to_string(proto));

    it->second->handle(pkt);
}

// ------------------------------------------------
// parse_cmd
// JSON에서 "cmd" 필드 추출
// ------------------------------------------------
std::string router2::parse_cmd(const std::string& jsonbody)
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