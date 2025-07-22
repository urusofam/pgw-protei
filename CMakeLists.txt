cmake_minimum_required(VERSION 3.31)
project(pgw_protei CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
)
FetchContent_MakeAvailable(json spdlog)

add_subdirectory(common)
add_subdirectory(pgw_server)
add_subdirectory(pgw_client)
add_subdirectory(tests)