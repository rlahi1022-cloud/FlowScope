#pragma once

// ------------------------------------------------
// router.h
// 외부 JSON 요청을 내부 프로토콜로 변환하고
// 해당 핸들러로 라우팅하는 컴포넌트
//
// 흐름 (로컬 처리):
//   epollserver → router.route(fd, json)
//              → target 파싱 → none이면 로컬 처리
//              → string_to_protocol 변환
//              → traceid 발급
//              → handler 선택 → handler.handle(packet)
//
// 흐름 (포워딩):
//   epollserver → router.route(fd, json)
//              → target 파싱 → server1~4이면 포워딩
//              → forwarder.forward(fd, target, json)
// ------------------------------------------------

#include "protocol.h"
#include "service/handler/basehandler.h"
#include "forwarder.h"
#include <string>
#include <memory>
#include <unordered_map>

// ------------------------------------------------
// router
// cmd 문자열 → internal_protocol 변환 및 핸들러 분기
// target 필드 있으면 forwarder로 포워딩
// ------------------------------------------------
class router
{
public:
    router();

    // ------------------------------------------------
    // route
    // fd: 요청이 들어온 클라이언트 소켓
    // jsonbody: 수신된 JSON 문자열
    // target 필드 파싱 후 포워딩 or 로컬 처리 분기
    // ------------------------------------------------
    void route(int fd, const std::string& jsonbody);

private:
    // cmd 파싱
    std::string parse_cmd(const std::string& jsonbody);

    // target 파싱
    std::string parse_target(const std::string& jsonbody);

    // internal_protocol → 핸들러 매핑 테이블
    std::unordered_map<int, std::shared_ptr<basehandler>> routemap;

    // 포워더
    forwarder forwarder_;
};