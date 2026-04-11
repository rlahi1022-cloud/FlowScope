// ------------------------------------------------
// echohandler3.cpp
// server3 echo/ping 핸들러 구현부
// ------------------------------------------------

#include "service/handler/echohandler3.h"
#include "common/logger.h"

// ------------------------------------------------
// 생성자
// ------------------------------------------------
echohandler3::echohandler3()
{
    log_event("echohandler3", "핸들러 초기화");
}

// ------------------------------------------------
// handle
// EventBus로부터 payload 수신 후 service로 위임
// payload 형식: "fd:traceid:jsonbody"
// ------------------------------------------------
void echohandler3::handle(const std::string& payload)
{
    log_event("echohandler3",
              "payload 수신 | payload=" + payload.substr(0, 50) + "...");
    log_flow("?", "eventbus3", "echohandler3", "handle() 호출");

    service.process(payload);
}
