cmake_minimum_required(VERSION 3.16)
project(flowscope_server VERSION 1.0.3 LANGUAGES CXX)

# C++17 표준 적용
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 파일 목록
set(SOURCES
    main.cpp
    eventbus/eventbus.cpp
    router/router.cpp
    service/handler/basehandler.cpp
    service/handler/echohandler.cpp
    service/echo/echoservice.cpp
    infra/epollserver.cpp
)

# 실행파일 생성
add_executable(flowscope_server ${SOURCES})

# 헤더 탐색 경로
target_include_directories(flowscope_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# 경고 플래그 설정
target_compile_options(flowscope_server PRIVATE
    -Wall -Wextra -Wpedantic
)

# pthread 링크
find_package(Threads REQUIRED)
target_link_libraries(flowscope_server PRIVATE Threads::Threads)