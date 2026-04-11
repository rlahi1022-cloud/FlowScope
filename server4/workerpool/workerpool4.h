#pragma once

// ------------------------------------------------
// workerpool4.h
// server4 Worker Thread Pool 정의
//
// 설계 원칙:
//   - 고정 개수의 Worker Thread를 생성하고 유지한다
//   - 각 Worker는 jobqueue4에서 job을 꺼내 처리한다
//   - epoll 스레드와 완전히 분리된 별도 스레드에서 동작
//   - job 처리 후 결과를 eventbus4로 publish
//   - 처리 함수는 외부에서 주입 (의존성 역전)
//
// 처리 흐름:
//   jobqueue4.pop() → handler(job4) → eventbus4.publish()
// ------------------------------------------------

#include <thread>
#include <vector>
#include <functional>
#include <string>
#include <atomic>
#include "jobqueue/jobqueue4.h"

// ------------------------------------------------
// workerpool4 클래스
// ------------------------------------------------
class workerpool4 {
public:
    // ------------------------------------------------
    // job 처리 함수 타입
    // 외부에서 실제 처리 로직을 주입한다
    // 인자: job4 (처리할 작업 단위)
    // ------------------------------------------------
    using handler_fn = std::function<void(const job4&)>;

    // ------------------------------------------------
    // 생성자
    // threadcount : 생성할 Worker Thread 수 (고정)
    // queue       : pop할 jobqueue4 참조
    // handler     : job 처리 함수 (외부 주입)
    // ------------------------------------------------
    workerpool4(std::size_t threadcount,
               jobqueue4&   queue,
               handler_fn  handler);

    // ------------------------------------------------
    // 소멸자
    // 모든 Worker Thread join 처리
    // ------------------------------------------------
    ~workerpool4();

    // ------------------------------------------------
    // start: Worker Thread 전체 시작
    // ------------------------------------------------
    void start();

    // ------------------------------------------------
    // stop: Worker Thread 전체 종료
    // jobqueue4 shutdown 후 join
    // ------------------------------------------------
    void stop();

    // ------------------------------------------------
    // threadcount: 현재 Worker Thread 수 반환
    // ------------------------------------------------
    std::size_t threadcount() const;

private:
    // ------------------------------------------------
    // workerloop: 각 Worker Thread의 실행 루프
    // jobqueue4.pop()이 false를 반환할 때까지 반복
    // ------------------------------------------------
    void workerloop(std::size_t workerid);

    std::size_t              threadcount_; // Worker Thread 수
    jobqueue4&                queue_;       // 작업을 꺼낼 큐
    handler_fn               handler_;    // job 처리 함수
    std::vector<std::thread> threads_;    // Worker Thread 목록
    std::atomic<bool>        running_;    // 실행 상태 플래그
};
