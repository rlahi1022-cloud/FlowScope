#pragma once

// ------------------------------------------------
// echohandler2.h
// server2 echo/ping 핸들러
//
// 흐름:
//   router2 → echohandler2.handle() → echoservice2.process()
//
// server2 특징:
//   - EventBus 없음 → echoservice2가 직접 fd write
//   - handlermap 기반 proto 분기 (확장성)
// ------------------------------------------------

#include "service/handler/basehandler2.h"
#include "service/echo/echoservice2.h"
#include "common/protocol.h"
#include <functional>
#include <unordered_map>

// ------------------------------------------------
// echohandler2
// echo / ping 요청을 echoservice2로 위임
// ------------------------------------------------
class echohandler2 : public basehandler2
{
public:
    // 생성자: handlermap 초기화
    echohandler2();

    // ------------------------------------------------
    // handle
    // packet2를 받아 handlermap에서 처리 함수 호출
    // ------------------------------------------------
    void handle(const packet2& pkt) override;

private:
    // internal_protocol → 처리 함수 매핑
    std::unordered_map<int, std::function<void(const packet2&)>> handlermap;

    // echo 비즈니스 로직 처리 객체
    echoservice2 service;
};