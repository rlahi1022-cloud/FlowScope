#pragma once

// ------------------------------------------------
// packetclass.h
// 클라이언트 전용 패킷 클래스 정의
//
// 책임 범위:
//   - 수신 완료 후 UI 레이어로 전달되는 클라이언트 처리 단위
//   - fd 필드 없음 (클라이언트는 단일 연결)
//
// include 위치:
//   client/ 전 계층 (클라이언트 전용)
//
// 관련 파일:
//   공통 프로토콜 enum  -> common/protocol.h
//   공통 context        -> common/context.h
//   서버 패킷 클래스    -> server/common/packetclass.h
// ------------------------------------------------

#include <string>
#include "../../common/protocol.h"   // internal_protocol enum
#include "../../common/context.h"    // requestcontext

// ------------------------------------------------
// class packet
// 클라이언트 계층 간 수신 데이터를 전달하는 단위
//
// ctx      : 수신 context (traceid, cmd, proto, jsonbody)
//            수신 응답을 UI 레이어로 전달할 때 사용한다
// ------------------------------------------------
class packet
{
public:
    requestcontext ctx;  // 수신 context (fd 없음)

    // ------------------------------------------------
    // 기본 생성자
    // ------------------------------------------------
    packet()
        : ctx{}
    {}

    // ------------------------------------------------
    // 초기화 생성자
    // clientsocket 수신 완료 후 생성할 때 사용한다
    // ------------------------------------------------
    explicit packet(const requestcontext& context)
        : ctx(context)
    {}
};