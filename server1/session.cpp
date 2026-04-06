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