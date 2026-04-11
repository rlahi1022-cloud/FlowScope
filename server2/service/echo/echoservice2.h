#pragma once

// ------------------------------------------------
// echoservice2.h
// server2 echo 비즈니스 로직 서비스
//
// server1과의 차이:
//   - server1: session 안에서 직접 처리
//   - server2: epoll 스레드 → router → handler → service
//              서비스가 직접 fd로 write (EventBus 없음)
//
// 흐름:
//   echohandler2 → echoservice2.process() → fd로 직접 write
// ------------------------------------------------

#include "common/packetclass.h"
#include <string>

// ------------------------------------------------
// echoservice2
// echo / ping 응답을 처리하고 fd로 직접 전송
// ------------------------------------------------
class echoservice2
{
public:
    // ------------------------------------------------
    // process
    // packet2를 받아 echo/ping 응답을 생성하고
    // fd로 직접 write한다 (EventBus 경유 없음)
    // ------------------------------------------------
    void process(const packet2& pkt);

private:
    // echo 응답 JSON 생성
    std::string build_echo_response(const std::string& body,
                                    const std::string& traceid);

    // ping 응답 JSON 생성
    std::string build_ping_response(const std::string& traceid);

    // fd로 4byte 헤더 + body 전송
    void send_response(int fd, const std::string& body);
};