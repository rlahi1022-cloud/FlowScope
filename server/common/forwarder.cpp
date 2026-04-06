// ------------------------------------------------
// forwarder.cpp
// 중앙서버 → 하위 서버 TCP 포워딩 구현부
// ------------------------------------------------

#include "forwarder.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

// ------------------------------------------------
// forward
// target 서버에 연결 → 패킷 전송 → 응답 수신
// → 클라이언트로 전달
// ------------------------------------------------
void forwarder::forward(int client_fd,
                        target_server target,
                        const std::string& jsonbody,
                        const std::string& traceid)
{
    int port = target_to_port(target);
    std::string tgt = target_to_string(target);

    log_event("forwarder",
              "포워딩 시작 | target=" + tgt +
              " | port=" + std::to_string(port),
              traceid);
    log_flow(traceid, "router", "forwarder",
             "target=" + tgt);

    // 하위 서버에 TCP 연결
    int srvfd = connect_to(port);
    if (srvfd < 0)
    {
        log_event("forwarder",
                  "하위 서버 연결 실패 | target=" + tgt,
                  traceid);
        std::string err = make_error_response(
            "target server unavailable: " + tgt, traceid);
        send_to_client(client_fd, err);
        return;
    }

    log_flow(traceid, "forwarder", tgt, "연결 성공");

    // 하위 서버로 패킷 전송
    if (!send_packet(srvfd, jsonbody))
    {
        log_event("forwarder",
                  "하위 서버 전송 실패 | target=" + tgt,
                  traceid);
        std::string err = make_error_response(
            "forward send failed: " + tgt, traceid);
        send_to_client(client_fd, err);
        close(srvfd);
        return;
    }

    log_flow(traceid, "forwarder", tgt, "패킷 전송 완료");

    // 하위 서버 응답 수신
    std::string response;
    if (!recv_packet(srvfd, response))
    {
        log_event("forwarder",
                  "하위 서버 응답 수신 실패 | target=" + tgt,
                  traceid);
        std::string err = make_error_response(
            "forward recv failed: " + tgt, traceid);
        send_to_client(client_fd, err);
        close(srvfd);
        return;
    }

    close(srvfd);

    log_event("forwarder",
              "포워딩 완료 | target=" + tgt +
              " | response=" + response,
              traceid);
    log_flow(traceid, tgt, "forwarder", "응답 수신 완료");
    log_flow(traceid, "forwarder", "client", "응답 전달");

    // 클라이언트로 응답 전달
    send_to_client(client_fd, response);
}

// ------------------------------------------------
// connect_to
// 127.0.0.1:port 에 블로킹 TCP 연결
// ------------------------------------------------
int forwarder::connect_to(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        close(fd);
        return -1;
    }

    return fd;
}

// ------------------------------------------------
// send_packet
// 4byte 빅엔디언 헤더 + body 전송
// ------------------------------------------------
bool forwarder::send_packet(int fd, const std::string& body)
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
        ssize_t n = write(fd, pkt.data() + sent, total - sent);
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
// recv_packet
// 4byte 헤더 수신 → bodylen 파싱 → body 수신
// ------------------------------------------------
bool forwarder::recv_packet(int fd, std::string& out_body)
{
    // 헤더 수신
    uint8_t header[HEADER_SIZE];
    size_t recvd = 0;

    while (recvd < static_cast<size_t>(HEADER_SIZE))
    {
        ssize_t n = read(fd,
                         header + recvd,
                         HEADER_SIZE - recvd);
        if (n <= 0)
            return false;
        recvd += static_cast<size_t>(n);
    }

    uint32_t bodylen = 0;
    bodylen |= static_cast<uint32_t>(header[0]) << 24;
    bodylen |= static_cast<uint32_t>(header[1]) << 16;
    bodylen |= static_cast<uint32_t>(header[2]) << 8;
    bodylen |= static_cast<uint32_t>(header[3]);

    if (bodylen > static_cast<uint32_t>(MAX_BODY_SIZE))
        return false;

    // 바디 수신
    out_body.resize(bodylen);
    size_t total = bodylen;
    recvd = 0;

    while (recvd < total)
    {
        ssize_t n = read(fd,
                         &out_body[recvd],
                         total - recvd);
        if (n <= 0)
            return false;
        recvd += static_cast<size_t>(n);
    }

    return true;
}

// ------------------------------------------------
// send_to_client
// 클라이언트 fd로 4byte 헤더 + body 전송
// ------------------------------------------------
void forwarder::send_to_client(int client_fd, const std::string& body)
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
        ssize_t n = write(client_fd,
                          pkt.data() + sent,
                          total - sent);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            return;
        }
        sent += static_cast<size_t>(n);
    }
}

// ------------------------------------------------
// make_error_response
// 포워딩 실패 시 에러 JSON 생성
// ------------------------------------------------
std::string forwarder::make_error_response(const std::string& reason,
                                            const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"error\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"reason\":\"" + reason + "\"";
    json += "}";
    return json;
}