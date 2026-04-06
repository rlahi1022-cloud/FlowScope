// ------------------------------------------------
// epollserver.cpp
// epoll 기반 non-blocking TCP 서버 구현부
// Edge Triggered 모드: EAGAIN까지 반복 읽기 필수
// epoll 스레드는 절대 blocking 작업 수행 금지
// ------------------------------------------------

#include "infra/epollserver.h"
#include "logger.h"
#include "packet.h"

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

static constexpr int MAX_EVENTS = 64;

epollserver::epollserver(int port)
    : port_(port)
    , listenfd_(-1)
    , epollfd_(-1)
    , running_(false)
{
}

epollserver::~epollserver()
{
    if (listenfd_ >= 0) close(listenfd_);
    if (epollfd_  >= 0) close(epollfd_);
}

void epollserver::run()
{
    if (!init_listen_socket())
        throw std::runtime_error("리슨 소켓 초기화 실패");

    epollfd_ = epoll_create1(0);
    if (epollfd_ < 0)
        throw std::runtime_error("epoll_create1 실패");

    if (!add_epoll_fd(listenfd_))
        throw std::runtime_error("리슨 소켓 epoll 등록 실패");

    running_ = true;
    log_event("epollserver", "서버 시작 | port=" + std::to_string(port_));

    epoll_event events[MAX_EVENTS];

    while (running_)
    {
        int nfds = epoll_wait(epollfd_, events, MAX_EVENTS, -1);

        if (nfds < 0)
        {
            if (errno == EINTR) continue;
            log_event("epollserver", "epoll_wait 오류");
            break;
        }

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            if (fd == listenfd_)
                handle_accept();
            else if (events[i].events & EPOLLIN)
                handle_read(fd);
            else if (events[i].events & (EPOLLERR | EPOLLHUP))
            {
                log_event("epollserver", "EPOLLERR/EPOLLHUP | fd=" + std::to_string(fd));
                close_client(fd);
            }
        }
    }

    log_event("epollserver", "서버 종료");
}

void epollserver::stop()
{
    running_ = false;
}

bool epollserver::init_listen_socket()
{
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd_ < 0)
    {
        log_event("epollserver", "socket() 실패");
        return false;
    }

    int optval = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (!set_nonblocking(listenfd_))
        return false;

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    if (bind(listenfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        log_event("epollserver", "bind 실패 | port=" + std::to_string(port_));
        return false;
    }

    if (listen(listenfd_, 128) < 0)
    {
        log_event("epollserver", "listen 실패");
        return false;
    }

    log_event("epollserver", "리슨 소켓 준비 완료 | port=" + std::to_string(port_));
    return true;
}

bool epollserver::set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

bool epollserver::add_epoll_fd(int fd)
{
    epoll_event ev{};
    ev.events  = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    return epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) == 0;
}

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
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            log_event("epollserver", "accept 오류");
            break;
        }

        set_nonblocking(clientfd);
        add_epoll_fd(clientfd);
        recvbuffers_[clientfd] = {};

        log_event("epollserver",
                  "클라이언트 연결 | fd=" + std::to_string(clientfd) +
                  " | ip=" + inet_ntoa(clientaddr.sin_addr));
    }
}

void epollserver::handle_read(int fd)
{
    auto& buf = recvbuffers_[fd];
    char tmp[4096];

    while (true)
    {
        ssize_t n = read(fd, tmp, sizeof(tmp));

        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            log_event("epollserver", "read 오류 | fd=" + std::to_string(fd));
            close_client(fd);
            return;
        }
        else if (n == 0)
        {
            log_event("epollserver", "클라이언트 연결 종료 | fd=" + std::to_string(fd));
            close_client(fd);
            return;
        }

        buf.insert(buf.end(), tmp, tmp + n);
    }

    parse_packet(fd);
}

void epollserver::close_client(int fd)
{
    epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, nullptr);
    recvbuffers_.erase(fd);
    close(fd);
}

void epollserver::parse_packet(int fd)
{
    auto& buf = recvbuffers_[fd];

    while (true)
    {
        // 헤더 4바이트 미만이면 대기
        if (buf.size() < static_cast<size_t>(HEADER_SIZE))
            break;

        // 빅엔디언 바디 길이 파싱
        uint32_t bodylen = 0;
        bodylen |= static_cast<uint32_t>(static_cast<uint8_t>(buf[0])) << 24;
        bodylen |= static_cast<uint32_t>(static_cast<uint8_t>(buf[1])) << 16;
        bodylen |= static_cast<uint32_t>(static_cast<uint8_t>(buf[2])) << 8;
        bodylen |= static_cast<uint32_t>(static_cast<uint8_t>(buf[3]));

        // 최대 바디 크기 초과 시 연결 종료
        if (bodylen > static_cast<uint32_t>(MAX_BODY_SIZE))
        {
            log_event("epollserver",
                      "패킷 크기 초과 | fd=" + std::to_string(fd) +
                      " | size=" + std::to_string(bodylen));
            close_client(fd);
            return;
        }

        // 전체 패킷 미도착 시 대기
        size_t total = static_cast<size_t>(HEADER_SIZE) + bodylen;
        if (buf.size() < total)
            break;

        // 바디 추출
        std::string jsonbody(buf.begin() + HEADER_SIZE,
                             buf.begin() + HEADER_SIZE + bodylen);

        // 처리한 패킷 버퍼에서 제거
        buf.erase(buf.begin(), buf.begin() + total);

        log_event("epollserver",
                  "패킷 수신 완료 | fd=" + std::to_string(fd) +
                  " | body=" + jsonbody);

        // 라우터로 전달
        router_.route(fd, jsonbody);
    }
}