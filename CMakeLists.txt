# --- 1. CMake 최소 버전 및 프로젝트 설정 ---
cmake_minimum_required(VERSION 3.2)  # 최신 FetchContent와 호환
project(MismatchMonitor LANGUAGES CXX)

# C++17 설정
set(CMAKE_CXX_STANDARD 20)

# --- 2. FetchContent 사용 설정 ---
include(FetchContent)

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


# --- 7. 메인 실행 파일 ---
add_subdirectory(src)