// ------------------------------------------------
// main.cpp
// FlowScope Server3 진입점
// EventBus 기반 이벤트 루프 서버
//
// 흐름:
//   main
//   → eventbus3 싱글턴 생성
//   → handler들을 "request" 토픽에 subscribe
//   → main 자신을 "response" 토픽에 subscribe
//   → epollserver3 초기화 및 run()
//   → epoll ET → router3 → eventbus3.publish("request")
//   → handler → service → eventbus3.publish("response")
//   → main의 response 구독자 → fd로 write
//
// server2와의 차이:
//   server2: handler가 직접 fd에 write
//   server3: handler → eventbus → main이 fd에 write
// ------------------------------------------------

#include "infra/epollserver3.h"
#include "eventbus/eventbus3.h"
#include "service/handler/echohandler3.h"
#include "service/handler/uieventhandler3.h"
#include "common/logger.h"
#include "common/packet.h"

#include <csignal>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

// server3 포트
static constexpr int SERVER3_PORT = 9003;

// 시그널 핸들러용 전역 포인터
static epollserver3* g_server = nullptr;

// ------------------------------------------------
// signal_handler
// SIGINT/SIGTERM 수신 시 서버 정상 종료
// ------------------------------------------------
static void signal_handler(int signum)
{
    log_event("main",
              "종료 시그널 수신 | sig=" + std::to_string(signum));
    if (g_server) g_server->stop();
}

// ------------------------------------------------
// send_response_to_client
// "response" 토픽 구독자 콜백
// payload 형식: "fd:traceid:json"
// 클라이언트 fd로 4byte 헤더 + body 전송
// ------------------------------------------------
static void send_response_to_client(const std::string& payload)
{
    log_event("main", "response 토픽 수신 | 클라이언트로 전송");

    // payload 파싱: "fd:traceid:json"
    size_t pos1 = payload.find(':');
    if (pos1 == std::string::npos)
    {
        log_event("main", "response payload 파싱 실패 (첫 번째 ':' 없음)");
        return;
    }

    int fd;
    try {
        fd = std::stoi(payload.substr(0, pos1));
    } catch (...) {
        log_event("main", "response payload fd 파싱 실패");
        return;
    }

    size_t pos2 = payload.find(':', pos1 + 1);
    if (pos2 == std::string::npos)
    {
        log_event("main", "response payload 파싱 실패 (두 번째 ':' 없음)");
        return;
    }

    std::string traceid = payload.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string json_response = payload.substr(pos2 + 1);

    log_event("main",
              "응답 전송 | fd=" + std::to_string(fd),
              "json=" + json_response.substr(0, 100) + "...", traceid);
    log_flow(traceid, "main", "client(fd)",
             "fd=" + std::to_string(fd) + " | write");

    // 4byte 빅엔디언 헤더 구성
    uint32_t bodylen = static_cast<uint32_t>(json_response.size());
    uint8_t header[HEADER_SIZE];
    header[0] = (bodylen >> 24) & 0xFF;
    header[1] = (bodylen >> 16) & 0xFF;
    header[2] = (bodylen >>  8) & 0xFF;
    header[3] = (bodylen      ) & 0xFF;

    std::string pkt;
    pkt.append(reinterpret_cast<char*>(header), HEADER_SIZE);
    pkt.append(json_response);

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
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 버퍼 가득 참: 잠시 대기 후 재시도
                log_event("main",
                          "write 대기 (EAGAIN) | fd=" + std::to_string(fd));
                continue;
            }
            log_event("main",
                      "write 실패 | fd=" + std::to_string(fd) +
                      " | err=" + strerror(errno),
                      "", traceid);
            return;
        }
        sent += static_cast<size_t>(n);
    }

    log_event("main",
              "write 완료 | fd=" + std::to_string(fd) +
              " | bytes=" + std::to_string(total),
              "", traceid);
}

// ------------------------------------------------
// main
// ------------------------------------------------
int main()
{
    log_event("main",
              "FlowScope Server3 시작 | EventBus 기반 epoll ET 이벤트 루프 서버");

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // EventBus 싱글턴 참조
    eventbus3& bus = eventbus3::instance();

    // ------------------------------------------------
    // Handler 생성 및 "request" 토픽 구독
    // ------------------------------------------------
    auto echo_handler = std::make_shared<echohandler3>();
    auto uievent_handler = std::make_shared<uieventhandler3>();

    // echo 핸들러: "request" 토픽에 구독
    bus.subscribe("request", [echo_handler](const std::string& payload)
    {
        // payload에서 proto 추출하여 echo 관련인지 판단
        // 간단한 구현: 모든 request를 echo_handler가 처리
        // 실제로는 proto 기반 분기 필요
        size_t pos = payload.find(":\"cmd\"");
        if (pos != std::string::npos)
        {
            std::string cmd_part = payload.substr(pos);
            if (cmd_part.find("echo") != std::string::npos ||
                cmd_part.find("ping") != std::string::npos)
            {
                echo_handler->handle(payload);
            }
        }
    });

    // uievent 핸들러: "request" 토픽에 구독
    bus.subscribe("request", [uievent_handler](const std::string& payload)
    {
        // payload에서 proto 추출하여 ui_* 관련인지 판단
        size_t pos = payload.find(":\"cmd\"");
        if (pos != std::string::npos)
        {
            std::string cmd_part = payload.substr(pos);
            if (cmd_part.find("ui_") != std::string::npos)
            {
                uievent_handler->handle(payload);
            }
        }
    });

    // ------------------------------------------------
    // "response" 토픽 구독
    // ------------------------------------------------
    bus.subscribe("response", send_response_to_client);

    log_event("main",
              "EventBus 구독 완료 | request, response 토픽 등록");

    // ------------------------------------------------
    // Server 시작
    // ------------------------------------------------
    epollserver3 server(SERVER3_PORT);
    g_server = &server;

    log_event("main",
              "서버 시작 | port=" + std::to_string(SERVER3_PORT));

    try {
        server.run();
    } catch (const std::exception& e) {
        log_event("main", "서버 실행 중 예외 발생 | " + std::string(e.what()));
    }

    log_event("main", "FlowScope Server3 종료");
    return 0;
}
