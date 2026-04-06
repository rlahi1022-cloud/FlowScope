// ------------------------------------------------
// main.cpp
// FlowScope Server1 진입점
// Thread-per-Connection 구조
// ------------------------------------------------

#include "tcpserver.h"
#include "logger.h"
#include <csignal>

static constexpr int SERVER_PORT = 9001;

static tcpserver* g_server = nullptr;

static void signal_handler(int signum)
{
    log_event("main",
              "종료 시그널 수신 | sig=" + std::to_string(signum));
    if (g_server) g_server->stop();
}

int main()
{
    log_event("main", "FlowScope Server1 시작 | Thread-per-Connection");

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    tcpserver server(SERVER_PORT);
    g_server = &server;

    log_event("main",
              "서버 시작 | port=" + std::to_string(SERVER_PORT));
    server.run();

    log_event("main", "FlowScope Server1 종료");
    return 0;
}