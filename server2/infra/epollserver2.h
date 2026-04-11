#pragma once

// ------------------------------------------------
// epollserver2.h
// server2 epoll ET 이벤트 루프 서버 헤더
//
// 흐름:
//   epoll_wait → 이벤트 분기
//     → accept (새 연결)
//     → read (데이터 수신) → process_buffer → router2
//     → close (연결 종료)
// ------------------------------------------------

#include "router/router2.h"
#include "common/logger.h"
#include "common/packet.h"

#include <string>
#include <unordered_map>
#include <atomic>

// epoll 최대 동시 이벤트 수
static constexpr int EPOLL_MAX_EVENTS2 = 64;

// ------------------------------------------------
// epollserver2
// epoll ET + non-blocking 기반 이벤트 루프 서버
// ------------------------------------------------
class epollserver2
{
public:
    explicit epollserver2(int port);
    ~epollserver2();

    // 서버 메인 루프 시작
    void run();

    // 이벤트 루프 종료 요청
    void stop();

private:
    // 초기화
    bool init_listen_socket();
    bool init_epoll();

    // epoll 유틸
    bool add_to_epoll(int fd, uint32_t events);
    void remove_from_epoll(int fd);
    bool set_nonblocking(int fd);

    // 이벤트 처리
    void handle_accept();
    void handle_read(int fd);
    void handle_close(int fd);

    // 수신 버퍼에서 완성된 패킷 추출 → router2 전달
    void process_buffer(int fd);

    // ------------------------------------------------
    // 멤버 변수
    // ------------------------------------------------
    int  port_;
    int  listenfd_;
    int  epollfd_;
    std::atomic<bool> running_;

    // fd별 수신 버퍼 (TCP 스트림 누적)
    std::unordered_map<int, std::string> recvbufs_;

    // 라우터 (패킷을 핸들러로 분기)
    router2 router_;
};
