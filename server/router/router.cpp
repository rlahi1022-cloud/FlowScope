// ------------------------------------------------
// router.cpp
// router 구현부
// 외부 cmd → internal_protocol 변환 + 핸들러 분기
// ------------------------------------------------

#include "router/router.h"
#include "service/handler/echohandler.h"
#include "common/logger.h"
#include "common/traceid.h"

// ------------------------------------------------
// 생성자
// routemap에 internal_protocol → 핸들러 등록
// echo, ping 모두 echohandler가 처리한다
// ------------------------------------------------
router::router()
{
    // echohandler 하나가 echo, ping 모두 담당
    auto handler = std::make_shared<echohandler>();

    routemap[static_cast<int>(internal_protocol::echo)] = handler;
    routemap[static_cast<int>(internal_protocol::ping)] = handler;

    log_event("router", "라우팅 테이블 초기화 완료 | echo, ping 등록");
}

// ------------------------------------------------
// route
// 1. JSON에서 cmd 파싱
// 2. internal_protocol 변환
// 3. traceid 발급
// 4. routemap에서 핸들러 조회
// 5. packet 생성 후 handler.handle() 호출
// ------------------------------------------------
void router::route(int fd, const std::string& jsonbody)
{
    // cmd 필드 추출
    std::string cmd = parse_cmd(jsonbody);

    // traceid 발급: 이 요청의 전체 흐름 추적에 사용
    std::string traceid = generate_traceid();

    log_event("router",
              "요청 수신 | fd=" + std::to_string(fd) +
              " | cmd=" + cmd,
              traceid);
    log_flow(traceid, "epollserver", "router",
             "cmd=" + cmd);

    // cmd → internal_protocol 변환
    internal_protocol proto = string_to_protocol(cmd);

    if (proto == internal_protocol::unknown)
    {
        log_event("router",
                  "알 수 없는 cmd | 처리 중단 | cmd=" + cmd,
                  traceid);
        return;
    }

    // routemap에서 핸들러 조회
    int key = static_cast<int>(proto);
    auto it = routemap.find(key);
    if (it == routemap.end())
    {
        log_event("router",
                  "핸들러 없음 | proto=" + protocol_to_string(proto),
                  traceid);
        return;
    }

    // packet 구성
    packet pkt;
    pkt.fd       = fd;
    pkt.protocol = proto;
    pkt.jsonbody = jsonbody;
    pkt.traceid  = traceid;

    log_event("router",
              "핸들러 호출 | proto=" + protocol_to_string(proto),
              traceid);
    log_flow(traceid, "router", "echohandler",
             "proto=" + protocol_to_string(proto));

    // 핸들러에 packet 전달
    it->second->handle(pkt);
}

// ------------------------------------------------
// parse_cmd
// JSON 문자열에서 "cmd" 필드 값을 추출한다
// 형식: {"cmd":"echo","data":...}
// 외부 라이브러리 없이 수동 파싱
// ------------------------------------------------
std::string router::parse_cmd(const std::string& jsonbody)
{
    // "cmd" 키 위치 탐색
    std::string key = "\"cmd\"";
    size_t pos = jsonbody.find(key);
    if (pos == std::string::npos)
        return "";

    // ':' 다음 위치로 이동
    pos = jsonbody.find(':', pos + key.size());
    if (pos == std::string::npos)
        return "";

    // 공백 건너뜀
    ++pos;
    while (pos < jsonbody.size() && jsonbody[pos] == ' ')
        ++pos;

    // '"' 시작 확인
    if (pos >= jsonbody.size() || jsonbody[pos] != '"')
        return "";

    ++pos; // 여는 '"' 건너뜀

    // 닫는 '"' 위치 탐색
    size_t end = jsonbody.find('"', pos);
    if (end == std::string::npos)
        return "";

    return jsonbody.substr(pos, end - pos);
}