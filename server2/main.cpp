// ------------------------------------------------
// main.cpp
// FlowScope Server2 진입점
// epoll ET 기반 이벤트 루프 서버
//
// 흐름:
//   main → epollserver2 초기화 → run()
//   epoll ET 이벤트 루프 → router2 → handler → service → fd write
//
// server1과의 차이:
//   server1: accept → 새 스레드 → blocking I/O
//   server2: accept → epoll 등록 → 이벤트 루프에서 처리
// ------------------------------------------------

#include "infra/epollserver2.h"
#include "common/logger.h"

#include <csignal>

// server2 포트
static constexpr int SERVER2_PORT = 9002;

// 시그널 핸들러용 전역 포인터
static epollserver2* g_server = nullptr;

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
// main
// ------------------------------------------------
int main()
{
    log_event("main",
              "FlowScope Server2 시작 | epoll ET 이벤트 루프 서버");

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    epollserver2 server(SERVER2_PORT);
    g_server = &server;

    log_event("main",
              "서버 시작 | port=" + std::to_string(SERVER2_PORT));

    server.run();

    log_event("main", "FlowScope Server2 종료");
    return 0;
}