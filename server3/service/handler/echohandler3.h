#pragma once

// ------------------------------------------------
// echohandler3.h
// server3 echo/ping 핸들러
//
// 흐름:
//   eventbus3.publish("request", payload)
//   → echohandler3.handle(payload)
//   → echoservice3.process()
//   → eventbus3.publish("response", response_payload)
//
// server3 특징:
//   - EventBus 경유: 요청/응답 모두 EventBus 토픽 사용
//   - 핸들러: 요청 처리 후 eventbus.publish("response", ...)
//   - main.cpp: response 토픽 구독하여 fd로 write
// ------------------------------------------------

#include "service/handler/basehandler3.h"
#include "service/echo/echoservice3.h"

// ------------------------------------------------
// echohandler3
// echo / ping 요청을 echoservice3로 위임
// ------------------------------------------------
class echohandler3 : public basehandler3
{
public:
    // 생성자
    echohandler3();

    // ------------------------------------------------
    // handle
    // payload를 받아 echo/ping 처리
    // payload 형식: "fd:traceid:jsonbody"
    // ------------------------------------------------
    void handle(const std::string& payload) override;

private:
    // echo 비즈니스 로직 처리 객체
    echoservice3 service;
};
