#pragma once

// ------------------------------------------------
// packetclass.h
// 서버 전용 패킷 클래스 정의
//
// 책임 범위:
//   - epoll 수신 완료 후 router -> handler -> service 로
//     전달되는 서버 내부 처리 단위
//   - fd 필드 포함 (서버는 다수 클라이언트를 fd 로 구분)
//
// include 위치:
//   server/ 전 계층 (서버 전용)
//
// 관련 파일:
//   공통 프로토콜 enum  -> common/protocol.h
//   공통 context        -> common/context.h
//   클라 패킷 클래스    -> client/common/packetclass.h
// ------------------------------------------------

#include <string>
#include "../../common/protocol.h"   // internal_protocol enum
#include "../../common/context.h"    // requestcontext

// ------------------------------------------------
// class packet
// 서버 계층 간 요청 데이터를 전달하는 단위
//
// fd       : 요청을 보낸 클라이언트 소켓 fd
//            응답 전송 시 이 fd 로 write 한다
//            서버는 다수 클라이언트를 fd 로 구분한다
// ctx      : 요청 context (traceid, cmd, proto, jsonbody)
//            계층 간 전달 시 context 단위로 묶어서 전달한다
// ------------------------------------------------
class packet
{
public:
    int            fd;   // 클라이언트 소켓 fd (서버 전용)
    requestcontext ctx;  // 요청 context

    // ------------------------------------------------
    // 기본 생성자
    // fd 를 -1 로 초기화하여 미할당 상태를 표시한다
    // ------------------------------------------------
    packet()
        : fd(-1)
        , ctx{}
    {}

    // ------------------------------------------------
    // 초기화 생성자
    // epoll 수신 직후 router 에서 생성할 때 사용한다
    // ------------------------------------------------
    packet(int clientfd, const requestcontext& context)
        : fd(clientfd)
        , ctx(context)
    {}
};