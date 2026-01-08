#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "transformer.hpp"

using Catch::Matchers::ContainsSubstring;

TEST_CASE("transform passing test", "[transformer]") {
    std::vector<tdd_guard::TestEvent> events = {{
        .name = "Addition",
        .full_name = "MathTest.Addition",
        .state = tdd_guard::TestEvent::State::Passed,
        .stdout_output = std::nullopt,
        .stderr_output = std::nullopt,
        .failure_messages = {}
    }};

    auto output = tdd_guard::transform_events(events, {});

    REQUIRE(output.test_modules.size() == 1);
    CHECK(output.test_modules[0].module_id == "MathTest");
    REQUIRE(output.test_modules[0].tests.size() == 1);
    CHECK(output.test_modules[0].tests[0].name == "Addition");
    CHECK(output.test_modules[0].tests[0].state == "passed");
    CHECK(output.reason == "passed");
}

TEST_CASE("transform failing test", "[transformer]") {
    std::vector<tdd_guard::TestEvent> events = {{
        .name = "Subtraction",
        .full_name = "MathTest.Subtraction",
        .state = tdd_guard::TestEvent::State::Failed,
        .stdout_output = std::nullopt,
        .stderr_output = std::nullopt,
        .failure_messages = {"Expected: 2\nActual: 3"}
    }};

    auto output = tdd_guard::transform_events(events, {});

    REQUIRE(output.test_modules.size() == 1);
    CHECK(output.test_modules[0].tests[0].state == "failed");
    CHECK(output.test_modules[0].tests[0].errors.size() == 1);
    CHECK(output.reason == "failed");
}

TEST_CASE("transform compilation errors", "[transformer]") {
    std::vector<tdd_guard::CompilationError> errors = {{
        .code = "E0001",
        .file = "src/main.cpp",
        .line = 10,
        .column = 5,
        .message = "undefined reference to 'foo'",
        .help = std::nullopt,
        .note = std::nullopt
    }};

    auto output = tdd_guard::transform_events({}, errors);

    REQUIRE(output.test_modules.size() == 1);
    CHECK(output.test_modules[0].module_id == "compilation");
    REQUIRE(output.test_modules[0].tests.size() == 1);
    CHECK(output.test_modules[0].tests[0].name == "build");
    CHECK(output.test_modules[0].tests[0].state == "failed");
    CHECK(output.reason == "failed");
}

TEST_CASE("output to_json produces valid format", "[transformer]") {
    std::vector<tdd_guard::TestEvent> events = {{
        .name = "Test1",
        .full_name = "Suite.Test1",
        .state = tdd_guard::TestEvent::State::Passed,
        .stdout_output = std::nullopt,
        .stderr_output = std::nullopt,
        .failure_messages = {}
    }};

    auto output = tdd_guard::transform_events(events, {});
    auto json = output.to_json();

    CHECK_THAT(json, ContainsSubstring("\"testModules\""));
    CHECK_THAT(json, ContainsSubstring("\"moduleId\""));
    CHECK_THAT(json, ContainsSubstring("\"fullName\""));
    CHECK_THAT(json, ContainsSubstring("\"reason\""));
}

TEST_CASE("modules are sorted alphabetically", "[transformer]") {
    std::vector<tdd_guard::TestEvent> events = {
        {.name = "Test1", .full_name = "Zoo.Test1", .state = tdd_guard::TestEvent::State::Passed},
        {.name = "Test2", .full_name = "Alpha.Test2", .state = tdd_guard::TestEvent::State::Passed},
        {.name = "Test3", .full_name = "Middle.Test3", .state = tdd_guard::TestEvent::State::Passed}
    };

    auto output = tdd_guard::transform_events(events, {});

    REQUIRE(output.test_modules.size() == 3);
    CHECK(output.test_modules[0].module_id == "Alpha");
    CHECK(output.test_modules[1].module_id == "Middle");
    CHECK(output.test_modules[2].module_id == "Zoo");
}
