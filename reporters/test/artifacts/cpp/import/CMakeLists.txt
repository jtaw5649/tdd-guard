cmake_minimum_required(VERSION 3.25)
project(import-test LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(test_runner src/test.cpp)
target_link_libraries(test_runner PRIVATE Catch2::Catch2WithMain)
