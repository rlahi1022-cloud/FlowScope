// ------------------------------------------------
// eventbus.cpp
// eventbus 구현부
// subscribe / publish 멀티스레드 안전 처리
// ------------------------------------------------

#include "eventbus/eventbus.h"
#include "common/logger.h"

// ------------------------------------------------
// instance
// 싱글턴 인스턴스 반환 (정적 지역 변수 방식)
// C++11 이후 스레드 안전 초기화 보장
// ------------------------------------------------
eventbus& eventbus::instance()
{
    static eventbus bus;
    return bus;
}

// ------------------------------------------------
// subscribe
// topic에 콜백 함수를 등록한다
// lock_guard로 멀티스레드 동시 등록 보호
// ------------------------------------------------
void eventbus::subscribe(const std::string& topic, handler_fn fn)
{
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_[topic].push_back(fn);

    log_event("eventbus",
              "구독 등록 | topic=" + topic);
}

// ------------------------------------------------
// publish
// topic의 구독자 목록을 복사 후 lock 해제
// lock 바깥에서 콜백 호출 (콜백 내부 deadlock 방지)
// ------------------------------------------------
void eventbus::publish(const std::string& topic, const std::string& data)
{
    std::vector<handler_fn> handlers;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscribers_.find(topic);
        if (it == subscribers_.end())
        {
            // 구독자 없음: 이벤트 드롭
            log_event("eventbus",
                      "publish 무시 (구독자 없음) | topic=" + topic);
            return;
        }
        // 구독자 목록 복사 (lock 범위 최소화)
        handlers = it->second;
    }

    log_event("eventbus",
              "publish | topic=" + topic +
              " | 구독자=" + std::to_string(handlers.size()) + "명");

    // lock 밖에서 각 구독자 콜백 호출
    for (auto& fn : handlers)
    {
        fn(data);
    }
}