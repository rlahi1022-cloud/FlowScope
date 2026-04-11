// ------------------------------------------------
// workerpool4.cpp
// server4 Worker Thread Pool 구현
// ------------------------------------------------

#include "workerpool/workerpool4.h"
#include "common/logger.h"

// ------------------------------------------------
// 생성자
// threadcount, queue, handler 저장
// 스레드는 start() 호출 시 생성
// ------------------------------------------------
workerpool4::workerpool4(std::size_t threadcount,
                       jobqueue4&   queue,
                       handler_fn  handler)
    : threadcount_(threadcount)
    , queue_(queue)
    , handler_(handler)
    , running_(false)
{
    log_event("workerpool4", "초기화 완료",
              "Worker 수: " + std::to_string(threadcount));
}

// ------------------------------------------------
// 소멸자
// 아직 실행 중이면 stop() 호출
// ------------------------------------------------
workerpool4::~workerpool4()
{
    if (running_.load()) {
        stop();
    }
}

// ------------------------------------------------
// start
// Worker Thread를 threadcount_ 개수만큼 생성
// 각 스레드는 workerloop()를 실행
// ------------------------------------------------
void workerpool4::start()
{
    if (running_.load()) {
        log_event("workerpool4", "start 무시 - 이미 실행 중");
        return;
    }

    running_.store(true);
    threads_.reserve(threadcount_);

    for (std::size_t i = 0; i < threadcount_; ++i) {
        // 각 Worker Thread 생성 및 workerloop 실행
        threads_.emplace_back([this, i]() {
            workerloop(i);
        });
    }

    log_event("workerpool4", "전체 Worker 시작 완료",
              "Worker 수: " + std::to_string(threadcount_));
}

// ------------------------------------------------
// stop
// 실행 상태 해제 후 모든 Worker Thread join
// jobqueue4는 외부에서 shutdown 처리
// ------------------------------------------------
void workerpool4::stop()
{
    if (!running_.load()) {
        return;
    }

    running_.store(false);

    log_event("workerpool4", "stop 요청 - Worker Thread join 대기");

    // 모든 Worker Thread가 종료될 때까지 대기
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }

    threads_.clear();

    log_event("workerpool4", "전체 Worker 종료 완료");
}

// ------------------------------------------------
// threadcount
// 현재 Worker Thread 수 반환
// ------------------------------------------------
std::size_t workerpool4::threadcount() const
{
    return threadcount_;
}

// ------------------------------------------------
// workerloop
// 각 Worker Thread의 실행 본체
//
// 동작:
//   1. jobqueue4.pop()으로 job 대기
//   2. pop 성공 시 handler_(job) 호출
//   3. pop이 false 반환(shutdown) 시 루프 종료
// ------------------------------------------------
void workerpool4::workerloop(std::size_t workerid)
{
    const std::string wid = "worker-" + std::to_string(workerid);

    log_event("workerpool4", "Worker 시작", wid);

    job4 j;

    // jobqueue4.pop()이 false(shutdown)를 반환할 때까지 반복
    while (queue_.pop(j)) {
        log_event("workerpool4", "job 처리 시작",
                  "fd: " + std::to_string(j.fd), wid);

        // 외부에서 주입된 처리 함수 호출
        handler_(j);

        log_event("workerpool4", "job 처리 완료",
                  "fd: " + std::to_string(j.fd), wid);
    }

    log_event("workerpool4", "Worker 종료", wid);
}
