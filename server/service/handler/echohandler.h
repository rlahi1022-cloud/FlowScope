#pragma once

// ------------------------------------------------
// echohandler.h
// echo / ping 요청을 처리하는 구체 핸들러
// router로부터 packet을 받아
// internal_protocol에 따라 echoservice를 호출한다
//
// 흐름:
//   router → echohandler → echoservice → eventbus
// ------------------------------------------------

// CMake include root = server/
// 모든 경로는 server/ 기준으로 작성한다
#include "service/handler/basehandler.h"
#include "service/echo/echoservice.h"
#include "protocol.h"
#include <functional>
#include <unordered_map>

// ------------------------------------------------
// echohandler
// basehandler를 상속받는 구체 핸들러
// internal_protocol 기반으로 echoservice를 호출한다
// ------------------------------------------------
class echohandler : public basehandler
{
public:
    // ------------------------------------------------
    // 생성자: internal_protocol → 처리 함수 매핑 초기화
    // ------------------------------------------------
    echohandler();

    // ------------------------------------------------
    // handle
    // router로부터 packet을 받아
    // handlermap에서 해당 protocol의 처리 함수를 호출한다
    // ------------------------------------------------
    void handle(const packet& pkt) override;

private:
    // ------------------------------------------------
    // handlermap
    // internal_protocol → 처리 함수 매핑 테이블
    // switch 분기 대신 map 기반으로 확장성을 확보한다
    // ------------------------------------------------
    std::unordered_map<int, std::function<void(const packet&)>> handlermap;

    // echo 비즈니스 로직 처리 객체
    echoservice service;
};