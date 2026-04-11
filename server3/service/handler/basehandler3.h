#pragma once

// ------------------------------------------------
// basehandler3.h
// server3 핸들러 추상 기반 클래스
//
// 역할:
//   - 모든 server3 핸들러의 공통 인터페이스
//   - handle(payload) 순수 가상 함수 정의
//
// server3 특징:
//   - EventBus 경유: eventbus publish → handler.handle()
//   - handler는 처리 후 eventbus publish("response", ...)
// ------------------------------------------------

#include <string>

// ------------------------------------------------
// basehandler3
// server3 핸들러 추상 클래스
// ------------------------------------------------
class basehandler3
{
public:
    virtual ~basehandler3() = default;

    // ------------------------------------------------
    // handle
    // payload를 받아 비즈니스 로직을 처리한다
    // payload 형식: "fd:traceid:jsonbody"
    //
    // 처리 완료 후 eventbus.publish("response", payload) 호출
    // 응답 payload 형식: "fd:traceid:response_json"
    // ------------------------------------------------
    virtual void handle(const std::string& payload) = 0;
};
