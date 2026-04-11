// ------------------------------------------------
// uieventservice.cpp
// 클라이언트 UI 이벤트 비즈니스 로직 구현부
// 처리 결과를 eventbus topic "response"로 publish
//
// 응답 JSON 구조:
//   {
//     "cmd": "ui_xxx_response",
//     "traceid": "req-XXXX",
//     "flow_step": 2,           // 흐름도에서 활성화할 단계 (-1 = 비활성)
//     "data": { ... }           // 이벤트별 응답 데이터
//   }
// ------------------------------------------------

#include "service/uievent/uieventservice.h"
#include "eventbus/eventbus.h"
#include "common/logger.h"

void uieventservice::process(const packet& pkt)
{
    log_event("uieventservice",
              "처리 시작 | proto=" + protocol_to_string(pkt.ctx.proto),
              pkt.ctx.traceid);

    std::string response;

    switch (pkt.ctx.proto)
    {
        case internal_protocol::ui_btn_click:
            response = build_btn_click_response(pkt.ctx.jsonbody, pkt.ctx.traceid);
            break;

        case internal_protocol::ui_server_select:
            response = build_server_select_response(pkt.ctx.jsonbody, pkt.ctx.traceid);
            break;

        case internal_protocol::ui_connect:
            response = build_connect_response(pkt.ctx.jsonbody, pkt.ctx.traceid);
            break;

        case internal_protocol::ui_disconnect:
            response = build_disconnect_response(pkt.ctx.traceid);
            break;

        case internal_protocol::ui_chat_msg:
            response = build_chat_response(pkt.ctx.jsonbody, pkt.ctx.traceid);
            break;

        case internal_protocol::ui_flow_start:
            response = build_flow_start_response(pkt.ctx.jsonbody, pkt.ctx.traceid);
            break;

        case internal_protocol::ui_flow_stop:
            response = build_flow_stop_response(pkt.ctx.traceid);
            break;

        default:
            log_event("uieventservice",
                      "알 수 없는 UI 프로토콜 - 처리 중단",
                      pkt.ctx.traceid);
            return;
    }

    log_event("uieventservice",
              "처리 완료 | response=" + response,
              pkt.ctx.traceid);
    log_flow(pkt.ctx.traceid, "uieventservice", "eventbus",
             "topic=response");

    // eventbus로 publish (main.cpp의 response handler가 수신)
    std::string payload = std::to_string(pkt.fd)
                          + ":" + pkt.ctx.traceid
                          + ":" + response;
    eventbus::instance().publish("response", payload);
}

// ------------------------------------------------
// parse_field - JSON에서 문자열 필드 추출 (간이 파서)
// ------------------------------------------------
std::string uieventservice::parse_field(const std::string& json,
                                         const std::string& field)
{
    std::string key = "\"" + field + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + key.size());
    if (pos == std::string::npos) return "";

    ++pos;
    while (pos < json.size() && json[pos] == ' ') ++pos;
    if (pos >= json.size() || json[pos] != '"') return "";
    ++pos;

    size_t end = json.find('"', pos);
    if (end == std::string::npos) return "";

    return json.substr(pos, end - pos);
}

// ------------------------------------------------
// 버튼 클릭 응답
// 클라이언트가 어떤 버튼을 눌렀는지 확인하고
// 해당 동작에 맞는 flow_step을 반환
// ------------------------------------------------
std::string uieventservice::build_btn_click_response(
    const std::string& body, const std::string& traceid)
{
    std::string btn_name = parse_field(body, "button");

    // 버튼별 flow_step 매핑
    int flow_step = -1;
    if (btn_name == "connect")       flow_step = 0;
    else if (btn_name == "send")     flow_step = 1;
    else if (btn_name == "start")    flow_step = 0;
    else if (btn_name == "stop")     flow_step = -1;

    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_btn_click_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"flow_step\":" + std::to_string(flow_step) + ",";
    json += "\"data\":{";
    json += "\"button\":\"" + btn_name + "\",";
    json += "\"status\":\"ok\",";
    json += "\"message\":\"Button '" + btn_name + "' event received\"";
    json += "}";
    json += "}";
    return json;
}

// ------------------------------------------------
// 서버 선택 응답
// 클라이언트가 선택한 서버 타입을 확인하고
// 서버 정보와 flow 리셋을 응답
// ------------------------------------------------
std::string uieventservice::build_server_select_response(
    const std::string& body, const std::string& traceid)
{
    std::string server_type = parse_field(body, "server_type");
    std::string server_name = parse_field(body, "server_name");

    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_server_select_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"flow_step\":-1,";
    json += "\"data\":{";
    json += "\"server_type\":\"" + server_type + "\",";
    json += "\"server_name\":\"" + server_name + "\",";
    json += "\"status\":\"ok\",";
    json += "\"message\":\"Server switched to " + server_name + "\"";
    json += "}";
    json += "}";
    return json;
}

// ------------------------------------------------
// 연결 응답
// 클라이언트 연결 요청에 대한 서버 확인 응답
// flow_step = 0 (Client request 단계 활성화)
// ------------------------------------------------
std::string uieventservice::build_connect_response(
    const std::string& body, const std::string& traceid)
{
    std::string ip = parse_field(body, "ip");
    std::string port = parse_field(body, "port");

    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_connect_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"flow_step\":0,";
    json += "\"data\":{";
    json += "\"ip\":\"" + ip + "\",";
    json += "\"port\":\"" + port + "\",";
    json += "\"status\":\"connected\",";
    json += "\"message\":\"Client connected from " + ip + ":" + port + "\"";
    json += "}";
    json += "}";
    return json;
}

// ------------------------------------------------
// 연결 해제 응답
// flow_step = -1 (흐름도 비활성화)
// ------------------------------------------------
std::string uieventservice::build_disconnect_response(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_disconnect_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"flow_step\":-1,";
    json += "\"data\":{";
    json += "\"status\":\"disconnected\",";
    json += "\"message\":\"Client disconnected\"";
    json += "}";
    json += "}";
    return json;
}

// ------------------------------------------------
// 채팅 메시지 응답
// 메시지를 수신하고 echo + flow_step 업데이트
// 메시지 처리 흐름: Client request(0) → detect(1) → Dispatcher(2) → Service(3) → write(4) → response(5)
// ------------------------------------------------
std::string uieventservice::build_chat_response(
    const std::string& body, const std::string& traceid)
{
    std::string message = parse_field(body, "message");

    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_chat_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"flow_step\":3,";
    json += "\"data\":{";
    json += "\"message\":\"[Server Echo] " + message + "\",";
    json += "\"original\":\"" + message + "\",";
    json += "\"status\":\"ok\"";
    json += "}";
    json += "}";
    return json;
}

// ------------------------------------------------
// 흐름 시작 응답
// flow_step = 0 (첫 단계 활성화)
// ------------------------------------------------
std::string uieventservice::build_flow_start_response(
    const std::string& body, const std::string& traceid)
{
    std::string server_type = parse_field(body, "server_type");

    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_flow_start_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"flow_step\":0,";
    json += "\"data\":{";
    json += "\"server_type\":\"" + server_type + "\",";
    json += "\"status\":\"started\",";
    json += "\"message\":\"Flow started for server type " + server_type + "\"";
    json += "}";
    json += "}";
    return json;
}

// ------------------------------------------------
// 흐름 중지 응답
// flow_step = -1 (흐름도 비활성화)
// ------------------------------------------------
std::string uieventservice::build_flow_stop_response(const std::string& traceid)
{
    std::string json;
    json += "{";
    json += "\"cmd\":\"ui_flow_stop_response\",";
    json += "\"traceid\":\"" + traceid + "\",";
    json += "\"flow_step\":-1,";
    json += "\"data\":{";
    json += "\"status\":\"stopped\",";
    json += "\"message\":\"Flow stopped\"";
    json += "}";
    json += "}";
    return json;
}
