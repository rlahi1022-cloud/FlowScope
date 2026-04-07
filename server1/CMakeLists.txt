cmake_minimum_required(VERSION 3.16)
project(flowscope_server1 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cpp
    tcpserver.cpp
    session.cpp
)

add_executable(flowscope_server1 ${SOURCES})

target_include_directories(flowscope_server1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../server/common
)

target_compile_options(flowscope_server1 PRIVATE
    -Wall -Wextra -Wpedantic
)

find_package(Threads REQUIRED)
target_link_libraries(flowscope_server1 PRIVATE Threads::Threads)