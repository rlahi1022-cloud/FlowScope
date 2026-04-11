#pragma once

// ------------------------------------------------
// traceid.h (server4 전용)
// 요청 추적 ID 생성 유틸리티
//
// 역할:
//   - 요청마다 고유한 traceid 발급
//   - 로그에서 요청 흐름 전체를 추적하는 데 사용
//   - 형식: TRC-XXXXXXXX (8자리 16진수)
// ------------------------------------------------

#include <string>
#include <atomic>
#include <sstream>
#include <iomanip>

// ------------------------------------------------
// generate_traceid
// 전역 카운터 기반 단조증가 traceid 생성
// thread-safe (atomic)
// ------------------------------------------------
inline std::string generate_traceid()
{
    static std::atomic<uint32_t> counter{0};
    uint32_t id = counter.fetch_add(1, std::memory_order_relaxed);

    std::ostringstream oss;
    oss << "TRC-"
        << std::hex << std::uppercase
        << std::setfill('0') << std::setw(8)
        << id;
    return oss.str();
}
