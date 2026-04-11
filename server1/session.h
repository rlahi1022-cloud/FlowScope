#pragma once

// ------------------------------------------------
// session.h
// Thread-per-Connection 세션 정의
//
// 역할:
//   - 클라이언트 1개 연결을 1개 스레드에서 처리
//   - 4byte 헤더 + JSON body 수신
//   - cmd 파싱 → echo/ping 처리 → 응답 전송
// ------------------------------------------------

#include "packet.h"
#include "protocol.h"
#include "logger.h"
#include "traceid.h"
#include <string>

// ------------------------------------------------
// session
// 클라이언트 소켓 하나를 담당하는 처리 단위
// 별도 스레드에서 run() 호출
// ------------------------------------------------
class session
{
public:
    explicit session(int fd);

    // ------------------------------------------------
    // run
    // 스레드 진입점
    // 연결 종료까지 수신 루프 실행
    // ------------------------------------------------
    void run();

private:
    // 4byte 헤더 + body 수신
    bool recv_packet(std::string& out_body);

    // 4byte 헤더 + body 전송
    bool send_packet(const std::string& body);

    // cmd 파싱
    std::string parse_cmd(const std::string& jsonbody);

    // echo 응답 생성
    std::string build_echo_response(const std::string& body,
                                    const std::string& traceid);

    // ping 응답 생성
    std::string build_ping_response(const std::string& traceid);

    // ------------------------------------------------
    // UI 이벤트 응답 생성
    // ------------------------------------------------
    std::string build_ui_response(const std::string& cmd,
                                  const std::string& body,
                                  const std::string& traceid);

    // JSON에서 특정 문자열 필드 추출
    std::string parse_field(const std::string& json,
                            const std::string& field);

    // UI cmd → response cmd 변환
    std::string get_response_cmd(const std::string& cmd);

    int fd_; // 클라이언트 소켓
};