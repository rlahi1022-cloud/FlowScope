cmake_minimum_required(VERSION 3.16)
project(flowscope_server3 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------
# Server3: EventBus 기반 epoll 이벤트 루프 서버
# epoll ET + non-blocking I/O + EventBus pub/sub
# 모든 요청/응답이 EventBus를 경유한다
# ------------------------------------------------

set(SOURCES
    main.cpp
    eventbus/eventbus3.cpp
    infra/epollserver3.cpp
    router/router3.cpp
    service/handler/basehandler3.cpp
    service/handler/echohandler3.cpp
    service/handler/uieventhandler3.cpp
    service/echo/echoservice3.cpp
    service/uievent/uieventservice3.cpp
)

add_executable(flowscope_server3 ${SOURCES})

target_include_directories(flowscope_server3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_compile_options(flowscope_server3 PRIVATE
    -Wall -Wextra -Wpedantic
)

find_package(Threads REQUIRED)
target_link_libraries(flowscope_server3 PRIVATE Threads::Threads)
