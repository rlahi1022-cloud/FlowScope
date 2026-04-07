// ------------------------------------------------
// epollserver2.cpp
// server2 epoll ET 이벤트 루프 서버 구현부
//
// 핵심 원칙:
//   1. epoll ET 모드: 이벤트 발생 시 EAGAIN까지 루프로 읽어야 함
//   2. 모든 소켓은 non-blocking
//   3. epoll 스레드가 직접 router2 호출 → service → write
//   4. JobQueue / WorkerPool / EventBus 없음
// ------------------------------------------------

#include "infra/epollserver2.h"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>

// ------------------------------------------------
// 생성자
// ------------------------------------------------
epollserver2::epollserver2(int port)
    : port_(port)
    , listenfd_(-1)
    , epollfd_(-1)
    , running_(false)
{
}

// ------------------------------------------------
// 소멸자
// fd 정리
// ------------------------------------------------
epollserver2::~epollserver2()
{
    if (listenfd_ >= 0) close(listenfd_);
    if (epollfd_  >= 0) close(epollfd_);
}

// ------------------------------------------------
// run
// 서버 메인 루프
// 1. 리슨 소켓 초기화
// 2. epoll 초기화
// 3. 이벤트 루프 (epoll_wait → 이벤트 처리)
// ------------------------------------------------
void epollserver2::run()
{
    if (!init_listen_socket())
        throw std::runtime_error("epollserver2: 리슨 소켓 초기화 실패");

    if (!init_epoll())
        throw std::runtime_error("epollserver2: epoll 초기화 실패");

    // 리슨 소켓을 epoll ET로 등록
    if (!add_to_epoll(listenfd_, EPOLLIN | EPOLLET))
        throw std::runtime_error("epollserver2: listenfd epoll 등록 실패");

    running_.store(true);

    log_event("epollserver2",
              "서버 시작 | port=" + std::to_string(port_) +
              " | 모드=epoll ET non-blocking");

    epoll_event events[EPOLL_MAX_EVENTS2];

    // ------------------------------------------------
    // 이벤트 루프
    // epoll_wait로 이벤트 감지 → 유형에 따라 처리
    // ------------------------------------------------
    while (running_.load())
    {
        int nfds = epoll_wait(epollfd_,
                              events,
                              EPOLL_MAX_EVENTS2,
                              500); // 500ms 타임아웃 (stop 감지용)

        if (nfds < 0)
        {
            if (errno == EINTR) continue; // 시그널 인터럽트 무시
            log_event("epollserver2", "epoll_wait 오류 | 루프 종료");
            break;
        }

        // 감지된 이벤트 순회
        for (int i = 0; i < nfds; ++i)
        {
            int  fd     = events[i].data.fd;
            uint32_t ev = events[i].events;

            // 에러 또는 연결 종료
            if ((ev & EPOLLERR) || (ev & EPOLLHUP))
            {
                log_event("epollserver2",
                          "에러/HUP 이벤트 | fd=" + std::to_string(fd));
                handle_close(fd);
                continue;
            }

            // 새 연결 요청 (listenfd)
            if (fd == listenfd_)
            {
                handle_accept();
                continue;
            }

            // 데이터 수신 가능
            if (ev & EPOLLIN)
            {
                handle_read(fd);
            }
        }
    }

    log_event("epollserver2", "서버 종료");
}

// ------------------------------------------------
// stop
// 이벤트 루프 종료 요청
// ------------------------------------------------
void epollserver2::stop()
{
    running_.store(false);
    log_event("epollserver2", "stop 요청");
}

// ------------------------------------------------
// init_listen_socket
// 리슨 소켓 생성 → SO_REUSEADDR → bind → listen
// non-blocking 설정
// ------------------------------------------------
bool epollserver2::init_listen_socket()
{
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd_ < 0) return false;

    // 포트 재사용 허용
    int optval = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // non-blocking 설정
    if (!set_nonblocking(listenfd_))
    {
        close(listenfd_);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    if (bind(listenfd_,
             reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) < 0)
    {
        log_event("epollserver2",
                  "bind 실패 | port=" + std::to_string(port_));
        return false;
    }

    if (listen(listenfd_, 128) < 0)
    {
        log_event("epollserver2", "listen 실패");
        return false;
    }

    log_event("epollserver2",
              "리슨 소켓 준비 완료 | port=" + std::to_string(port_));
    return true;
}

// ------------------------------------------------
// init_epoll
// epoll 인스턴스 생성
// ------------------------------------------------
bool epollserver2::init_epoll()
{
    epollfd_ = epoll_create1(0);
    if (epollfd_ < 0)
    {
        log_event("epollserver2", "epoll_create1 실패");
        return false;
    }

    log_event("epollserver2", "epoll 인스턴스 생성 완료");
    return true;
}

// ------------------------------------------------
// add_to_epoll
// fd를 epoll에 ET 모드로 등록
// ------------------------------------------------
bool epollserver2::add_to_epoll(int fd, uint32_t events)
{
    epoll_event ev{};
    ev.events  = events;
    ev.data.fd = fd;

    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        log_event("epollserver2",
                  "epoll_ctl ADD 실패 | fd=" + std::to_string(fd));
        return false;
    }

    return true;
}

// ------------------------------------------------
// remove_from_epoll
// fd를 epoll에서 제거
// ------------------------------------------------
void epollserver2::remove_from_epoll(int fd)
{
    epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, nullptr);
}

// ------------------------------------------------
// set_nonblocking
// fd를 O_NONBLOCK으로 설정
// ------------------------------------------------
bool epollserver2::set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return false;

    return true;
}

// ------------------------------------------------
// handle_accept
// 새 클라이언트 연결 처리
// ET 모드: EAGAIN까지 루프로 accept
// ------------------------------------------------
void epollserver2::handle_accept()
{
    while (true)
    {
        sockaddr_in clientaddr{};
        socklen_t   addrlen = sizeof(clientaddr);

        int clientfd = accept(listenfd_,
                              reinterpret_cast<sockaddr*>(&clientaddr),
                              &addrlen);

        if (clientfd < 0)
        {
            // EAGAIN: 더 이상 받을 연결 없음 (ET 정상 종료)
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            log_event("epollserver2", "accept 오류");
            break;
        }

        // non-blocking 설정
        if (!set_nonblocking(clientfd))
        {
            log_event("epollserver2",
                      "clientfd non-blocking 설정 실패 | fd=" +
                      std::to_string(clientfd));
            close(clientfd);
            continue;
        }

        // epoll ET 등록 (읽기 이벤트)
        if (!add_to_epoll(clientfd, EPOLLIN | EPOLLET))
        {
            close(clientfd);
            continue;
        }

        // 클라이언트별 수신 버퍼 초기화
        recvbufs_[clientfd] = "";

        log_event("epollserver2",
                  "클라이언트 연결 | fd=" + std::to_string(clientfd) +
                  " | ip=" + inet_ntoa(clientaddr.sin_addr));
    }
}

// ------------------------------------------------
// handle_read
// 클라이언트 데이터 수신
// ET 모드: EAGAIN이 나올 때까지 루프로 전부 읽어야 함
// 데이터를 recvbuf에 누적 후 process_buffer 호출
// ------------------------------------------------
void epollserver2::handle_read(int fd)
{
    char buf[4096];

    while (true)
    {
        ssize_t n = read(fd, buf, sizeof(buf));

        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // ET 모드: EAGAIN = 이 시점에서 더 읽을 데이터 없음
                // 버퍼에 누적된 데이터로 패킷 처리 시도
                process_buffer(fd);
                break;
            }
            // 실제 오류
            log_event("epollserver2",
                      "read 오류 | fd=" + std::to_string(fd) +
                      " | err=" + strerror(errno));
            handle_close(fd);
            return;
        }

        if (n == 0)
        {
            // 클라이언트 정상 종료
            log_event("epollserver2",
                      "클라이언트 연결 종료 | fd=" + std::to_string(fd));
            handle_close(fd);
            return;
        }

        // 수신 데이터를 버퍼에 누적
        recvbufs_[fd].append(buf, static_cast<size_t>(n));

        log_event("epollserver2",
                  "데이터 수신 | fd=" + std::to_string(fd) +
                  " | bytes=" + std::to_string(n) +
                  " | bufsize=" + std::to_string(recvbufs_[fd].size()));
    }
}

// ------------------------------------------------
// handle_close
// 클라이언트 연결 종료 처리
// epoll 해제 → 버퍼 제거 → fd 닫기
// ------------------------------------------------
void epollserver2::handle_close(int fd)
{
    log_event("epollserver2",
              "연결 해제 처리 | fd=" + std::to_string(fd));

    remove_from_epoll(fd);
    recvbufs_.erase(fd);
    close(fd);
}

// ------------------------------------------------
// process_buffer
// 수신 버퍼에서 완성된 패킷을 추출해 router2로 전달
//
// 패킷 구조: [4byte 빅엔디언 bodylen][body]
// 버퍼에 완성된 패킷이 여러 개 있을 수 있으므로 루프 처리
// ------------------------------------------------
void epollserver2::process_buffer(int fd)
{
    std::string& buf = recvbufs_[fd];

    // 완성된 패킷이 있는 동안 반복 처리
    while (true)
    {
        // 헤더 4바이트가 아직 안 쌓임
        if (buf.size() < static_cast<size_t>(HEADER_SIZE))
            break;

        // 헤더에서 bodylen 파싱 (빅엔디언)
        uint32_t bodylen = 0;
        bodylen |= static_cast<uint32_t>(
            static_cast<uint8_t>(buf[0])) << 24;
        bodylen |= static_cast<uint32_t>(
            static_cast<uint8_t>(buf[1])) << 16;
        bodylen |= static_cast<uint32_t>(
            static_cast<uint8_t>(buf[2])) << 8;
        bodylen |= static_cast<uint32_t>(
            static_cast<uint8_t>(buf[3]));

        // 비정상 크기 → 연결 종료
        if (bodylen > static_cast<uint32_t>(MAX_BODY_SIZE))
        {
            log_event("epollserver2",
                      "bodylen 초과 | fd=" + std::to_string(fd) +
                      " | bodylen=" + std::to_string(bodylen));
            handle_close(fd);
            return;
        }

        // 바디가 아직 다 안 쌓임
        size_t total = static_cast<size_t>(HEADER_SIZE) + bodylen;
        if (buf.size() < total) break;

        // 완성된 패킷 추출
        std::string jsonbody = buf.substr(HEADER_SIZE, bodylen);

        // 처리한 만큼 버퍼에서 제거
        buf.erase(0, total);

        log_event("epollserver2",
                  "패킷 완성 | fd=" + std::to_string(fd) +
                  " | bodylen=" + std::to_string(bodylen));
        log_flow("?", "epollserver2", "router2",
                 "fd=" + std::to_string(fd));

        // router2로 전달 (traceid는 router2에서 발급)
        router_.route(fd, jsonbody);
    }
}