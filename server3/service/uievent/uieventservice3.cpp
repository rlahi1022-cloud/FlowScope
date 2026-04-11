// ------------------------------------------------
// uieventservice3.cpp
// server3 UI event 비즈니스 로직 구현부
// ------------------------------------------------

#include "service/uievent/uieventservice3.h"
#include "eventbus/eventbus3.h"
#include "common/logger.h"
#include "common/protocol.h"

// ------------------------------------------------
// process
// payload 파싱 → ui_* 이벤트 처리 → EventBus publish("response", ...)
// ------------------------------------------------
void uieventservice3::process(const std::string& payload)
{
    int fd;
    std::string traceid;
    std::string jsonbody;

    if (!parse_payload(payload, fd, traceid, jsonbody))
    {
        log_event("uieventservice3",
                  "payload 파싱 실패 | 처리 중단",
                  "payload=" + payload);
        return;
    }

    std::string cmd = parse_cmd(jsonbody);
    internal_protocol proto = string_to_protocol(cmd);

    log_event("uieventservice3",
              "처리 시작 | proto=" + protocol_to_string(proto),
              "", traceid);
    log_flow(traceid, "uieventhandler3", "uieventservice3",
             "proto=" + protocol_to_string(proto));

    std::string response;

    switch (proto)
    {
        case internal_protocol::ui_btn_click:
            response = handle_ui_btn_click(traceid);
            break;

        case internal_protocol::ui_server_select:
            response = handle_ui_server_select(traceid);
            break;

        case internal_protocol::ui_connect:
            response = handle_ui_connect(traceid);
            break;

        case internal_protocol::ui_disconnect:
            response = handle_ui_disconnect(traceid);
            break;

        case internal_protocol::ui_chat_msg:
            response = handle_ui_chat_msg(traceid);
            break;

        case internal_protocol::ui_flow_start:
            response = handle_ui_flow_start(traceid);
            break;

        case internal_protocol::ui_flow_stop:
            response = handle_ui_flow_stop(traceid);
            break;

        default:
            log_event("uieventservice3",
                      "알 수 없는 프로토콜 - 처리 중단",
                      "", traceid);
            return;
    }

    log_event("uieventservice3",
              "처리 완료 - EventBus publish | fd=" + std::to_string(fd),
              "response=" + response.substr(0, 100) + "...", traceid);
    log_flow(traceid, "uieventservice3", "eventbus3",
             "topic=response | fd=" + std::to_string(fd));

    // EventBus publish: payload = "fd:traceid:json"
    std::string response_payload = std::to_string(fd) + ":" + traceid + ":" + response;
    eventbus3::instance().publish("response", response_payload);
}

// ------------------------------------------------
// parse_payload
// payload 파싱: "fd:traceid:jsonbody"
// ------------------------------------------------
bool uieventservice3::parse_payload(const std::string& payload,
                                     int& fd,
                                     std::string& traceid,
                                     std::string& jsonbody)
{
    // 첫 번째 ':' 찾기 (fd와 traceid 구분)
    size_t pos1 = payload.find(':');
    if (pos1 == std::string::npos) return false;

    // fd 파싱
    try {
        fd = std::stoi(payload.substr(0, pos1));
    } catch (...) {
        return false;
    }

    // 두 번째 ':' 찾기 (traceid와 jsonbody 구분)
    size_t pos2 = payload.find(':', pos1 + 1);
    if (pos2 == std::string::npos) return false;

    traceid = payload.substr(pos1 + 1, pos2 - pos1 - 1);
    jsonbody = payload.substr(pos2 + 1);

    return true;
}

// ------------------------------------------------
// parse_cmd
// JSON에서 "cmd" 필드 추출
// ------------------------------------------------
std::string uieventservice3::parse_cmd(const std::string& jsonbody)
{
    std::string key = "\"cmd\"";
    size_t pos = jsonbody.find(key);
    if (pos == std::string::npos) return "";

    pos = jsonbody.find(':', pos + key.size());
    if (pos == std::string::npos) return "";

    ++pos;
    while (pos < jsonbody.size() && jsonbody[pos] == ' ') ++pos;
    if (pos >= jsonbody.size() || jsonbody[pos] != '"') return "";
    ++pos;

    size_t end = jsonbody.find('"', pos);
    if (end == std::string::npos) return "";

    return jsonbody.substr(pos, end - pos);
}

// ------------------------------------------------
// handle_ui_btn_click
// 버튼 클릭 이벤트 처리
// ------------------------------------------------
std::string uieventservice3::handle_ui_btn_click(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_btn_click_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server3\",";
    json += "\"flow_step\":1,";
    json += "\"data\":{";
    json += "\"button\":\"unknown\",";
    json += "\"status\":\"ok\",";
    json += "\"message\":\"Button event processed (EventBus)\"";
    json += "}}";
    return json;
}

// ------------------------------------------------
// handle_ui_server_select
// 서버 선택 변경 이벤트 처리
// ------------------------------------------------
std::string uieventservice3::handle_ui_server_select(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_server_select_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server3\",";
    json += "\"flow_step\":-1,";
    json += "\"data\":{";
    json += "\"server_name\":\"EventBus\",";
    json += "\"status\":\"ok\",";
    json += "\"message\":\"Server switched to EventBus\"";
    json += "}}";
    return json;
}

// ------------------------------------------------
// handle_ui_connect
// 서버 연결 요청 처리
// ------------------------------------------------
std::string uieventservice3::handle_ui_connect(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_connect_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server3\",";
    json += "\"flow_step\":0,";
    json += "\"data\":{";
    json += "\"status\":\"connected\",";
    json += "\"message\":\"Client connected to server3 (EventBus)\"";
    json += "}}";
    return json;
}

// ------------------------------------------------
// handle_ui_disconnect
// 서버 연결 해제 처리
// ------------------------------------------------
std::string uieventservice3::handle_ui_disconnect(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_disconnect_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server3\",";
    json += "\"flow_step\":-1,";
    json += "\"data\":{";
    json += "\"status\":\"disconnected\",";
    json += "\"message\":\"Client disconnected\"";
    json += "}}";
    return json;
}

// ------------------------------------------------
// handle_ui_chat_msg
// 채팅 메시지 전송 처리
// 주의: cmd는 "ui_chat_response" (클라이언트가 이 이름으로 파싱)
// ------------------------------------------------
std::string uieventservice3::handle_ui_chat_msg(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_chat_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server3\",";
    json += "\"flow_step\":3,";
    json += "\"data\":{";
    json += "\"message\":\"[Server3 Echo] Message received via EventBus\",";
    json += "\"status\":\"ok\"";
    json += "}}";
    return json;
}

// ------------------------------------------------
// handle_ui_flow_start
// 흐름 처리 시작 요청 처리
// ------------------------------------------------
std::string uieventservice3::handle_ui_flow_start(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_flow_start_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server3\",";
    json += "\"flow_step\":0,";
    json += "\"data\":{";
    json += "\"status\":\"started\",";
    json += "\"message\":\"Flow started on server3 (EventBus)\"";
    json += "}}";
    return json;
}

// ------------------------------------------------
// handle_ui_flow_stop
// 흐름 처리 중지 요청 처리
// ------------------------------------------------
std::string uieventservice3::handle_ui_flow_stop(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_flow_stop_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"server\":\"server3\",";
    json += "\"flow_step\":-1,";
    json += "\"data\":{";
    json += "\"status\":\"stopped\",";
    json += "\"message\":\"Flow stopped\"";
    json += "}}";
    return json;
}
