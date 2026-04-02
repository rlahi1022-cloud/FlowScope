#pragma once

// ------------------------------------------------
// echoservice.h
// echo 비즈니스 로직 처리 서비스
// echohandler로부터 호출되며
// 처리 결과를 eventbus에 publish한다
//
// Service는 Worker를 직접 호출하지 않는다
// 반드시 eventbus를 통해 결과를 전달한다
// ------------------------------------------------

#include "common/protocol.h"
#include <string>

// ------------------------------------------------
// echoservice
// echo / ping 요청에 대한 응답 생성
// 결과는 eventbus topic "response"로 publish된다
// ------------------------------------------------
class echoservice
{
public:
    // ------------------------------------------------
    // process
    // packet을 받아 echo 또는 ping 응답을 생성하고
    // eventbus를 통해 결과를 전달한다
    // ------------------------------------------------
    void process(const packet& pkt);

private:
    // ------------------------------------------------
    // build_echo_response
    // echo 응답 JSON 문자열 생성
    // ------------------------------------------------
    std::string build_echo_response(const std::string& body,
                                    const std::string& traceid);

    // ------------------------------------------------
    // build_ping_response
    // ping 응답 JSON 문자열 생성
    // ------------------------------------------------
    std::string build_ping_response(const std::string& traceid);
};