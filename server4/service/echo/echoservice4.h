#pragma once

// ------------------------------------------------
// echoservice4.h
// server4 echo/ping/debug_status 서비스
//
// 책임:
//   - echo/ping/debug_status 비즈니스 로직 처리
//   - 응답 JSON 구성
//   - eventbus4 "response" 토픽에 publish
//     (최종적으로 main.cpp의 response subscriber가 fd로 write)
//
// 응답 형식:
//   {
//     "cmd": "echo_response" | "pong" | "debug_status_response",
//     "traceid": "TRC-XXXXXXXX",
//     "server": "server4",
//     "data": {...}
//   }
// ------------------------------------------------

#include "common/packetclass.h"
#include <string>

// ------------------------------------------------
// echoservice4
// ------------------------------------------------
class echoservice4 {
public:
    // ------------------------------------------------
    // process
    // packet4를 받아 proto에 따라 처리
    // 완료 후 eventbus4에 "response" publish
    // ------------------------------------------------
    void process(const packet4& pkt);

private:
    // ------------------------------------------------
    // build_echo_response
    // echo 응답 JSON 생성
    // ------------------------------------------------
    std::string build_echo_response(const std::string& body,
                                    const std::string& traceid);

    // ------------------------------------------------
    // build_ping_response
    // ping 응답 JSON 생성
    // ------------------------------------------------
    std::string build_ping_response(const std::string& traceid);

    // ------------------------------------------------
    // build_debug_status_response
    // debug_status 응답 JSON 생성
    // ------------------------------------------------
    std::string build_debug_status_response(const std::string& traceid);

    // ------------------------------------------------
    // publish_response
    // 응답을 eventbus4 "response" 토픽에 publish
    // 형식: "fd:traceid:json"
    // ------------------------------------------------
    void publish_response(int fd,
                         const std::string& traceid,
                         const std::string& json);
};
