cmake_minimum_required(VERSION 3.31)
project(pgw_protei CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

add_subdirectory(libs)
add_subdirectory(pgw_server)
add_subdirectory(pgw_client)
add_subdirectory(tests)