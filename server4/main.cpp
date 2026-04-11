// ------------------------------------------------
// main.cpp
// server4 엔트리 포인트
//
// 흐름:
// 1. eventbus4, jobqueue4, workerpool4 초기화
// 2. "response" 토픽 구독자 등록
//    - eventbus4.publish("response", "fd:traceid:json") 수신
//    - fd로 응답 전송
// 3. epollserver4 시작 및 실행
// 4. graceful shutdown: 워커 중지 → jobqueue shutdown
// ------------------------------------------------

#include "infra/epollserver4.h"
#include "eventbus/eventbus4.h"
#include "jobqueue/jobqueue4.h"
#include "workerpool/workerpool4.h"
#include "router/router4.h"
#include "common/logger.h"
#include "common/packet.h"

#include <iostream>
#include <memory>
#include <csignal>
#include <atomic>
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>

// ------------------------------------------------
// 전역 서버 인스턴스 및 상태
// ------------------------------------------------
static std::unique_ptr<epollserver4> g_server;
static std::atomic<bool> g_shutdown_requested{false};
jobqueue4* g_jobqueue4_ptr = nullptr;  // router4에서 참조

// ------------------------------------------------
// signal handler
// SIGINT/SIGTERM 처리
// ------------------------------------------------
extern "C" void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        g_shutdown_requested.store(true);
        if (g_server)
        {
            g_server->stop();
        }
    }
}

// ------------------------------------------------
// send_response_to_fd
// fd로 4byte 헤더 + json body 전송
// ------------------------------------------------
void send_response_to_fd(int fd, const std::string& json)
{
    if (fd < 0) return;

    uint32_t bodylen = static_cast<uint32_t>(json.size());

    // 빅엔디언 헤더 구성
    uint8_t header[HEADER_SIZE];
    header[0] = (bodylen >> 24) & 0xFF;
    header[1] = (bodylen >> 16) & 0xFF;
    header[2] = (bodylen >>  8) & 0xFF;
    header[3] = (bodylen      ) & 0xFF;

    std::string pkt;
    pkt.append(reinterpret_cast<char*>(header), HEADER_SIZE);
    pkt.append(json);

    size_t total = pkt.size();
    size_t sent  = 0;

    // 전체 전송 보장 루프
    while (sent < total)
    {
        ssize_t n = write(fd,
                          pkt.data() + sent,
                          total - sent);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            log_event("main",
                      "write 실패 | fd=" + std::to_string(fd) +
                      " | err=" + strerror(errno));
            return;
        }
        sent += static_cast<size_t>(n);
    }

    log_event("main",
              "write 완료 | fd=" + std::to_string(fd) +
              " | bytes=" + std::to_string(total));
}

// ------------------------------------------------
// response_callback
// eventbus4 "response" 토픽의 구독자 콜백
// 형식: "fd:traceid:json"
// ------------------------------------------------
void response_callback(const std::string& data)
{
    // "fd:traceid:json" 형식 파싱
    size_t first_colon = data.find(':');
    if (first_colon == std::string::npos)
    {
        log_event("main", "response 파싱 실패 (첫 번째 colon 없음)");
        return;
    }

    size_t second_colon = data.find(':', first_colon + 1);
    if (second_colon == std::string::npos)
    {
        log_event("main", "response 파싱 실패 (두 번째 colon 없음)");
        return;
    }

    std::string fd_str = data.substr(0, first_colon);
    std::string traceid = data.substr(first_colon + 1, second_colon - first_colon - 1);
    std::string json = data.substr(second_colon + 1);

    int fd = std::stoi(fd_str);

    log_event("main",
              "response 콜백 | fd=" + std::to_string(fd),
              "traceid=" + traceid, traceid);
    log_flow(traceid, "eventbus4", "main (send_response_to_fd)",
             "fd=" + std::to_string(fd));

    // fd로 응답 전송
    send_response_to_fd(fd, json);
}

// ------------------------------------------------
// async_job_handler
// workerpool4의 Worker가 job 처리할 함수
// job을 분석해 서비스 호출 후 eventbus4에 publish
// ------------------------------------------------
void async_job_handler(const job4& job)
{
    log_event("main",
              "async_job_handler 호출 | fd=" + std::to_string(job.fd),
              "jobsize=" + std::to_string(job.jsonbody.size()));

    // 여기서는 flow_* 비동기 작업 처리
    // 현재는 간단한 응답만 생성 (실제로는 복잡한 처리)

    // cmd 파싱
    std::string key = "\"cmd\"";
    size_t pos = job.jsonbody.find(key);
    std::string cmd = "";
    if (pos != std::string::npos)
    {
        pos = job.jsonbody.find(':', pos + key.size());
        if (pos != std::string::npos)
        {
            ++pos;
            while (pos < job.jsonbody.size() && job.jsonbody[pos] == ' ') ++pos;
            if (pos < job.jsonbody.size() && job.jsonbody[pos] == '"')
            {
                ++pos;
                size_t end = job.jsonbody.find('"', pos);
                if (end != std::string::npos)
                {
                    cmd = job.jsonbody.substr(pos, end - pos);
                }
            }
        }
    }

    log_event("main",
              "async_job_handler 처리 | cmd=" + cmd,
              "", "");

    // 응답 JSON 구성 (간단한 예제)
    std::string response;
    response += "{";
    response += "\"cmd\":\"" + cmd + "_response\",";
    response += "\"server\":\"server4\",";
    response += "\"data\":{\"status\":\"async_processed\"}";
    response += "}";

    // eventbus4의 "response" 토픽에 publish
    std::string response_data = std::to_string(job.fd) + ":?:" + response;
    eventbus4::instance().publish("response", response_data);
}

// ------------------------------------------------
// main
// ------------------------------------------------
int main(int argc, char* argv[])
{
    std::cout << "================================================\n";
    std::cout << "   FlowScope Server4 - Hybrid TCP Server\n";
    std::cout << "   Port 9004 (epoll + dispatcher + eventbus)\n";
    std::cout << "================================================\n\n";

    try
    {
        // ------------------------------------------------
        // 1. EventBus4 초기화
        // ------------------------------------------------
        log_event("main", "eventbus4 초기화");
        eventbus4& bus = eventbus4::instance();

        // ------------------------------------------------
        // 2. JobQueue4 초기화 (max 1024)
        // ------------------------------------------------
        log_event("main", "jobqueue4 초기화 | maxsize=1024");
        jobqueue4 job_queue(1024);
        g_jobqueue4_ptr = &job_queue;  // router4에서 사용할 수 있도록 포인터 설정

        // ------------------------------------------------
        // 3. WorkerPool4 초기화 (4개 워커)
        // ------------------------------------------------
        log_event("main", "workerpool4 초기화 | threadcount=4");
        workerpool4 worker_pool(4, job_queue, async_job_handler);
        worker_pool.start();

        // ------------------------------------------------
        // 4. EventBus4의 "response" 토픽 구독자 등록
        // ------------------------------------------------
        log_event("main", "eventbus4 \"response\" 토픽 구독자 등록");
        bus.subscribe("response", response_callback);

        // ------------------------------------------------
        // 5. Signal handler 등록
        // ------------------------------------------------
        signal(SIGINT,  signal_handler);
        signal(SIGTERM, signal_handler);

        // ------------------------------------------------
        // 6. EPollServer4 생성 및 실행
        // ------------------------------------------------
        log_event("main", "epollserver4 생성 | port=9004");
        g_server = std::make_unique<epollserver4>(9004);

        log_event("main", "epollserver4 실행 시작");
        g_server->run();

        log_event("main", "epollserver4 실행 종료");

        // ------------------------------------------------
        // 7. Graceful shutdown
        // ------------------------------------------------
        log_event("main", "graceful shutdown 시작");

        // worker 정지
        log_event("main", "workerpool4 중지");
        worker_pool.stop();

        // jobqueue shutdown
        log_event("main", "jobqueue4 shutdown");
        job_queue.shutdown();

        log_event("main", "graceful shutdown 완료");

        std::cout << "\n================================================\n";
        std::cout << "   Server4 정상 종료\n";
        std::cout << "================================================\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n ERROR: " << e.what() << "\n";
        return 1;
    }
}
