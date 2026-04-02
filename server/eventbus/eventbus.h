#pragma once

// ------------------------------------------------
// eventbus.h
// 서버 내부 pub/sub 이벤트 버스
// Service는 Worker를 직접 호출하지 않고
// 반드시 EventBus를 통해 이벤트를 publish한다
//
// 구조:
//   publisher → eventbus.publish(topic, data)
//             → subscriber callback 호출
// ------------------------------------------------

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>

// ------------------------------------------------
// eventbus
// topic 기반 pub/sub 구조
// subscribe: topic에 콜백 등록
// publish: topic 구독자 전체에게 data 전달
// ------------------------------------------------
class eventbus
{
public:
    // ------------------------------------------------
    // handler_fn
    // 구독자 콜백 함수 타입
    // topic publish 시 data(string) 를 인자로 호출된다
    // ------------------------------------------------
    using handler_fn = std::function<void(const std::string& data)>;

    // ------------------------------------------------
    // instance
    // 싱글턴 인스턴스 반환
    // 서버 전역에서 단일 eventbus를 공유한다
    // ------------------------------------------------
    static eventbus& instance();

    // ------------------------------------------------
    // subscribe
    // 지정 topic에 콜백 함수를 등록한다
    // 동일 topic에 여러 구독자 등록 가능
    // ------------------------------------------------
    void subscribe(const std::string& topic, handler_fn fn);

    // ------------------------------------------------
    // publish
    // 지정 topic의 모든 구독자에게 data를 전달한다
    // 구독자가 없으면 아무 동작도 하지 않는다
    // ------------------------------------------------
    void publish(const std::string& topic, const std::string& data);

private:
    // ------------------------------------------------
    // 생성자/소멸자 private (싱글턴)
    // ------------------------------------------------
    eventbus()  = default;
    ~eventbus() = default;

    // 복사/이동 금지
    eventbus(const eventbus&)            = delete;
    eventbus& operator=(const eventbus&) = delete;

    // topic → 구독자 콜백 목록
    std::unordered_map<std::string, std::vector<handler_fn>> subscribers_;

    // 멀티스레드 안전을 위한 뮤텍스
    std::mutex mutex_;
};