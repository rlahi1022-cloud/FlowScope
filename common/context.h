#pragma once

// ------------------------------------------------
// context.h
// 서버 / 클라이언트 공통 요청 context 정의
//
// 책임 범위:
//   - 요청 하나의 메타 정보를 담는 공통 구조체
//   - traceid, cmd, jsonbody 를 함께 전달하는 단위
//
// include 위치:
//   서버 : router -> handler -> service 계층 간 전달
//   클라 : 수신 응답을 UI 레이어로 전달 시 사용
//
// 관련 파일:
//   프로토콜 enum    -> common/protocol.h
//   네트워크 상수    -> common/packet.h
//   서버 패킷 클래스 -> server/common/packetclass.h
// ------------------------------------------------

#include <string>
#include "protocol.h"   // internal_protocol enum

// ------------------------------------------------
// requestcontext
// 요청 하나의 메타 정보를 계층 간에 전달하는 구조체
//
// traceid  : 요청 추적 ID
//            로그에서 요청 흐름 전체를 추적하는 데 사용한다
// cmd      : 원본 JSON "cmd" 문자열
//            디버그 로그 출력 시 사용한다
// proto    : string_to_protocol() 으로 변환된 내부 enum
//            handler / service 분기 기준으로 사용한다
// jsonbody : 원본 JSON 전체 문자열
//            서비스 레이어에서 필요한 필드를 직접 파싱한다
// ------------------------------------------------
struct requestcontext
{
    std::string       traceid;   // 요청 추적 ID
    std::string       cmd;       // 원본 cmd 문자열 (디버그용)
    internal_protocol proto;     // 내부 프로토콜 enum
    std::string       jsonbody;  // 원본 JSON 바디 전체
};