// ------------------------------------------------
// uieventservice2.cpp
// server2 UI 이벤트 비즈니스 로직 구현부
//
// server2 핵심 특징:
//   EventBus 없음 → 처리 완료 후 fd로 직접 write
//   epoll 스레드에서 즉시 처리 (블로킹 없음)
// ------------------------------------------------

#include "service/uievent/uieventservice2.h"
#include "common/logger.h"
#include "common/packet.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

// ------------------------------------------------
// process
// proto에 따라 UI 응답을 생성하고
// fd로 직접 전송한다
// ------------------------------------------------
void uieventservice2::process(const packet2& pkt)
{
    log_event("uieventservice2",
              "처리 시작 | proto=" + protocol_to_string(pkt.ctx.proto),
              "", pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "uieventhandler2", "uieventservice2",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    std::string response = build_ui_response(pkt);

    if (response.empty())
    {
        log_event("uieventservice2",
                  "알 수 없는 UI 프로토콜 - 처리 중단",
                  "", pkt.ctx.traceid);
        return;
    }

    log_event("uieventservice2",
              "처리 완료 - fd로 직접 write | fd=" + std::to_string(pkt.fd),
              "response=" + response, pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "uieventservice2", "client(fd)",
             "fd=" + std::to_string(pkt.fd) + " | 직접 write");

    send_response(pkt.fd, response);
}

// ------------------------------------------------
// get_response_cmd
// UI cmd → response cmd 변환
// 특수: ui_chat_msg → ui_chat_response
// ------------------------------------------------
std::string uieventservice2::get_response_cmd(const std::string& cmd)
{
    if (cmd == "ui_chat_msg") return "ui_chat_response";
    return cmd + "_response";
}

// ------------------------------------------------
// parse_field
// JSON에서 특정 문자열 필드 값 추출
// ------------------------------------------------
std::string uieventservice2::parse_field(const std::string& json,
                                          const std::string& field)
{
    std::string key = "\"" + field + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + key.size());
    if (pos == std::string::npos) return "";

    ++pos;
    while (pos < json.size() && json[pos] == ' ') ++pos;
    if (pos >= json.size() || json[pos] != '"') return "";
    ++pos;

    size_t end = json.find('"', pos);
    if (end == std::string::npos) return "";

    return json.substr(pos, end - pos);
}

// ------------------------------------------------
// build_ui_response
// 7가지 UI 이벤트에 대한 응답 JSON 생성
// ------------------------------------------------
std::string uieventservice2::build_ui_response(const packet2& pkt)
{
    std::string cmd      = pkt.ctx.cmd;
    std::string resp_cmd = get_response_cmd(cmd);
    int flow_step        = -1;
    std::string message;

    switch (pkt.ctx.proto)
    {
        case internal_protocol::ui_btn_click:
        {
            flow_step = 1;
            std::string btn = parse_field(pkt.ctx.jsonbody, "button");
            message = "[Server2] btn_click: " + btn;
            break;
        }
        case internal_protocol::ui_server_select:
        {
            flow_step = -1;
            std::string srv = parse_field(pkt.ctx.jsonbody, "server");
            message = "[Server2] server_select: " + srv;
            break;
        }
        case internal_protocol::ui_connect:
        {
            flow_step = 0;
            message = "[Server2] connected";
            break;
        }
        case internal_protocol::ui_disconnect:
        {
            flow_step = -1;
            message = "[Server2] disconnected";
            break;
        }
        case internal_protocol::ui_chat_msg:
        {
            flow_step = 3;
            std::string msg = parse_field(pkt.ctx.jsonbody, "message");
            message = "[Server2 Echo] " + msg;
            break;
        }
        case internal_protocol::ui_flow_start:
        {
            flow_step = 0;
            message = "[Server2] flow started";
            break;
        }
        case internal_protocol::ui_flow_stop:
        {
            flow_step = -1;
            message = "[Server2] flow stopped";
            break;
        }
        default:
            return "";
    }

    std::string json;
    json += "{";
    json += "\"cmd\":\"" + resp_cmd + "\",";
    json += "\"traceid\":\"" + pkt.ctx.traceid + "\",";
    json += "\"server\":\"server2\",";
    json += "\"flow_step\":" + std::to_string(flow_step) + ",";
    json += "\"data\":{";
    json += "\"message\":\"" + message + "\",";
    json += "\"status\":\"ok\"";
    json += "}";
    json += "}";

    return json;
}

// ------------------------------------------------
// send_response
// 4byte 빅엔디언 헤더 + body를 fd로 전송
// ------------------------------------------------
void uieventservice2::send_response(int fd, const std::string& body)
{
    uint32_t bodylen = static_cast<uint32_t>(body.size());

    uint8_t header[HEADER_SIZE];
    header[0] = (bodylen >> 24) & 0xFF;
    header[1] = (bodylen >> 16) & 0xFF;
    header[2] = (bodylen >>  8) & 0xFF;
    header[3] = (bodylen      ) & 0xFF;

    std::string pkt;
    pkt.append(reinterpret_cast<char*>(header), HEADER_SIZE);
    pkt.append(body);

    size_t total = pkt.size();
    size_t sent  = 0;

    while (sent < total)
    {
        ssize_t n = write(fd,
                          pkt.data() + sent,
                          total - sent);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            log_event("uieventservice2",
                      "write 실패 | fd=" + std::to_string(fd) +
                      " | err=" + strerror(errno));
            return;
        }
        sent += static_cast<size_t>(n);
    }

    log_event("uieventservice2",
              "write 완료 | fd=" + std::to_string(fd) +
              " | bytes=" + std::to_string(total));
}
