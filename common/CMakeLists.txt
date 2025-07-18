add_library(common_lib
        config.h
        config.cpp
)
target_link_libraries(common_lib PUBLIC nlohmann_json::nlohmann_json)
