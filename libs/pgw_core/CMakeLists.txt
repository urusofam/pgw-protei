add_library(pgw_core
        cdr_writer.h
        cdr_writer.cpp
        session_manager.h
        session_manager.cpp
)

target_include_directories(pgw_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(pgw_core PUBLIC common_lib)