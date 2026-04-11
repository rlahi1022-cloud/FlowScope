#pragma once

// ------------------------------------------------
// echohandler4.h
// server4 echo/ping/debug_status 핸들러
//
// 흐름:
//   router4 → echohandler4.handle() → echoservice4.process()
//
// server4 특징:
//   - EventBus 있음 → echoservice4가 eventbus4에 publish
//   - 핸들러맵 기반 proto 분기 (확장성)
// ------------------------------------------------

#include "service/handler/basehandler4.h"
#include "service/echo/echoservice4.h"
#include "common/protocol.h"
#include <functional>
#include <unordered_map>

// ------------------------------------------------
// echohandler4
// echo / ping / debug_status 요청을 echoservice4로 위임
// ------------------------------------------------
class echohandler4 : public basehandler4
{
public:
    // 생성자: handlermap 초기화
    echohandler4();

    // ------------------------------------------------
    // handle
    // packet4를 받아 handlermap에서 처리 함수 호출
    // ------------------------------------------------
    void handle(const packet4& pkt) override;

private:
    // internal_protocol → 처리 함수 매핑
    std::unordered_map<int, std::function<void(const packet4&)>> handlermap;

    // echo 비즈니스 로직 처리 객체
    echoservice4 service;
};
