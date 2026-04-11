#pragma once

// ------------------------------------------------
// packetclass.h (server2 전용)
// epoll 이벤트 루프 서버 내부 패킷 클래스
//
// 역할:
//   - epoll 수신 완료 후 router → handler → service
//     계층 간 전달되는 요청 단위
//   - fd 포함 (응답 전송 시 이 fd로 직접 write)
//
// server2 특징:
//   - EventBus 없음 → service가 직접 fd로 write
//   - WorkerPool 없음 → epoll 스레드에서 즉시 처리
// ------------------------------------------------

#include <string>
#include "protocol.h"
#include "context.h"

// ------------------------------------------------
// packet2
// server2 계층 간 요청 전달 단위
// ------------------------------------------------
class packet2
{
public:
    int            fd;   // 클라이언트 소켓 fd
    requestcontext ctx;  // 요청 context (traceid, cmd, proto, jsonbody)

    // 기본 생성자: fd = -1 (미할당)
    packet2()
        : fd(-1)
        , ctx{}
    {}

    // 초기화 생성자: epoll 수신 직후 router에서 사용
    packet2(int clientfd, const requestcontext& context)
        : fd(clientfd)
        , ctx(context)
    {}
};