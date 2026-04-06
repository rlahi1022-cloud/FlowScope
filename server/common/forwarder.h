#pragma once

// ------------------------------------------------
// forwarder.h
// 중앙서버 → 하위 서버 TCP 포워딩 컴포넌트
//
// 흐름:
//   router → forwarder.forward(fd, target, jsonbody)
//          → 하위 서버에 TCP 연결
//          → 4byte 헤더 + JSON body 전송
//          → 응답 수신
//          → 클라이언트 fd로 응답 전송
//
// 설계 원칙:
//   - 하위 서버와의 연결은 요청마다 새로 생성 (단순 구조)
//   - 연결 실패 시 에러 응답을 클라이언트에 반환
//   - 모든 송수신은 4byte 빅엔디언 헤더 + JSON body 형식
// ------------------------------------------------

#include "protocol.h"
#include "packet.h"
#include "logger.h"
#include <string>

// ------------------------------------------------
// forwarder
// target_server로 패킷을 포워딩하고
// 응답을 클라이언트 fd로 전달한다
// ------------------------------------------------
class forwarder
{
public:
    // ------------------------------------------------
    // forward
    // client_fd : 응답을 돌려줄 클라이언트 소켓
    // target    : 포워딩 대상 서버
    // jsonbody  : 전달할 JSON 문자열
    // traceid   : 로그 추적용 ID
    // ------------------------------------------------
    void forward(int client_fd,
                 target_server target,
                 const std::string& jsonbody,
                 const std::string& traceid);

private:
    // ------------------------------------------------
    // connect_to
    // 127.0.0.1:port 에 TCP 연결
    // 성공 시 fd 반환, 실패 시 -1 반환
    // ------------------------------------------------
    int connect_to(int port);

    // ------------------------------------------------
    // send_packet
    // fd로 4byte 빅엔디언 헤더 + JSON body 전송
    // ------------------------------------------------
    bool send_packet(int fd, const std::string& body);

    // ------------------------------------------------
    // recv_packet
    // fd에서 4byte 헤더 + body 수신
    // ------------------------------------------------
    bool recv_packet(int fd, std::string& out_body);

    // ------------------------------------------------
    // send_to_client
    // 클라이언트 fd로 응답 전송
    // ------------------------------------------------
    void send_to_client(int client_fd, const std::string& body);

    // ------------------------------------------------
    // make_error_response
    // 포워딩 실패 시 클라이언트에 반환할 에러 JSON 생성
    // ------------------------------------------------
    std::string make_error_response(const std::string& reason,
                                    const std::string& traceid);
};