#pragma once

// ------------------------------------------------
// epollserver.h
// epoll 기반 non-blocking TCP 서버
// Edge Triggered 모드 사용
// epoll 스레드는 절대 blocking 작업을 하지 않는다
//
// 흐름:
//   accept → EPOLLIN 등록 →
//   데이터 수신 → router.route() 호출
// ------------------------------------------------

#include "router/router.h"
#include "packet.h"
#include <string>
#include <unordered_map>
#include <vector>

// ------------------------------------------------
// epollserver
// non-blocking epoll 기반 TCP 서버
// ------------------------------------------------
class epollserver
{
public:
    explicit epollserver(int port);
    ~epollserver();
    void run();
    void stop();

private:
    bool init_listen_socket();
    bool set_nonblocking(int fd);
    bool add_epoll_fd(int fd);
    void handle_accept();
    void handle_read(int fd);
    void close_client(int fd);
    void parse_packet(int fd);

    int port_;
    int listenfd_;
    int epollfd_;
    bool running_;

    std::unordered_map<int, std::vector<char>> recvbuffers_;
    router router_;
};