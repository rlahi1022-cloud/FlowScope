#pragma once

// ------------------------------------------------
// router3.h
// server3 라우터
//
// 흐름:
//   epollserver3 → router3.route(fd, jsonbody)
//               → cmd 파싱 → proto 변환
//               → traceid 발급
//               → packet3 생성 → eventbus3.publish("request", ...)
//
// server3 특징:
//   - 핸들러 직접 호출 없음
//   - EventBus로 "request" 토픽 publish
//   - 구독 핸들러가 처리 후 "response" 토픽 publish
// ------------------------------------------------

#include "common/protocol.h"
#include "common/packetclass.h"
#include <string>

// ------------------------------------------------
// router3
// cmd → internal_protocol 변환 및 EventBus publish
// ------------------------------------------------
class router3
{
public:
    // 생성자
    router3();

    // ------------------------------------------------
    // route
    // fd : 요청 클라이언트 소켓
    // jsonbody : 수신된 JSON 문자열
    // ------------------------------------------------
    void route(int fd, const std::string& jsonbody);

private:
    // JSON에서 "cmd" 필드 추출
    std::string parse_cmd(const std::string& jsonbody);
};
