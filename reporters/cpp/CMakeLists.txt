cmake_minimum_required(VERSION 3.25)

project(tdd-guard-cpp
    VERSION 0.1.0
    DESCRIPTION "C++ test reporter for TDD Guard validation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(tdd-guard-cpp
    src/main.cpp
    src/error_parser.cpp
    src/parser.cpp
    src/transformer.cpp
)

target_include_directories(tdd-guard-cpp PRIVATE src)

target_link_libraries(tdd-guard-cpp PRIVATE
    nlohmann_json::nlohmann_json
)

target_compile_options(tdd-guard-cpp PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -Werror -Wno-error=deprecated-declarations>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic -Werror -Wno-error=deprecated-declarations>
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(tdd-guard-cpp PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-O3 -flto>
        $<$<CXX_COMPILER_ID:Clang>:-O3 -flto>
    )
    set_target_properties(tdd-guard-cpp PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
    )
endif()

option(BUILD_TESTING "Build tests" ON)

if(BUILD_TESTING)
    enable_testing()

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(tdd-guard-cpp-tests
        test/main_test.cpp
        test/error_parser_test.cpp
        test/parser_test.cpp
        test/transformer_test.cpp
        src/error_parser.cpp
        src/parser.cpp
        src/transformer.cpp
    )

    target_include_directories(tdd-guard-cpp-tests PRIVATE src)

    target_link_libraries(tdd-guard-cpp-tests PRIVATE
        Catch2::Catch2WithMain
        nlohmann_json::nlohmann_json
    )

    include(CTest)
    include(Catch)
    catch_discover_tests(tdd-guard-cpp-tests)
endif()

install(TARGETS tdd-guard-cpp
    RUNTIME DESTINATION bin
)
