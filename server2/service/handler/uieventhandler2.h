#pragma once

// ------------------------------------------------
// uieventhandler2.h
// server2 UI 이벤트 핸들러
//
// 흐름:
//   router2 → uieventhandler2.handle() → uieventservice2.process()
//
// server2 특징:
//   - EventBus 없음 → uieventservice2가 직접 fd write
//   - handlermap 기반 proto 분기 (확장성)
// ------------------------------------------------

#include "service/handler/basehandler2.h"
#include "service/uievent/uieventservice2.h"
#include "common/protocol.h"
#include <functional>
#include <unordered_map>

// ------------------------------------------------
// uieventhandler2
// ui_* 요청을 uieventservice2로 위임
// ------------------------------------------------
class uieventhandler2 : public basehandler2
{
public:
    // 생성자: handlermap 초기화 (7개 UI 프로토콜 등록)
    uieventhandler2();

    // ------------------------------------------------
    // handle
    // packet2를 받아 handlermap에서 처리 함수 호출
    // ------------------------------------------------
    void handle(const packet2& pkt) override;

private:
    // internal_protocol → 처리 함수 매핑
    std::unordered_map<int, std::function<void(const packet2&)>> handlermap;

    // UI 이벤트 비즈니스 로직 처리 객체
    uieventservice2 service;
};
