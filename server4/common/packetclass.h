#pragma once

// ------------------------------------------------
// packetclass.h (server4 전용)
// server4 내부 패킷 클래스
//
// 역할:
//   - epoll 수신 완료 후 dispatcher → router → handler → service
//     계층 간 전달되는 요청 단위
//   - fd 포함 (최종 응답 전송 시 이 fd로 eventbus 거쳐 write)
//
// server4 특징:
//   - EventBus 있음 → service가 eventbus에 publish
//   - WorkerPool 있음 → async 요청은 jobqueue → worker
//   - Dispatcher 있음 → sync/async/event 분기
// ------------------------------------------------

#include <string>
#include "../../common/protocol.h"
#include "../../common/context.h"

// ------------------------------------------------
// packet4
// server4 계층 간 요청 전달 단위
// ------------------------------------------------
class packet4
{
public:
    int            fd;   // 클라이언트 소켓 fd
    requestcontext ctx;  // 요청 context (traceid, cmd, proto, jsonbody)

    // 기본 생성자: fd = -1 (미할당)
    packet4()
        : fd(-1)
        , ctx{}
    {}

    // 초기화 생성자: epoll 수신 직후 router에서 사용
    packet4(int clientfd, const requestcontext& context)
        : fd(clientfd)
        , ctx(context)
    {}
};
