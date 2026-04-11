#pragma once

// ------------------------------------------------
// uieventhandler.h
// 클라이언트 UI 이벤트 요청을 처리하는 구체 핸들러
// router로부터 packet을 받아
// internal_protocol에 따라 uieventservice를 호출한다
//
// 흐름:
//   router → uieventhandler → uieventservice → eventbus
// ------------------------------------------------

#include "service/handler/basehandler.h"
#include "service/uievent/uieventservice.h"
#include "protocol.h"
#include <functional>
#include <unordered_map>

class uieventhandler : public basehandler
{
public:
    // ------------------------------------------------
    // 생성자: internal_protocol → 처리 함수 매핑 초기화
    // ------------------------------------------------
    uieventhandler();

    // ------------------------------------------------
    // handle
    // router로부터 packet을 받아
    // handlermap에서 해당 protocol의 처리 함수를 호출한다
    // ------------------------------------------------
    void handle(const packet& pkt) override;

private:
    // internal_protocol → 처리 함수 매핑 테이블
    std::unordered_map<int, std::function<void(const packet&)>> handlermap;

    // UI 이벤트 비즈니스 로직 처리 객체
    uieventservice service;
};
