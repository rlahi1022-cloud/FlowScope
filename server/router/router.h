#pragma once

// ------------------------------------------------
// router.h
// 외부 JSON 요청을 내부 프로토콜로 변환하고
// 해당 핸들러로 라우팅하는 컴포넌트
//
// 흐름:
//   epollserver → router.route(fd, json)
//              → string_to_protocol 변환
//              → traceid 발급
//              → handler 선택 → handler.handle(packet)
// ------------------------------------------------

#include "common/protocol.h"
#include "service/handler/basehandler.h"
#include <string>
#include <memory>
#include <unordered_map>

// ------------------------------------------------
// router
// cmd 문자열 → internal_protocol 변환 및 핸들러 분기
// ------------------------------------------------
class router
{
public:
    // ------------------------------------------------
    // 생성자: 핸들러 등록 및 라우팅 테이블 초기화
    // ------------------------------------------------
    router();

    // ------------------------------------------------
    // route
    // fd: 요청이 들어온 클라이언트 소켓
    // jsonbody: 수신된 JSON 문자열
    // JSON에서 cmd를 파싱하여 internal_protocol로 변환
    // traceid를 발급하고 해당 핸들러를 호출한다
    // ------------------------------------------------
    void route(int fd, const std::string& jsonbody);

private:
    // ------------------------------------------------
    // parse_cmd
    // JSON 문자열에서 "cmd" 필드 값을 추출한다
    // 외부 라이브러리 없이 단순 문자열 파싱 사용
    // ------------------------------------------------
    std::string parse_cmd(const std::string& jsonbody);

    // ------------------------------------------------
    // routemap
    // internal_protocol → 핸들러 매핑 테이블
    // ------------------------------------------------
    std::unordered_map<int, std::shared_ptr<basehandler>> routemap;
};