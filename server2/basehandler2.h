#pragma once

// ------------------------------------------------
// basehandler2.h
// server2 핸들러 추상 기반 클래스
//
// 역할:
//   - 모든 server2 핸들러의 공통 인터페이스
//   - handle(packet2) 순수 가상 함수 정의
// ------------------------------------------------

#include "common/packetclass.h"

// ------------------------------------------------
// basehandler2
// server2 핸들러 추상 클래스
// ------------------------------------------------
class basehandler2
{
public:
    virtual ~basehandler2() = default;

    // ------------------------------------------------
    // handle
    // packet2를 받아 비즈니스 로직을 처리한다
    // server2는 EventBus 없이 직접 fd로 응답 전송
    // ------------------------------------------------
    virtual void handle(const packet2& pkt) = 0;
};