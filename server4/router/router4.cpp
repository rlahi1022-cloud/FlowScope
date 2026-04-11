// ------------------------------------------------
// router4.cpp
// server4 라우터 구현부
// ------------------------------------------------

#include "router/router4.h"
#include "service/handler/echohandler4.h"
#include "service/handler/uieventhandler4.h"
#include "common/logger.h"
#include "common/traceid.h"
#include "eventbus/eventbus4.h"

// jobqueue4 전역 인스턴스 (main.cpp에서 참조 가능하도록 선언)
extern jobqueue4* g_jobqueue4_ptr;

// ------------------------------------------------
// 생성자
// routemap에 proto → 핸들러 등록
// ------------------------------------------------
router4::router4()
{
    // echo / ping 핸들러
    auto echo_handler = std::make_shared<echohandler4>();
    routemap[static_cast<int>(internal_protocol::echo)] = echo_handler;
    routemap[static_cast<int>(internal_protocol::ping)] = echo_handler;
    routemap[static_cast<int>(internal_protocol::debug_status)] = echo_handler;

    // ui_* 이벤트 핸들러
    auto ui_handler = std::make_shared<uieventhandler4>();
    routemap[static_cast<int>(internal_protocol::ui_btn_click)] = ui_handler;
    routemap[static_cast<int>(internal_protocol::ui_server_select)] = ui_handler;
    routemap[static_cast<int>(internal_protocol::ui_connect)] = ui_handler;
    routemap[static_cast<int>(internal_protocol::ui_disconnect)] = ui_handler;
    routemap[static_cast<int>(internal_protocol::ui_chat_msg)] = ui_handler;
    routemap[static_cast<int>(internal_protocol::ui_flow_start)] = ui_handler;
    routemap[static_cast<int>(internal_protocol::ui_flow_stop)] = ui_handler;

    log_event("router4", "라우팅 테이블 초기화 완료");
}

// ------------------------------------------------
// route
// 1. cmd 파싱 → proto 변환
// 2. traceid 발급
// 3. dispatcher로 처리 경로 결정
// 4. 경로별 처리 수행
// ------------------------------------------------
void router4::route(int fd, const std::string& jsonbody)
{
    std::string traceid = generate_traceid();
    std::string cmd     = parse_cmd(jsonbody);

    log_event("router4",
              "요청 수신 | fd=" + std::to_string(fd) +
              " | cmd=" + cmd,
              "", traceid);
    log_flow(traceid, "epollserver4", "router4", "cmd=" + cmd);

    internal_protocol proto = string_to_protocol(cmd);

    if (proto == internal_protocol::unknown)
    {
        log_event("router4",
                  "알 수 없는 cmd | 처리 중단 | cmd=" + cmd,
                  "", traceid);
        return;
    }

    // dispatcher로 처리 경로 결정
    dispatch_type dtype = dispatch4::determine(proto);

    if (dtype == dispatch_type::unknown)
    {
        log_event("router4",
                  "분기 불가 | proto=" + protocol_to_string(proto),
                  "", traceid);
        return;
    }

    // packet4 구성
    requestcontext ctx;
    ctx.traceid  = traceid;
    ctx.cmd      = cmd;
    ctx.proto    = proto;
    ctx.jsonbody = jsonbody;

    packet4 pkt(fd, ctx);

    // 처리 경로별 분기
    if (dtype == dispatch_type::sync)
    {
        // 핸들러 직접 호출
        int key = static_cast<int>(proto);
        auto it = routemap.find(key);

        if (it == routemap.end())
        {
            log_event("router4",
                      "핸들러 없음 (sync) | proto=" + protocol_to_string(proto),
                      "", traceid);
            return;
        }

        log_event("router4",
                  "sync 경로 | proto=" + protocol_to_string(proto),
                  "", traceid);
        log_flow(traceid, "router4", "handler (sync)",
                 "proto=" + protocol_to_string(proto));

        it->second->handle(pkt);
    }
    else if (dtype == dispatch_type::async)
    {
        // jobqueue4에 push (nullptr 체크는 main.cpp에서 보장)
        if (!g_jobqueue4_ptr)
        {
            log_event("router4",
                      "jobqueue4 포인터 없음 (초기화 되지 않음)",
                      "", traceid);
            return;
        }

        job4 job;
        job.fd = fd;
        job.jsonbody = jsonbody;

        if (!g_jobqueue4_ptr->push(job))
        {
            log_event("router4",
                      "jobqueue4 push 실패 (backpressure)",
                      "", traceid);
            return;
        }

        log_event("router4",
                  "async 경로 | jobqueue4 push | proto=" + protocol_to_string(proto),
                  "", traceid);
        log_flow(traceid, "router4", "jobqueue4",
                 "proto=" + protocol_to_string(proto));
    }
    else if (dtype == dispatch_type::event)
    {
        // eventbus4에 publish (관심 있는 구독자에게 전달)
        eventbus4::instance().publish(cmd, jsonbody);

        log_event("router4",
                  "event 경로 | eventbus4 publish | proto=" + protocol_to_string(proto),
                  "", traceid);
        log_flow(traceid, "router4", "eventbus4",
                 "proto=" + protocol_to_string(proto));
    }
}

// ------------------------------------------------
// parse_cmd
// JSON에서 "cmd" 필드 추출
// ------------------------------------------------
std::string router4::parse_cmd(const std::string& jsonbody)
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
// parse_traceid
// JSON에서 기존 traceid 추출 (없으면 빈 문자열)
// ------------------------------------------------
std::string router4::parse_traceid(const std::string& jsonbody)
{
    std::string key = "\"traceid\"";
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
