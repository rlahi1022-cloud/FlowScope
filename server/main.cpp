// ------------------------------------------------
// main.cpp
// FlowScope 서버 진입점
// Stage 1: 내부 프로토콜 기반 흐름 구현
// Stage 2: traceid 기반 flow 시각화 포함
//
// 시작 흐름:
//   main → eventbus 구독 등록 →
//   epollserver 초기화 → run()
// ------------------------------------------------

#include "infra/epollserver.h"
#include "eventbus/eventbus.h"
#include "logger.h"
#include "packet.h"

#include <csignal>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

// ------------------------------------------------
// 서버 포트
// ------------------------------------------------
static constexpr int SERVER_PORT = 9000;

// ------------------------------------------------
// 전역 서버 포인터 (시그널 핸들러용)
// ------------------------------------------------
static epollserver* g_server = nullptr;

// ------------------------------------------------
// signal_handler
// SIGINT (Ctrl+C) 수신 시 서버를 정상 종료한다
// ------------------------------------------------
static void signal_handler(int signum)
{
    log_event("main", "종료 시그널 수신 | sig=" + std::to_string(signum));
    if (g_server)
        g_server->stop();
}

// ------------------------------------------------
// send_response
// fd로 4byte 빅엔디언 헤더 + JSON body 전송
// epoll ET 모드이므로 전체 데이터를 루프로 전송한다
// ------------------------------------------------
static void send_response(int fd, const std::string& body)
{
    uint32_t bodylen = static_cast<uint32_t>(body.size());

    // 빅엔디언 헤더 4바이트 구성
    uint8_t header[HEADER_SIZE];
    header[0] = (bodylen >> 24) & 0xFF;
    header[1] = (bodylen >> 16) & 0xFF;
    header[2] = (bodylen >>  8) & 0xFF;
    header[3] = (bodylen      ) & 0xFF;

    // 헤더 + 바디 합쳐서 전송 버퍼 구성
    std::string packet;
    packet.append(reinterpret_cast<char*>(header), HEADER_SIZE);
    packet.append(body);

    // 전체 전송 보장 루프
    size_t total = packet.size();
    size_t sent  = 0;
    while (sent < total)
    {
        ssize_t n = write(fd,
                          packet.data() + sent,
                          total - sent);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            log_event("main", "send_response write 실패 | fd=" + std::to_string(fd));
            return;
        }
        sent += static_cast<size_t>(n);
    }

    log_event("main",
              "응답 전송 완료 | fd=" + std::to_string(fd) +
              " | size=" + std::to_string(total));
}

// ------------------------------------------------
// register_eventbus_handlers
// eventbus 구독자 등록
// "response" topic: echoservice가 publish한 결과를 수신
// ------------------------------------------------
static void register_eventbus_handlers()
{
    // "response" topic 구독
    // payload 형식: "fd:traceid:json_body"
    eventbus::instance().subscribe("response",
        [](const std::string& payload)
        {
            // payload 파싱: fd:traceid:json_body
            size_t pos1 = payload.find(':');
            size_t pos2 = payload.find(':', pos1 + 1);

            if (pos1 == std::string::npos || pos2 == std::string::npos)
            {
                log_event("main", "response payload 파싱 실패");
                return;
            }

            std::string fdstr   = payload.substr(0, pos1);
            std::string traceid = payload.substr(pos1 + 1, pos2 - pos1 - 1);
            std::string body    = payload.substr(pos2 + 1);

            int fd = std::stoi(fdstr);

            log_event("main",
                      "response 수신 | fd=" + fdstr +
                      " | body=" + body,
                      traceid);
            log_flow(traceid, "eventbus", "main(response handler)",
                     "fd=" + fdstr);

            // 클라이언트로 실제 응답 전송
            send_response(fd, body);

            log_event("main",
                      "★ 처리 완료 | fd=" + fdstr +
                      " | 응답=" + body,
                      traceid);
        });

    log_event("main", "eventbus 구독 등록 완료 | topic=response");
}

// ------------------------------------------------
// internal_test
// Stage 1: 클라이언트 없이 내부 요청으로 흐름 검증
// router.route()를 직접 호출하여 전체 흐름을 테스트
// ------------------------------------------------
static void internal_test()
{
    log_event("main", "========== Stage 1 내부 흐름 테스트 시작 ==========");

    // 라우터 직접 생성 후 내부 요청 전송
    router testrouterobj;

    // 테스트 케이스 1: echo 요청
    log_event("main", "테스트 1: echo 요청");
    testrouterobj.route(0, R"({"cmd":"echo","data":{"msg":"hello flowscope"}})");

    // 테스트 케이스 2: ping 요청
    log_event("main", "테스트 2: ping 요청");
    testrouterobj.route(0, R"({"cmd":"ping"})");

    // 테스트 케이스 3: unknown 요청
    log_event("main", "테스트 3: unknown 요청");
    testrouterobj.route(0, R"({"cmd":"unknown_cmd","data":{}})");

    log_event("main", "========== Stage 1 내부 흐름 테스트 완료 ==========");
}

// ------------------------------------------------
// main
// 1. eventbus 구독 등록
// 2. Stage 1 내부 흐름 테스트
// 3. 실제 epoll 서버 시작
// ------------------------------------------------
int main()
{
    log_event("main", "FlowScope 서버 시작");

    // eventbus 구독자 등록
    register_eventbus_handlers();

    // Stage 1: 내부 프로토콜 흐름 검증 (클라이언트 없이)
    internal_test();

    // 시그널 핸들러 등록 (정상 종료)
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // epoll 서버 시작
    epollserver server(SERVER_PORT);
    g_server = &server;

    log_event("main",
              "epoll 서버 시작 | port=" + std::to_string(SERVER_PORT));
    server.run();

    log_event("main", "FlowScope 서버 종료");
    return 0;
}