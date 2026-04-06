#pragma once

// ------------------------------------------------
// basehandler.h
// 모든 핸들러의 추상 기반 클래스
// ------------------------------------------------

#include "common/protocol.h"
#include "common/packetclass.h"
#include <string>

class basehandler
{
public:
    virtual ~basehandler() = default;
    virtual void handle(const packet& pkt) = 0;
};