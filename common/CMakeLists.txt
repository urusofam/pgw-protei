add_library(common_lib
        config.h
        config.cpp
        logger.cpp
        logger.h
        bcd.cpp
        bcd.h
)

target_include_directories(common_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(common_lib PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog)
