# ================================================
# server4 CMakeLists.txt
# FlowScope Server4 - Hybrid TCP Server
# ================================================

cmake_minimum_required(VERSION 3.10)
project(server4 CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# ================================================
# 인클루드 디렉토리
# ================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# ================================================
# 소스 파일 수집
# ================================================
set(SOURCES
    # Main
    main.cpp

    # Common
    common/logger.h
    common/packet.h
    common/packetclass.h
    common/traceid.h

    # EventBus
    eventbus/eventbus4.h
    eventbus/eventbus4.cpp

    # JobQueue
    jobqueue/jobqueue4.h
    jobqueue/jobqueue4.cpp

    # WorkerPool
    workerpool/workerpool4.h
    workerpool/workerpool4.cpp

    # Dispatcher
    dispatcher/dispatch4.h
    dispatcher/dispatch4.cpp

    # Router
    router/router4.h
    router/router4.cpp

    # Service - Handler - Base
    service/handler/basehandler4.h

    # Service - Handler - Echo
    service/handler/echohandler4.h
    service/handler/echohandler4.cpp

    # Service - Handler - UI Event
    service/handler/uieventhandler4.h
    service/handler/uieventhandler4.cpp

    # Service - Echo
    service/echo/echoservice4.h
    service/echo/echoservice4.cpp

    # Service - UI Event
    service/uievent/uieventservice4.h
    service/uievent/uieventservice4.cpp

    # Infra - EPoll
    infra/epollserver4.h
    infra/epollserver4.cpp
)

# ================================================
# 실행 파일 빌드
# ================================================
add_executable(server4 ${SOURCES})

# ================================================
# 링킹 (pthread 필요)
# ================================================
target_link_libraries(server4 PRIVATE pthread)

# ================================================
# 빌드 메시지
# ================================================
message(STATUS "Server4 CMakeLists.txt 설정 완료")
message(STATUS "  - C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - Build Type: ${CMAKE_BUILD_TYPE}")
