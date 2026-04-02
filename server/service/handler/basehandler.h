#pragma once

// ------------------------------------------------
// basehandler.h
// 모든 핸들러의 추상 기반 클래스
// handler는 router로부터 packet을 받아
// internal_protocol 기반으로 서비스를 호출한다
// ------------------------------------------------

#include "common/protocol.h"
#include <string>

// ------------------------------------------------
// basehandler
// 순수 가상 함수 handle()을 정의한다
// 모든 구체 핸들러는 이 클래스를 상속받는다
// ------------------------------------------------
class basehandler
{
public:
    virtual ~basehandler() = default;

    // ------------------------------------------------
    // handle
    // 요청 packet을 받아 처리한다
    // 구체 핸들러에서 반드시 구현해야 한다
    // ------------------------------------------------
    virtual void handle(const packet& pkt) = 0;
};