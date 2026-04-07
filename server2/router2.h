#pragma once

// ------------------------------------------------
// router2.h
// server2 라우터
//
// 흐름:
//   epollserver2 → router2.route(fd, jsonbody)
//                → cmd 파싱 → proto 변환
//                → traceid 발급
//                → handler 선택 → handle(packet2)
//
// server2 특징:
//   - target 포워딩 없음 (하위 서버 자체)
//   - EventBus 없음
//   - 처리 결과를 handler → service → fd 직접 write
// ------------------------------------------------

#include "common/protocol.h"
#include "common/packetclass.h"
#include "service/handler/basehandler2.h"
#include <string>
#include <memory>
#include <unordered_map>

// ------------------------------------------------
// router2
// cmd → internal_protocol 변환 및 핸들러 분기
// ------------------------------------------------
class router2
{
public:
    // 생성자: 핸들러 등록
    router2();

    // ------------------------------------------------
    // route
    // fd : 요청 클라이언트 소켓
    // jsonbody : 수신된 JSON 문자열
    // ------------------------------------------------
    void route(int fd, const std::string& jsonbody);

private:
    // JSON에서 "cmd" 필드 추출
    std::string parse_cmd(const std::string& jsonbody);

    // internal_protocol → 핸들러 매핑 테이블
    std::unordered_map<int, std::shared_ptr<basehandler2>> routemap;
};