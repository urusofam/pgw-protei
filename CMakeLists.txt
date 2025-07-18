cmake_minimum_required(VERSION 3.31)
project(pgw_protei CXX)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG 3.12.0
)
FetchContent_MakeAvailable(json)

add_subdirectory(common)