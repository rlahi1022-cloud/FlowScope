#pragma once

// ------------------------------------------------
// jobqueue4.h
// server4 thread-safe Job 큐 정의
//
// 설계 원칙:
//   - epoll 스레드는 절대 blocking 작업 금지
//   - 무거운 작업(flow_*) 은 이 큐를 통해 Worker Thread로 전달
//   - mutex + condition_variable 로 thread-safe 보장
//   - 큐 크기 상한 설정으로 backpressure 처리
//   - overflow 시 새 작업 drop (거부) 처리
// ------------------------------------------------

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <atomic>

// ------------------------------------------------
// job4 구조체
// Worker Thread가 처리할 작업 단위
// ------------------------------------------------
struct job4 {
    int         fd;       // 응답을 돌려줄 클라이언트 소켓 fd
    std::string jsonbody; // 처리할 JSON 문자열 (원본 바디)
};

// ------------------------------------------------
// jobqueue4 클래스
// thread-safe FIFO 큐
// ------------------------------------------------
class jobqueue4 {
public:
    // ------------------------------------------------
    // 생성자
    // maxsize: 큐 최대 크기 (backpressure 기준)
    // ------------------------------------------------
    explicit jobqueue4(std::size_t maxsize);

    // ------------------------------------------------
    // 소멸자
    // shutdown 후 대기 중인 스레드 모두 깨움
    // ------------------------------------------------
    ~jobqueue4();

    // ------------------------------------------------
    // push: 큐에 job을 추가한다
    // 반환값: true = 성공, false = 큐 가득 참 (drop)
    // ------------------------------------------------
    bool push(const job4& j);

    // ------------------------------------------------
    // pop: 큐에서 job을 꺼낸다
    // 큐가 비어있으면 새 job이 들어올 때까지 blocking
    // shutdown 상태이면 false 반환
    // ------------------------------------------------
    bool pop(job4& out);

    // ------------------------------------------------
    // shutdown: 큐를 종료 상태로 전환
    // 이후 push는 false 반환, pop은 즉시 false 반환
    // ------------------------------------------------
    void shutdown();

    // ------------------------------------------------
    // size: 현재 큐에 쌓인 job 수 (근사값)
    // ------------------------------------------------
    std::size_t size() const;

private:
    std::queue<job4>        queue_;       // 실제 job 저장소
    mutable std::mutex      mutex_;       // 큐 접근 보호
    std::condition_variable cond_;        // pop 대기/알림용
    std::size_t             maxsize_;     // 큐 최대 크기
    std::atomic<bool>       shutdown_;    // 종료 플래그
};
