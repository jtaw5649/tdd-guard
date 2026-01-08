#include <catch2/catch_test_macros.hpp>
#include "error_parser.hpp"

TEST_CASE("parse GCC error format with full location", "[error_parser]") {
    std::vector<std::string> lines = {
        "src/main.cpp:10:5: error: 'foo' was not declared in this scope"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].file == "src/main.cpp");
    CHECK(errors[0].line == 10);
    CHECK(errors[0].column == 5);
    CHECK(errors[0].message == "'foo' was not declared in this scope");
    CHECK_FALSE(errors[0].code.has_value());
}

TEST_CASE("parse GCC error format without column", "[error_parser]") {
    std::vector<std::string> lines = {
        "src/main.cpp:15: error: expected ';' before '}'"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].file == "src/main.cpp");
    CHECK(errors[0].line == 15);
    CHECK_FALSE(errors[0].column.has_value());
    CHECK(errors[0].message == "expected ';' before '}'");
}

TEST_CASE("parse MSVC error format", "[error_parser]") {
    std::vector<std::string> lines = {
        "main.cpp(42): error C2065: 'undeclared': undeclared identifier"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].file == "main.cpp");
    CHECK(errors[0].line == 42);
    CHECK(errors[0].code == "C2065");
    CHECK(errors[0].message == "'undeclared': undeclared identifier");
}

TEST_CASE("parse simple error without location", "[error_parser]") {
    std::vector<std::string> lines = {
        "error: ld returned 1 exit status"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 1);
    CHECK_FALSE(errors[0].file.has_value());
    CHECK_FALSE(errors[0].line.has_value());
    CHECK(errors[0].message == "ld returned 1 exit status");
}

TEST_CASE("parse multiple errors", "[error_parser]") {
    std::vector<std::string> lines = {
        "src/foo.cpp:5:10: error: 'bar' was not declared in this scope",
        "src/foo.cpp:8:3: error: expected ';' before 'return'"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 2);
    CHECK(errors[0].file == "src/foo.cpp");
    CHECK(errors[0].line == 5);
    CHECK(errors[1].file == "src/foo.cpp");
    CHECK(errors[1].line == 8);
}

TEST_CASE("parse error with note", "[error_parser]") {
    std::vector<std::string> lines = {
        "src/main.cpp:10:5: error: 'vector' is not a member of 'std'",
        "   10 |     std::vector<int> v;",
        "      |     ^~~",
        "note: 'std::vector' is defined in header '<vector>'"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message == "'vector' is not a member of 'std'");
    CHECK(errors[0].note == "'std::vector' is defined in header '<vector>'");
}

TEST_CASE("strip ANSI escape codes", "[error_parser]") {
    std::vector<std::string> lines = {
        "\x1b[1m\x1b[31msrc/main.cpp:10:5: error:\x1b[0m undefined reference to 'foo'"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].file == "src/main.cpp");
    CHECK(errors[0].message == "undefined reference to 'foo'");
}

TEST_CASE("skip boilerplate lines", "[error_parser]") {
    std::vector<std::string> lines = {
        "In file included from src/main.cpp:1:",
        "include/header.hpp:5:10: error: 'missing' was not declared in this scope"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].file == "include/header.hpp");
}

TEST_CASE("fallback when errors cannot be parsed", "[error_parser]") {
    std::vector<std::string> lines = {
        "Some unusual compiler output",
        "error: something went wrong",
        "More output"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message == "something went wrong");
}

TEST_CASE("fallback creates generic error when no structured parse", "[error_parser]") {
    std::vector<std::string> lines = {
        "weird error: format not matching standard patterns",
        "  at some location"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    // Should create fallback since "error:" is present but doesn't match our patterns
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message == "Compilation failed");
    CHECK(errors[0].note.has_value());
}

TEST_CASE("empty input returns empty errors", "[error_parser]") {
    std::vector<std::string> lines = {};

    auto errors = tdd_guard::parse_error_buffer(lines);

    CHECK(errors.empty());
}

TEST_CASE("no error indicators returns empty errors", "[error_parser]") {
    std::vector<std::string> lines = {
        "Compiling src/main.cpp",
        "Linking executable",
        "Build completed successfully"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    CHECK(errors.empty());
}

TEST_CASE("parse GCC fatal error format", "[error_parser]") {
    std::vector<std::string> lines = {
        "/tmp/test.cpp:2:10: fatal error: nonexistent_header.hpp: No such file or directory"
    };

    auto errors = tdd_guard::parse_error_buffer(lines);

    REQUIRE(errors.size() == 1);
    CHECK(errors[0].file == "/tmp/test.cpp");
    CHECK(errors[0].line == 2);
    CHECK(errors[0].column == 10);
    CHECK(errors[0].message == "nonexistent_header.hpp: No such file or directory");
}
