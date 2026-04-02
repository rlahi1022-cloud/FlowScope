// ------------------------------------------------
// main.cpp
// FlowScope 서버 진입점
// Stage 1: 내부 프로토콜 기반 흐름 구현
// Stage 2: traceid 기반 flow 시각화 포함
//
// 시작 흐름:
//   main → eventbus 구독 등록 →
//   epollserver 초기화 → run()
// ------------------------------------------------

#include "infra/epollserver.h"
#include "eventbus/eventbus.h"
#include "common/logger.h"

#include <csignal>
#include <iostream>

// ------------------------------------------------
// 서버 포트
// ------------------------------------------------
static constexpr int SERVER_PORT = 9000;

// ------------------------------------------------
// 전역 서버 포인터 (시그널 핸들러용)
// ------------------------------------------------
static epollserver* g_server = nullptr;

// ------------------------------------------------
// signal_handler
// SIGINT (Ctrl+C) 수신 시 서버를 정상 종료한다
// ------------------------------------------------
static void signal_handler(int signum)
{
    log_event("main", "종료 시그널 수신 | sig=" + std::to_string(signum));
    if (g_server)
        g_server->stop();
}

// ------------------------------------------------
// register_eventbus_handlers
// eventbus 구독자 등록
// "response" topic: echoservice가 publish한 결과를 수신
// ------------------------------------------------
static void register_eventbus_handlers()
{
    // "response" topic 구독
    // payload 형식: "fd:traceid:json_body"
    eventbus::instance().subscribe("response",
        [](const std::string& payload)
        {
            // payload 파싱: fd:traceid:json_body
            size_t pos1 = payload.find(':');
            size_t pos2 = payload.find(':', pos1 + 1);

            if (pos1 == std::string::npos || pos2 == std::string::npos)
            {
                log_event("main", "response payload 파싱 실패");
                return;
            }

            std::string fdstr   = payload.substr(0, pos1);
            std::string traceid = payload.substr(pos1 + 1, pos2 - pos1 - 1);
            std::string body    = payload.substr(pos2 + 1);

            log_event("main",
                      "response 수신 | fd=" + fdstr +
                      " | body=" + body,
                      traceid);
            log_flow(traceid, "eventbus", "main(response handler)",
                     "fd=" + fdstr);

            // ------------------------------------------------
            // Stage 1: 내부 흐름 검증 단계
            // 실제 클라이언트 응답 송신은
            // 클라이언트 연결 이후 구현 예정
            // 현재는 로그로 결과 확인
            // ------------------------------------------------
            log_event("main",
                      "★ 처리 완료 | fd=" + fdstr +
                      " | 응답=" + body,
                      traceid);
        });

    log_event("main", "eventbus 구독 등록 완료 | topic=response");
}

// ------------------------------------------------
// internal_test
// Stage 1: 클라이언트 없이 내부 요청으로 흐름 검증
// router.route()를 직접 호출하여 전체 흐름을 테스트
// ------------------------------------------------
static void internal_test()
{
    log_event("main", "========== Stage 1 내부 흐름 테스트 시작 ==========");

    // 라우터 직접 생성 후 내부 요청 전송
    router testrouterobj;

    // 테스트 케이스 1: echo 요청
    log_event("main", "테스트 1: echo 요청");
    testrouterobj.route(0, R"({"cmd":"echo","data":{"msg":"hello flowscope"}})");

    // 테스트 케이스 2: ping 요청
    log_event("main", "테스트 2: ping 요청");
    testrouterobj.route(0, R"({"cmd":"ping"})");

    // 테스트 케이스 3: unknown 요청
    log_event("main", "테스트 3: unknown 요청");
    testrouterobj.route(0, R"({"cmd":"unknown_cmd","data":{}})");

    log_event("main", "========== Stage 1 내부 흐름 테스트 완료 ==========");
}

// ------------------------------------------------
// main
// 1. eventbus 구독 등록
// 2. Stage 1 내부 흐름 테스트
// 3. 실제 epoll 서버 시작
// ------------------------------------------------
int main()
{
    log_event("main", "FlowScope 서버 시작");

    // eventbus 구독자 등록
    register_eventbus_handlers();

    // Stage 1: 내부 프로토콜 흐름 검증 (클라이언트 없이)
    internal_test();

    // 시그널 핸들러 등록 (정상 종료)
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // epoll 서버 시작
    epollserver server(SERVER_PORT);
    g_server = &server;

    log_event("main",
              "epoll 서버 시작 | port=" + std::to_string(SERVER_PORT));
    server.run();

    log_event("main", "FlowScope 서버 종료");
    return 0;
}