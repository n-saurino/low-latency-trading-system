cmake_minimum_required(VERSION 3.0.0)
project(low-latency-trading-system VERSION 0.1.0 LANGUAGES C CXX)

find_package(Threads REQUIRED)

include(CTest)
enable_testing()

add_executable(low-latency-trading-system common/logging_example.cpp)

target_link_libraries(low-latency-trading-system Threads::Threads)
target_include_directories(low-latency-trading-system PRIVATE ${CMAKE_SOURCE_DIR}/common)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
