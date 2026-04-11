#pragma once

// ------------------------------------------------
// basehandler4.h
// server4 핸들러 추상 기반 클래스
//
// 역할:
//   - 모든 server4 핸들러의 공통 인터페이스
//   - handle(packet4) 순수 가상 함수 정의
// ------------------------------------------------

#include "common/packetclass.h"

// ------------------------------------------------
// basehandler4
// server4 핸들러 추상 클래스
// ------------------------------------------------
class basehandler4
{
public:
    virtual ~basehandler4() = default;

    // ------------------------------------------------
    // handle
    // packet4를 받아 비즈니스 로직을 처리한다
    // server4는 EventBus를 통해 응답 전송
    // ------------------------------------------------
    virtual void handle(const packet4& pkt) = 0;
};
