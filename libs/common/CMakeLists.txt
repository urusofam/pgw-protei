add_library(common_lib
        config.h
        config.cpp
        logger.cpp
        logger.h
        bcd.cpp
        bcd.h
        socket_raii.h
        socket_raii.cpp
)

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

target_include_directories(common_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(common_lib PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog)
