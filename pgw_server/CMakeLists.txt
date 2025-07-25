add_executable(pgw_server pgw_server.cpp)

FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
        GIT_TAG v0.23.1
)
FetchContent_MakeAvailable(httplib)

target_link_libraries(pgw_server PRIVATE pgw_core httplib)