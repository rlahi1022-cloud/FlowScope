#pragma once

// ------------------------------------------------
// packetclass.h (server3 전용)
// eventbus 기반 서버 내부 패킷 클래스
//
// 역할:
//   - epoll 이벤트 루프 서버 내부 패킷 클래스
//   - fd + requestcontext 포함
//   - EventBus pub/sub 경유로 전달됨
//
// server3 특징:
//   - EventBus 사용 → router → eventbus.publish → handler → service → eventbus.publish
//   - fd는 응답 송신 시 사용 (eventbus "response" 토픽 구독자가 처리)
// ------------------------------------------------

#include <string>
#include "protocol.h"
#include "context.h"

// ------------------------------------------------
// packet3
// server3 계층 간 요청 전달 단위
// ------------------------------------------------
class packet3
{
public:
    int            fd;   // 클라이언트 소켓 fd
    requestcontext ctx;  // 요청 context (traceid, cmd, proto, jsonbody)

    // 기본 생성자: fd = -1 (미할당)
    packet3()
        : fd(-1)
        , ctx{}
    {}

    // 초기화 생성자: epoll 수신 직후 router에서 사용
    packet3(int clientfd, const requestcontext& context)
        : fd(clientfd)
        , ctx(context)
    {}
};
