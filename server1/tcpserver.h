#pragma once

// ------------------------------------------------
// tcpserver.h
// Thread-per-Connection TCP 서버
//
// 흐름:
//   accept → 새 스레드 생성 → session::run()
//   메인 스레드는 accept 루프만 담당
// ------------------------------------------------

#include "logger.h"
#include <stdexcept>
#include <string>

class tcpserver
{
public:
    explicit tcpserver(int port);
    ~tcpserver();

    void run();
    void stop();

private:
    bool init_listen_socket();

    int  port_;
    int  listenfd_;
    bool running_;
};