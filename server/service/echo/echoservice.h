#pragma once

// ------------------------------------------------
// echoservice.h
// echo 비즈니스 로직 처리 서비스
//
// 흐름:
//   echohandler → echoservice.process() → 클라이언트 응답 전송
// ------------------------------------------------

// CMake include root = server/ 및 flowScope/common/
// protocol.h, packetclass.h 는 직접 참조 가능
#include "protocol.h"
#include "packetclass.h"
#include <string>

class echoservice
{
public:
    // packet을 받아 echo/ping 응답을 처리한다
    void process(const packet& pkt);

private:
    // echo 응답 JSON 생성
    std::string build_echo_response(const std::string& body,
                                    const std::string& traceid);
    // ping 응답 JSON 생성
    std::string build_ping_response(const std::string& traceid);
};