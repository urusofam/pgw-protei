FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

add_executable(common_lib_test common_lib_test.cpp)
target_link_libraries(common_lib_test PRIVATE common_lib gtest gmock)

enable_testing()
add_test(NAME common_lib COMMAND common_lib_test)