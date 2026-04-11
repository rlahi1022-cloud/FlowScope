#pragma once

// ------------------------------------------------
// logger.h
// 서버 전역 로그 출력 유틸리티
//
// 출력 형식:
// -------------------------
// [컴포넌트명] HH:MM:SS.mmm
// 이벤트 : (내용)
// 상세   : (detail)       ← 있을 때만 출력
// traceid : (id)          ← 있을 때만 출력
// -------------------------
//
// thread-safe: 전역 mutex로 동시 출력 보호
// ------------------------------------------------

#include <iostream>
#include <string>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>

// ------------------------------------------------
// log_mutex
// 멀티스레드 환경에서 로그 출력 순서 보호
// static local로 단 하나만 생성됨
// ------------------------------------------------
inline std::mutex& log_mutex()
{
    static std::mutex mtx;
    return mtx;
}

// ------------------------------------------------
// current_time
// 현재 시각을 HH:MM:SS.mmm 형식으로 반환
// ------------------------------------------------
inline std::string current_time()
{
    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_r(&t, &tm_buf);
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << tm_buf.tm_hour << ":"
        << std::setw(2) << tm_buf.tm_min  << ":"
        << std::setw(2) << tm_buf.tm_sec  << "."
        << std::setw(3) << ms.count();
    return oss.str();
}

// ------------------------------------------------
// log_event
// 주요 이벤트 로그 출력
//
// component : 컴포넌트 이름  (예: "epoll", "EventBus")
// event     : 이벤트 내용    (예: "publish 완료")
// detail    : 부가 상태 정보 (예: "fd: 1 | topic: request")  선택
// traceid   : 요청 추적 ID   (예: "req-0001")            선택
// ------------------------------------------------
inline void log_event(const std::string& component,
                      const std::string& event,
                      const std::string& detail  = "",
                      const std::string& traceid = "")
{
    std::lock_guard<std::mutex> lock(log_mutex());
    std::cout << "\n-------------------------\n";
    std::cout << "[" << component << "] " << current_time() << "\n";
    std::cout << "이벤트 : " << event << "\n";
    if (!detail.empty())
        std::cout << "상세   : " << detail << "\n";
    if (!traceid.empty())
        std::cout << "traceid : " << traceid << "\n";
    std::cout << "-------------------------\n";
    std::cout.flush();
}

// ------------------------------------------------
// log_flow
// 요청 흐름 추적 전용 로그
// 요청이 어느 컴포넌트에서 어디로 이동하는지 출력
//
// traceid : 요청 추적 ID
// from    : 출발 컴포넌트  (예: "Router")
// to      : 도착 컴포넌트  (예: "EventBus")
// detail  : 부가 정보      (예: "topic: request")  선택
// ------------------------------------------------
inline void log_flow(const std::string& traceid,
                     const std::string& from,
                     const std::string& to,
                     const std::string& detail = "")
{
    std::lock_guard<std::mutex> lock(log_mutex());
    std::cout << "\n[FLOW] " << current_time()
              << " | traceid=" << traceid
              << " | " << from << " → " << to;
    if (!detail.empty())
        std::cout << " | " << detail;
    std::cout << "\n";
    std::cout.flush();
}
