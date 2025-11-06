# --- 1. CMake 최소 버전 및 프로젝트 설정 ---
cmake_minimum_required(VERSION 3.20)  # 최신 FetchContent와 호환
project(MismatchMonitor LANGUAGES CXX)

# C++17 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 2. FetchContent 사용 설정 ---
include(FetchContent)

# CMP0135 정책(타임스탬프) 적용
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# --- 3. GoogleTest ---
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

# --- 4. spdlog ---
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

# --- 5. nlohmann/json ---
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# --- 6. Include 경로 설정 ---
set(MIS_MON_INCLUDE_DIRS
        include
        src
)

# --- 7. 메인 실행 파일 ---
add_executable(mis_mon
        src/mis_mon.cpp
        src/application/golden_parameter_monitor.cpp
        src/application/golden_parameter_monitor.h
)

target_include_directories(mis_mon PRIVATE ${MIS_MON_INCLUDE_DIRS})
target_link_libraries(mis_mon PRIVATE spdlog::spdlog nlohmann_json::nlohmann_json)

# --- 8. 테스트 실행 파일 ---
add_executable(runTests
        test/support/test_utils.cpp
)

target_include_directories(runTests PRIVATE ${MIS_MON_INCLUDE_DIRS})
target_link_libraries(runTests PRIVATE gtest_main spdlog::spdlog nlohmann_json::nlohmann_json)

include(GoogleTest)
gtest_discover_tests(runTests)
