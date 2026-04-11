// ------------------------------------------------
// uieventhandler3.cpp
// server3 UI event 핸들러 구현부
// ------------------------------------------------

#include "service/handler/uieventhandler3.h"
#include "common/logger.h"

// ------------------------------------------------
// 생성자
// ------------------------------------------------
uieventhandler3::uieventhandler3()
{
    log_event("uieventhandler3", "핸들러 초기화");
}

// ------------------------------------------------
// handle
// EventBus로부터 payload 수신 후 service로 위임
// payload 형식: "fd:traceid:jsonbody"
// ------------------------------------------------
void uieventhandler3::handle(const std::string& payload)
{
    log_event("uieventhandler3",
              "payload 수신 | payload=" + payload.substr(0, 50) + "...");
    log_flow("?", "eventbus3", "uieventhandler3", "handle() 호출");

    service.process(payload);
}
