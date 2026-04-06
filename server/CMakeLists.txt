cmake_minimum_required(VERSION 3.16)
project(flowscope_server VERSION 1.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cpp
    eventbus/eventbus.cpp
    router/router.cpp
    service/handler/basehandler.cpp
    service/handler/echohandler.cpp
    service/echo/echoservice.cpp
    infra/epollserver.cpp
    common/forwarder.cpp
)

add_executable(flowscope_server ${SOURCES})

target_include_directories(flowscope_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

target_compile_options(flowscope_server PRIVATE
    -Wall -Wextra -Wpedantic
)

find_package(Threads REQUIRED)
target_link_libraries(flowscope_server PRIVATE Threads::Threads)