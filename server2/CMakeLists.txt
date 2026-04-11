cmake_minimum_required(VERSION 3.16)
project(flowscope_server2 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------
# Server2: epoll 이벤트 루프 서버
# epoll ET + non-blocking I/O
# EventBus 없음 / JobQueue 없음 / WorkerPool 없음
# epoll 스레드에서 직접 처리 후 write
# ------------------------------------------------

set(SOURCES
    main.cpp
    infra/epollserver2.cpp
    router/router2.cpp
    service/handler/basehandler2.cpp
    service/handler/echohandler2.cpp
    service/echo/echoservice2.cpp
    service/handler/uieventhandler2.cpp
    service/uievent/uieventservice2.cpp
)

add_executable(flowscope_server2 ${SOURCES})

target_include_directories(flowscope_server2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_compile_options(flowscope_server2 PRIVATE
    -Wall -Wextra -Wpedantic
)

find_package(Threads REQUIRED)
target_link_libraries(flowscope_server2 PRIVATE Threads::Threads)
