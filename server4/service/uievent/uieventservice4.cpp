// ------------------------------------------------
// uieventservice4.cpp
// server4 UI 이벤트 서비스 구현부
// ------------------------------------------------

#include "service/uievent/uieventservice4.h"
#include "common/logger.h"
#include "eventbus/eventbus4.h"
#include <cstdlib>
#include <ctime>

// ------------------------------------------------
// process
// UI 이벤트별 응답 생성 후 eventbus4에 publish
// ------------------------------------------------
void uieventservice4::process(const packet4& pkt)
{
    log_event("uieventservice4",
              "처리 시작 | proto=" + protocol_to_string(pkt.ctx.proto),
              "", pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "uieventhandler4", "uieventservice4",
             "proto=" + protocol_to_string(pkt.ctx.proto));

    std::string response = build_ui_response(pkt);

    log_event("uieventservice4",
              "처리 완료 - eventbus4에 publish | fd=" + std::to_string(pkt.fd),
              "response=" + response, pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "uieventservice4", "eventbus4 (response)",
             "fd=" + std::to_string(pkt.fd));

    // EventBus 거쳐서 publish
    publish_response(pkt.fd, pkt.ctx.traceid, response);
}

// ------------------------------------------------
// build_ui_response
// UI 이벤트별 응답 JSON 생성
// 클라이언트가 파싱하는 cmd명과 data.message 필드 포함
// ------------------------------------------------
std::string uieventservice4::build_ui_response(const packet4& pkt)
{
    std::string response_cmd = get_response_cmd(pkt.ctx.cmd);
    int flow_step = -1;
    std::string message;

    switch (pkt.ctx.proto)
    {
        case internal_protocol::ui_btn_click:
            flow_step = 1;
            message = "Button event processed (Hybrid)";
            break;
        case internal_protocol::ui_server_select:
            flow_step = -1;
            message = "Server switched to Hybrid";
            break;
        case internal_protocol::ui_connect:
            flow_step = 0;
            message = "Client connected to server4 (Hybrid)";
            break;
        case internal_protocol::ui_disconnect:
            flow_step = -1;
            message = "Client disconnected";
            break;
        case internal_protocol::ui_chat_msg:
            flow_step = 5;  // Hybrid: Worker 단계
            message = "[Server4 Echo] Message processed via Hybrid pipeline";
            break;
        case internal_protocol::ui_flow_start:
            flow_step = 0;
            message = "Flow started on server4 (Hybrid)";
            break;
        case internal_protocol::ui_flow_stop:
            flow_step = -1;
            message = "Flow stopped";
            break;
        default:
            flow_step = -1;
            message = "Unknown UI event";
            break;
    }

    std::string json;
    json += "{";
    json += "\"cmd\":\"" + response_cmd + "\",";
    json += "\"traceid\":\"" + pkt.ctx.traceid + "\",";
    json += "\"server\":\"server4\",";
    json += "\"flow_step\":" + std::to_string(flow_step) + ",";
    json += "\"data\":{";
    json += "\"message\":\"" + message + "\",";
    json += "\"status\":\"ok\"";
    json += "}}";
    return json;
}

// ------------------------------------------------
// publish_response
// 응답을 eventbus4 "response" 토픽에 publish
// 형식: "fd:traceid:json"
// ------------------------------------------------
void uieventservice4::publish_response(int fd,
                                      const std::string& traceid,
                                      const std::string& json)
{
    std::string data = std::to_string(fd) + ":" + traceid + ":" + json;
    eventbus4::instance().publish("response", data);

    log_event("uieventservice4",
              "eventbus4 publish 완료 | topic=response",
              "fd=" + std::to_string(fd), traceid);
}

// ------------------------------------------------
// get_response_cmd
// 원본 cmd에 "_response" suffix 추가
// ------------------------------------------------
std::string uieventservice4::get_response_cmd(const std::string& cmd)
{
    // 클라이언트가 기대하는 응답 cmd명으로 변환
    // ui_chat_msg → ui_chat_response (클라이언트 파싱 규칙)
    if (cmd == "ui_chat_msg") return "ui_chat_response";
    return cmd + "_response";
}
