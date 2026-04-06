// ------------------------------------------------
// tcpserver.cpp
// Thread-per-Connection TCP 서버 구현부
// ------------------------------------------------

#include "tcpserver.h"
#include "session.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <stdexcept>

tcpserver::tcpserver(int port)
    : port_(port)
    , listenfd_(-1)
    , running_(false)
{
}

tcpserver::~tcpserver()
{
    if (listenfd_ >= 0) close(listenfd_);
}

void tcpserver::run()
{
    if (!init_listen_socket())
        throw std::runtime_error("리슨 소켓 초기화 실패");

    running_ = true;
    log_event("tcpserver",
              "서버 시작 | port=" + std::to_string(port_));

    while (running_)
    {
        sockaddr_in clientaddr{};
        socklen_t   addrlen = sizeof(clientaddr);

        int clientfd = accept(listenfd_,
                              reinterpret_cast<sockaddr*>(&clientaddr),
                              &addrlen);
        if (clientfd < 0)
        {
            if (!running_) break;
            log_event("tcpserver", "accept 오류");
            continue;
        }

        log_event("tcpserver",
                  "클라이언트 연결 | fd=" + std::to_string(clientfd) +
                  " | ip=" + inet_ntoa(clientaddr.sin_addr));

        // 클라이언트마다 새 스레드 생성
        std::thread([clientfd]() {
            session s(clientfd);
            s.run();
        }).detach();
    }

    log_event("tcpserver", "서버 종료");
}

void tcpserver::stop()
{
    running_ = false;
    if (listenfd_ >= 0) close(listenfd_);
}

bool tcpserver::init_listen_socket()
{
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd_ < 0) return false;

    int optval = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    if (bind(listenfd_,
             reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) < 0)
    {
        log_event("tcpserver",
                  "bind 실패 | port=" + std::to_string(port_));
        return false;
    }

    if (listen(listenfd_, 128) < 0)
    {
        log_event("tcpserver", "listen 실패");
        return false;
    }

    log_event("tcpserver",
              "리슨 소켓 준비 완료 | port=" + std::to_string(port_));
    return true;
}