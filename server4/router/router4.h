#pragma once

// ------------------------------------------------
// router4.h
// server4 라우터
//
// 책임:
//   1. JSON body에서 cmd 파싱
//   2. traceid 발급
//   3. proto 변환
//   4. dispatcher로 처리 경로 결정
//   5. 경로별 처리:
//      - sync: 핸들러 직접 호출
//      - async: jobqueue4 push
//      - event: eventbus4 publish
// ------------------------------------------------

#include <memory>
#include <string>
#include <unordered_map>
#include "dispatcher/dispatch4.h"
#include "service/handler/basehandler4.h"
#include "jobqueue/jobqueue4.h"

// ------------------------------------------------
// router4
// ------------------------------------------------
class router4 {
public:
    // ------------------------------------------------
    // 생성자
    // 핸들러 등록
    // ------------------------------------------------
    router4();

    // ------------------------------------------------
    // route
    // fd와 JSON body를 받아
    // dispatcher를 통해 처리 경로 결정 후 처리
    // ------------------------------------------------
    void route(int fd, const std::string& jsonbody);

private:
    // ------------------------------------------------
    // parse_cmd
    // JSON에서 "cmd" 필드 추출
    // ------------------------------------------------
    std::string parse_cmd(const std::string& jsonbody);

    // ------------------------------------------------
    // parse_traceid (선택)
    // JSON에서 기존 traceid 추출 (없으면 새로 발급)
    // ------------------------------------------------
    std::string parse_traceid(const std::string& jsonbody);

    // internal_protocol → 핸들러 매핑
    std::unordered_map<int, std::shared_ptr<basehandler4>> routemap;
};
