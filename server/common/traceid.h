#pragma once

// ------------------------------------------------
// traceid.h
// Stage 2: 요청 추적용 고유 ID 생성기
// 각 요청이 서버 내부를 이동할 때 동일한 traceid를
// 유지하여 흐름을 순서대로 추적할 수 있게 한다
//
// 형식: TRC-00001, TRC-00002, ...
// ------------------------------------------------

#include <atomic>
#include <string>
#include <sstream>
#include <iomanip>

// ------------------------------------------------
// generate_traceid
// 원자적 카운터 기반 고유 traceid 생성
// 멀티스레드 환경에서 중복 없이 발급된다
// ------------------------------------------------
inline std::string generate_traceid()
{
    // 정적 원자 카운터: 프로그램 시작부터 누적 증가
    static std::atomic<uint64_t> counter{1};
    uint64_t id = counter.fetch_add(1, std::memory_order_relaxed);

    std::ostringstream oss;
    oss << "TRC-" << std::setfill('0') << std::setw(5) << id;
    return oss.str();
}