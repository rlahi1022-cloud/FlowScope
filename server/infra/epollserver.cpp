// ------------------------------------------------
// epollserver.cpp
// epoll 기반 non-blocking TCP 서버 구현부
// Edge Triggered 모드: EAGAIN까지 반복 읽기 필수
// epoll 스레드는 절대 blocking 작업 수행 금지
// ------------------------------------------------

#include "infra/epollserver.h"
#include "common/logger.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <algorithm>

// epoll_wait 최대 이벤트 수
static constexpr int MAX_EVENTS = 64;

// ------------------------------------------------
// 생성자
// ------------------------------------------------
epollserver::epollserver(int port)
    : port_(port)
    , listenfd_(-1)
    , epollfd_(-1)
    , running_(false)
{
}

// ------------------------------------------------
// 소멸자: fd 자원 해제
// ------------------------------------------------
epollserver::~epollserver()
{
    if (listenfd_ >= 0) close(listenfd_);
    if (epollfd_  >= 0) close(epollfd_);
}

// ------------------------------------------------
// run
// 1. 리슨 소켓 초기화
// 2. epoll 인스턴스 생성
// 3. 이벤트 루프 시작
// ------------------------------------------------
void epollserver::run()
{
    // 리슨 소켓 초기화
    if (!init_listen_socket())
        throw std::runtime_error("리슨 소켓 초기화 실패");

    // epoll 인스턴스 생성
    epollfd_ = epoll_create1(0);
    if (epollfd_ < 0)
        throw std::runtime_error("epoll_create1 실패");

    // 리슨 소켓을 epoll에 등록
    if (!add_epoll_fd(listenfd_))
        throw std::runtime_error("리슨 소켓 epoll 등록 실패");

    running_ = true;
    log_event("epollserver",
              "서버 시작 | port=" + std::to_string(port_));

    // ------------------------------------------------
    // epoll 이벤트 루프
    // ------------------------------------------------
    epoll_event events[MAX_EVENTS];

    while (running_)
    {
        // 이벤트 대기 (timeout=-1: 무한 대기)
        int nfds = epoll_wait(epollfd_, events, MAX_EVENTS, -1);

        if (nfds < 0)
        {
            if (errno == EINTR)
                continue; // 시그널 인터럽트: 재시도
            log_event("epollserver", "epoll_wait 오류");
            break;
        }

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            if (fd == listenfd_)
            {
                // 새 연결 수락
                handle_accept();
            }
            else if (events[i].events & EPOLLIN)
            {
                // 데이터 수신
                handle_read(fd);
            }
            else if (events[i].events & (EPOLLERR | EPOLLHUP))
            {
                // 오류 또는 연결 종료
                log_event("epollserver",
                          "EPOLLERR/EPOLLHUP | fd=" + std::to_string(fd));
                close_client(fd);
            }
        }
    }

    log_event("epollserver", "서버 종료");
}

// ------------------------------------------------
// stop
// 이벤트 루프 종료 플래그 설정
// ------------------------------------------------
void epollserver::stop()
{
    running_ = false;
}

// ------------------------------------------------
// init_listen_socket
// 소켓 생성 → SO_REUSEADDR → bind → listen
// ------------------------------------------------
bool epollserver::init_listen_socket()
{
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd_ < 0)
    {
        log_event("epollserver", "socket() 실패");
        return false;
    }

    // SO_REUSEADDR: 서버 재시작 시 포트 즉시 재사용
    int optval = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // non-blocking 설정
    if (!set_nonblocking(listenfd_))
        return false;

    // bind
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    if (bind(listenfd_, reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) < 0)
    {
        log_event("epollserver",
                  "bind 실패 | port=" + std::to_string(port_));
        return false;
    }

    // listen (백로그 128)
    if (listen(listenfd_, 128) < 0)
    {
        log_event("epollserver", "listen 실패");
        return false;
    }

    log_event("epollserver",
              "리슨 소켓 준비 완료 | port=" + std::to_string(port_));
    return true;
}

// ------------------------------------------------
// set_nonblocking
// fcntl로 O_NONBLOCK 플래그 설정
// ------------------------------------------------
bool epollserver::set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

// ------------------------------------------------
// add_epoll_fd
// EPOLLIN | EPOLLET (Edge Triggered) 로 등록
// ------------------------------------------------
bool epollserver::add_epoll_fd(int fd)
{
    epoll_event ev{};
    ev.events  = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    return epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) == 0;
}

// ------------------------------------------------
// handle_accept
// Edge Triggered: EAGAIN까지 반복 accept
// ------------------------------------------------
void epollserver::handle_accept()
{
    while (true)
    {
        sockaddr_in clientaddr{};
        socklen_t addrlen = sizeof(clientaddr);

        int clientfd = accept(listenfd_,
                              reinterpret_cast<sockaddr*>(&clientaddr),
                              &addrlen);
        if (clientfd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; // 더 이상 연결 없음
            log_event("epollserver", "accept 오류");
            break;
        }

        // 클라이언트 소켓 non-blocking 설정
        set_nonblocking(clientfd);

        // epoll에 등록
        add_epoll_fd(clientfd);

        // 수신 버퍼 초기화
        recvbuffers_[clientfd] = {};

        log_event("epollserver",
                  "클라이언트 연결 | fd=" + std::to_string(clientfd) +
                  " | ip=" + inet_ntoa(clientaddr.sin_addr));
    }
}

// ------------------------------------------------
// handle_read
// Edge Triggered: EAGAIN까지 반복 read
// 읽은 데이터를 recvbuffer에 누적 후 패킷 파싱
// ------------------------------------------------
void epollserver::handle_read(int fd)
{
    auto& buf = recvbuffers_[fd];
    char tmp[4096];

    while (true)
    {
        ssize_t n = read(fd, tmp, sizeof(tmp));

        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; // 더 읽을 데이터 없음
            log_event("epollserver",
                      "read 오류 | fd=" + std::to_string(fd));
            close_client(fd);
            return;
        }
        else if (n == 0)
        {
            // 클라이언트 정상 종료
            log_event("epollserver",
                      "클라이언트 연결 종료 | fd=" + std::to_string(fd));
            close_client(fd);
            return;
        }

        // 수신 버퍼에 누적
        buf.insert(buf.end(), tmp, tmp + n);
    }

    // 누적 버퍼에서 완성된 패킷 파싱
    parse_packet(fd);
}

// ------------------------------------------------
// close_client
// 클라이언트 fd epoll 제거 + 버퍼 정리 + fd 닫기
// ------------------------------------------------
void epollserver::close_client(int fd)
{
    epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, nullptr);
    recvbuffers_.erase(fd);
    close(fd);
}

// ------------------------------------------------
// parse_packet
// 4바이트 빅엔디언 길이 헤더 + JSON 바디 파싱
// 버퍼에 완성된 패킷이 있으면 router로 전달
// 불완전한 패킷은 버퍼에 남겨두고 다음 수신 대기
// ------------------------------------------------
void epollserver::parse_packet(int fd)
{
    auto& buf = recvbuffers_[fd];

    while (true)
    {
        // 헤더 4바이트 미만이면 대기
        if (buf.size() < static_cast<size_t>(HEADER_SIZE))
            break;

        // 빅엔디언 4바이트 → 바디 길이 파싱
        uint32_t bodylen = 0;
        bodylen |= static_cast<uint32_t>(static_cast<uint8_t>(buf[0])) << 24;
        bodylen |= static_cast<uint32_t>(static_cast<uint8_t>(buf[1])) << 16;
        bodylen |= static_cast<uint32_t>(static_cast<uint8_t>(buf[2])) << 8;
        bodylen |= static_cast<uint32_t>(static_cast<uint8_t>(buf[3]));

        // 바디 크기 상한 검사
        if (bodylen > static_cast<uint32_t>(MAX_BODY_SIZE))
        {
            log_event("epollserver",
                      "패킷 크기 초과 | fd=" + std::to_string(fd) +
                      " | size=" + std::to_string(bodylen));
            close_client(fd);
            return;
        }

        // 헤더 + 바디 전체 수신 완료 확인
        size_t total = static_cast<size_t>(HEADER_SIZE) + bodylen;
        if (buf.size() < total)
            break; // 아직 바디 미완성 → 대기

        // 바디 추출
        std::string jsonbody(buf.begin() + HEADER_SIZE,
                             buf.begin() + HEADER_SIZE + bodylen);

        // 처리한 패킷을 버퍼에서 제거
        buf.erase(buf.begin(), buf.begin() + total);

        log_event("epollserver",
                  "패킷 수신 완료 | fd=" + std::to_string(fd) +
                  " | body=" + jsonbody);

        // router로 전달 (epoll 스레드는 경량 처리만)
        router_.route(fd, jsonbody);
    }
}