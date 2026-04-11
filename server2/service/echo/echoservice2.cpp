// ------------------------------------------------
// echoservice2.cpp
// server2 echo 비즈니스 로직 구현부
//
// server2 핵심 특징:
//   EventBus 없음 → 처리 완료 후 fd로 직접 write
//   epoll 스레드에서 즉시 처리 (블로킹 없음)
// ------------------------------------------------

#include "service/echo/echoservice2.h"
#include "common/logger.h"
#include "common/packet.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

// ------------------------------------------------
// process
// proto에 따라 echo/ping 응답을 생성하고
// fd로 직접 전송한다
// ------------------------------------------------
void echoservice2::process(const packet2& pkt)
{
    log_event("echoservice2",
              "처리 시작 | proto=" + protocol_to_string(pkt.ctx.proto),
              "", pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "echohandler2", "echoservice2",
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

        default:
            log_event("echoservice2",
                      "알 수 없는 프로토콜 - 처리 중단",
                      "", pkt.ctx.traceid);
            return;
    }

    log_event("echoservice2",
              "처리 완료 - fd로 직접 write | fd=" + std::to_string(pkt.fd),
              "response=" + response, pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "echoservice2", "client(fd)",
             "fd=" + std::to_string(pkt.fd) + " | 직접 write");

    // EventBus 없이 fd로 직접 전송 (server2의 핵심 구조)
    send_response(pkt.fd, response);
}

// ------------------------------------------------
// build_echo_response
// echo 응답 JSON 생성
// ------------------------------------------------
std::string echoservice2::build_echo_response(const std::string& body,
                                               const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"echo_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server2\",";
    json += "\"data\":" + body;
    json += "}";
    return json;
}

// ------------------------------------------------
// build_ping_response
// ping 응답 JSON 생성
// ------------------------------------------------
std::string echoservice2::build_ping_response(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"pong\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server2\",";
    json += "\"data\":\"pong\"";
    json += "}";
    return json;
}

// ------------------------------------------------
// send_response
// 4byte 빅엔디언 헤더 + body를 fd로 전송
// epoll ET 모드이므로 전체 데이터 루프 전송
// ------------------------------------------------
void echoservice2::send_response(int fd, const std::string& body)
{
    uint32_t bodylen = static_cast<uint32_t>(body.size());

    // 빅엔디언 헤더 구성
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

    // 전체 전송 보장 루프
    while (sent < total)
    {
        ssize_t n = write(fd,
                          pkt.data() + sent,
                          total - sent);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            log_event("echoservice2",
                      "write 실패 | fd=" + std::to_string(fd) +
                      " | err=" + strerror(errno));
            return;
        }
        sent += static_cast<size_t>(n);
    }

    log_event("echoservice2",
              "write 완료 | fd=" + std::to_string(fd) +
              " | bytes=" + std::to_string(total));
}