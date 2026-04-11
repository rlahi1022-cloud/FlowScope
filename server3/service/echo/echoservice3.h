#pragma once

// ------------------------------------------------
// echoservice3.h
// server3 echo 비즈니스 로직 서비스
//
// server3 특징:
//   - EventBus 경유: handler 요청 수신 → 처리 → EventBus publish
//   - payload 형식: "fd:traceid:jsonbody"
//
// 흐름:
//   echohandler3 → echoservice3.process(payload)
//               → eventbus3.publish("response", response_payload)
// ------------------------------------------------

#include <string>

// ------------------------------------------------
// echoservice3
// echo / ping 응답을 처리하고 EventBus "response" 토픽으로 publish
// ------------------------------------------------
class echoservice3
{
public:
    // ------------------------------------------------
    // process
    // payload를 받아 echo/ping 응답을 생성하고
    // EventBus "response" 토픽으로 publish
    // payload 형식: "fd:traceid:jsonbody"
    // ------------------------------------------------
    void process(const std::string& payload);

private:
    // payload 파싱
    bool parse_payload(const std::string& payload,
                      int& fd,
                      std::string& traceid,
                      std::string& jsonbody);

    // echo 응답 JSON 생성
    std::string build_echo_response(const std::string& jsonbody,
                                    const std::string& traceid);

    // ping 응답 JSON 생성
    std::string build_ping_response(const std::string& traceid);

    // JSON에서 "cmd" 필드 추출
    std::string parse_cmd(const std::string& jsonbody);
};
