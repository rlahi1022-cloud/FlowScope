// ------------------------------------------------
// jobqueue4.cpp
// server4 thread-safe Job 큐 구현
// ------------------------------------------------

#include "jobqueue/jobqueue4.h"
#include "common/logger.h"

// ------------------------------------------------
// 생성자
// ------------------------------------------------
jobqueue4::jobqueue4(std::size_t maxsize)
    : maxsize_(maxsize)
    , shutdown_(false)
{
    log_event("jobqueue4", "초기화 완료", "최대 크기: " + std::to_string(maxsize));
}

// ------------------------------------------------
// 소멸자
// 아직 shutdown 안 됐으면 자동 shutdown 호출
// ------------------------------------------------
jobqueue4::~jobqueue4()
{
    shutdown();
}

// ------------------------------------------------
// push: 큐에 job 추가
// shutdown 상태이거나 큐가 가득 찼으면 drop
// ------------------------------------------------
bool jobqueue4::push(const job4& j)
{
    std::unique_lock<std::mutex> lock(mutex_);

    // shutdown 상태이면 새 작업 거부
    if (shutdown_.load()) {
        log_event("jobqueue4", "push 거부 - shutdown 상태", "fd: " + std::to_string(j.fd));
        return false;
    }

    // 큐가 최대 크기에 도달했으면 drop (backpressure)
    if (queue_.size() >= maxsize_) {
        log_event("jobqueue4", "push 거부 - 큐 가득 참 (drop)", "fd: " + std::to_string(j.fd)
                  + " | 현재크기: " + std::to_string(queue_.size()));
        return false;
    }

    // job 큐에 삽입
    queue_.push(j);

    log_event("jobqueue4", "push 완료", "fd: " + std::to_string(j.fd)
              + " | 큐크기: " + std::to_string(queue_.size()));

    // 대기 중인 Worker Thread 하나 깨움
    cond_.notify_one();
    return true;
}

// ------------------------------------------------
// pop: 큐에서 job 꺼내기
// 큐가 비어있으면 새 job이 들어올 때까지 대기
// shutdown 상태이고 큐도 비어있으면 false 반환
// ------------------------------------------------
bool jobqueue4::pop(job4& out)
{
    std::unique_lock<std::mutex> lock(mutex_);

    // 큐가 비어있고 shutdown이 아닐 때 대기
    cond_.wait(lock, [this] {
        return !queue_.empty() || shutdown_.load();
    });

    // shutdown 상태이고 큐도 비어있으면 종료 신호
    if (queue_.empty() && shutdown_.load()) {
        return false;
    }

    // 큐 앞에서 job 꺼내기
    out = queue_.front();
    queue_.pop();

    log_event("jobqueue4", "pop 완료", "fd: " + std::to_string(out.fd)
              + " | 남은크기: " + std::to_string(queue_.size()));
    return true;
}

// ------------------------------------------------
// shutdown: 큐 종료 처리
// 대기 중인 모든 Worker Thread를 깨워서 종료시킴
// ------------------------------------------------
void jobqueue4::shutdown()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (shutdown_.load()) return; // 중복 shutdown 방지
        shutdown_.store(true);
        log_event("jobqueue4", "shutdown 요청 처리");
    }
    // 모든 대기 스레드 깨움
    cond_.notify_all();
}

// ------------------------------------------------
// size: 현재 큐 크기 반환 (근사값)
// ------------------------------------------------
std::size_t jobqueue4::size() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
}
