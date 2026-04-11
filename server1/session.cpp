// ------------------------------------------------
// session.cpp
// Thread-per-Connection 세션 구현부
// ------------------------------------------------

#include "session.h"
#include <unistd.h>
#include <cstring>
#include <cerrno>

session::session(int fd)
    : fd_(fd)
{
}

// ------------------------------------------------
// run
// 패킷 수신 루프
// 연결 종료 또는 에러 시 소켓 닫고 종료
// ------------------------------------------------
void session::run()
{
    log_event("session",
              "세션 시작 | fd=" + std::to_string(fd_));

    while (true)
    {
        std::string body;
        if (!recv_packet(body))
        {
            log_event("session",
                      "수신 종료 | fd=" + std::to_string(fd_));
            break;
        }

        std::string traceid = generate_traceid();
        std::string cmd     = parse_cmd(body);

        log_event("session",
                  "패킷 수신 | fd=" + std::to_string(fd_) +
                  " | cmd=" + cmd,
                  traceid);

        std::string response;
        internal_protocol proto = string_to_protocol(cmd);

        switch (proto)
        {
            case internal_protocol::echo:
                response = build_echo_response(body, traceid);
                break;

            case internal_protocol::ping:
                response = build_ping_response(traceid);
                break;

            // UI 이벤트 7종
            case internal_protocol::ui_btn_click:
            case internal_protocol::ui_server_select:
            case internal_protocol::ui_connect:
            case internal_protocol::ui_disconnect:
            case internal_protocol::ui_chat_msg:
            case internal_protocol::ui_flow_start:
            case internal_protocol::ui_flow_stop:
                response = build_ui_response(cmd, body, traceid);
                break;

            default:
                log_event("session",
                          "알 수 없는 cmd | 처리 중단 | cmd=" + cmd,
                          traceid);
                continue;
        }

        log_event("session",
                  "응답 전송 | fd=" + std::to_string(fd_) +
                  " | response=" + response,
                  traceid);

        if (!send_packet(response))
        {
            log_event("session",
                      "전송 실패 | fd=" + std::to_string(fd_));
            break;
        }
    }

    close(fd_);
    log_event("session",
              "세션 종료 | fd=" + std::to_string(fd_));
}

// ------------------------------------------------
// recv_packet
// 4byte 빅엔디언 헤더 수신 → bodylen 파싱 → body 수신
// ------------------------------------------------
bool session::recv_packet(std::string& out_body)
{
    uint8_t header[HEADER_SIZE];
    size_t  recvd = 0;

    while (recvd < static_cast<size_t>(HEADER_SIZE))
    {
        ssize_t n = read(fd_,
                         header + recvd,
                         HEADER_SIZE - recvd);
        if (n <= 0) return false;
        recvd += static_cast<size_t>(n);
    }

    uint32_t bodylen = 0;
    bodylen |= static_cast<uint32_t>(header[0]) << 24;
    bodylen |= static_cast<uint32_t>(header[1]) << 16;
    bodylen |= static_cast<uint32_t>(header[2]) << 8;
    bodylen |= static_cast<uint32_t>(header[3]);

    if (bodylen > static_cast<uint32_t>(MAX_BODY_SIZE))
        return false;

    out_body.resize(bodylen);
    size_t total = bodylen;
    recvd = 0;

    while (recvd < total)
    {
        ssize_t n = read(fd_,
                         &out_body[recvd],
                         total - recvd);
        if (n <= 0) return false;
        recvd += static_cast<size_t>(n);
    }

    return true;
}

// ------------------------------------------------
// send_packet
// 4byte 빅엔디언 헤더 + body 전송
// ------------------------------------------------
bool session::send_packet(const std::string& body)
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
        ssize_t n = write(fd_,
                          pkt.data() + sent,
                          total - sent);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        sent += static_cast<size_t>(n);
    }

    return true;
}

// ------------------------------------------------
// parse_cmd
// JSON에서 "cmd" 필드 추출
// ------------------------------------------------
std::string session::parse_cmd(const std::string& jsonbody)
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
// build_echo_response
// ------------------------------------------------
std::string session::build_echo_response(const std::string& body,
                                          const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"echo_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server1\",";
    json += "\"data\":" + body;
    json += "}";
    return json;
}

// ------------------------------------------------
// build_ping_response
// ------------------------------------------------
std::string session::build_ping_response(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"pong\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server1\",";
    json += "\"data\":\"pong\"";
    json += "}";
    return json;
}

// ------------------------------------------------
// get_response_cmd
// UI cmd 문자열을 response cmd로 변환
// 특수: ui_chat_msg → ui_chat_response (클라이언트 규칙)
// ------------------------------------------------
std::string session::get_response_cmd(const std::string& cmd)
{
    if (cmd == "ui_chat_msg") return "ui_chat_response";
    return cmd + "_response";
}

// ------------------------------------------------
// parse_field
// JSON에서 특정 문자열 필드 값 추출
// ------------------------------------------------
std::string session::parse_field(const std::string& json,
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
// flow_step: 클라이언트 흐름도 업데이트용 단계 번호
// ------------------------------------------------
std::string session::build_ui_response(const std::string& cmd,
                                        const std::string& body,
                                        const std::string& traceid)
{
    std::string resp_cmd = get_response_cmd(cmd);
    int flow_step = -1;
    std::string message;

    if (cmd == "ui_btn_click")
    {
        flow_step = 1;
        std::string btn = parse_field(body, "button");
        message = "[Server1] btn_click: " + btn;
    }
    else if (cmd == "ui_server_select")
    {
        flow_step = -1;
        std::string srv = parse_field(body, "server");
        message = "[Server1] server_select: " + srv;
    }
    else if (cmd == "ui_connect")
    {
        flow_step = 0;
        message = "[Server1] connected";
    }
    else if (cmd == "ui_disconnect")
    {
        flow_step = -1;
        message = "[Server1] disconnected";
    }
    else if (cmd == "ui_chat_msg")
    {
        flow_step = 3;
        std::string msg = parse_field(body, "message");
        message = "[Server1 Echo] " + msg;
    }
    else if (cmd == "ui_flow_start")
    {
        flow_step = 0;
        message = "[Server1] flow started";
    }
    else if (cmd == "ui_flow_stop")
    {
        flow_step = -1;
        message = "[Server1] flow stopped";
    }

    std::string json;
    json += "{";
    json += "\"cmd\":\"" + resp_cmd + "\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server1\",";
    json += "\"flow_step\":" + std::to_string(flow_step) + ",";
    json += "\"data\":{";
    json += "\"message\":\"" + message + "\",";
    json += "\"status\":\"ok\"";
    json += "}";
    json += "}";

    log_event("session",
              "UI 응답 생성 | cmd=" + resp_cmd +
              " | flow_step=" + std::to_string(flow_step),
              "", traceid);

    return json;
}