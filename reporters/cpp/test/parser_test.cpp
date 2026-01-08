#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "parser.hpp"

using Catch::Matchers::ContainsSubstring;

TEST_CASE("detect GoogleTest from JSON structure", "[parser]") {
    std::string json = R"({
        "testsuites": [{
            "name": "TestSuite",
            "testsuite": [{"name": "Test1"}]
        }]
    })";

    auto framework = tdd_guard::Parser::detect_framework(json);
    CHECK(framework == tdd_guard::Framework::GoogleTest);
}

TEST_CASE("detect Catch2 from JSON structure", "[parser]") {
    std::string json = R"({
        "test-run": {
            "test-cases": [{"name": "Test1"}]
        }
    })";

    auto framework = tdd_guard::Parser::detect_framework(json);
    CHECK(framework == tdd_guard::Framework::Catch2);
}

TEST_CASE("detect unknown framework", "[parser]") {
    std::string json = R"({"unknown": "format"})";

    auto framework = tdd_guard::Parser::detect_framework(json);
    CHECK(framework == tdd_guard::Framework::Unknown);
}

TEST_CASE("extract module name from GoogleTest format", "[parser]") {
    CHECK(tdd_guard::Parser::extract_module("TestSuite.TestName") == "TestSuite");
    CHECK(tdd_guard::Parser::extract_module("TestSuite") == "tests");
}

TEST_CASE("extract simple test name", "[parser]") {
    CHECK(tdd_guard::Parser::extract_simple_name("TestSuite.TestName") == "TestName");
    CHECK(tdd_guard::Parser::extract_simple_name("TestName") == "TestName");
}

TEST_CASE("parse GoogleTest passing test", "[parser][googletest]") {
    std::string json = R"({
        "testsuites": [{
            "name": "MathTest",
            "testsuite": [{
                "name": "Addition",
                "status": "RUN",
                "time": "0.001s"
            }]
        }]
    })";

    tdd_guard::Parser parser;
    REQUIRE(parser.parse(json));

    auto events = parser.events();
    REQUIRE(events.size() == 1);
    CHECK(events[0].name == "Addition");
    CHECK(events[0].full_name == "MathTest.Addition");
    CHECK(events[0].state == tdd_guard::TestEvent::State::Passed);
}

TEST_CASE("parse GoogleTest failing test", "[parser][googletest]") {
    std::string json = R"({
        "testsuites": [{
            "name": "MathTest",
            "testsuite": [{
                "name": "Addition",
                "status": "RUN",
                "failures": [{
                    "message": "Value of: add(1, 1)\n  Actual: 3\nExpected: 2"
                }]
            }]
        }]
    })";

    tdd_guard::Parser parser;
    REQUIRE(parser.parse(json));

    auto events = parser.events();
    REQUIRE(events.size() == 1);
    CHECK(events[0].state == tdd_guard::TestEvent::State::Failed);
    REQUIRE(events[0].error_message().has_value());
    CHECK_THAT(*events[0].error_message(), ContainsSubstring("Actual: 3"));
}

TEST_CASE("parse GoogleTest skipped test", "[parser][googletest]") {
    std::string json = R"({
        "testsuites": [{
            "name": "MathTest",
            "testsuite": [{
                "name": "Disabled",
                "status": "NOTRUN"
            }]
        }]
    })";

    tdd_guard::Parser parser;
    REQUIRE(parser.parse(json));

    auto events = parser.events();
    REQUIRE(events.size() == 1);
    CHECK(events[0].state == tdd_guard::TestEvent::State::Skipped);
}

TEST_CASE("parse Catch2 passing test", "[parser][catch2]") {
    std::string json = R"({
        "version": 1,
        "test-run": {
            "test-cases": [{
                "test-info": {
                    "name": "addition works",
                    "tags": ["math"]
                },
                "runs": [{
                    "path": [{
                        "kind": "assertion",
                        "status": true
                    }]
                }],
                "totals": {
                    "assertions": {"passed": 1, "failed": 0}
                }
            }]
        }
    })";

    tdd_guard::Parser parser;
    REQUIRE(parser.parse(json));

    auto events = parser.events();
    REQUIRE(events.size() == 1);
    CHECK(events[0].name == "addition works");
    CHECK(events[0].state == tdd_guard::TestEvent::State::Passed);
}

TEST_CASE("parse Catch2 failing test", "[parser][catch2]") {
    std::string json = R"({
        "version": 1,
        "test-run": {
            "test-cases": [{
                "test-info": {
                    "name": "subtraction fails",
                    "tags": ["math"]
                },
                "runs": [{
                    "path": [{
                        "kind": "assertion",
                        "status": false,
                        "expression": {
                            "expanded": "3 == 2",
                            "original": "subtract(5, 2) == 2"
                        }
                    }]
                }],
                "totals": {
                    "assertions": {"passed": 0, "failed": 1}
                }
            }]
        }
    })";

    tdd_guard::Parser parser;
    REQUIRE(parser.parse(json));

    auto events = parser.events();
    REQUIRE(events.size() == 1);
    CHECK(events[0].state == tdd_guard::TestEvent::State::Failed);
    REQUIRE(events[0].error_message().has_value());
    CHECK_THAT(*events[0].error_message(), ContainsSubstring("3 == 2"));
}

TEST_CASE("parse Catch2 skipped test", "[parser][catch2]") {
    std::string json = R"({
        "version": 1,
        "test-run": {
            "test-cases": [{
                "test-info": {
                    "name": "skipped test",
                    "tags": ["skip"]
                },
                "runs": [{
                    "path": [{
                        "kind": "section",
                        "name": "skipped test",
                        "path": []
                    }]
                }],
                "totals": {
                    "assertions": {"passed": 0, "failed": 0, "skipped": 1}
                }
            }]
        }
    })";

    tdd_guard::Parser parser;
    REQUIRE(parser.parse(json));

    auto events = parser.events();
    REQUIRE(events.size() == 1);
    CHECK(events[0].state == tdd_guard::TestEvent::State::Skipped);
}

TEST_CASE("parse Catch2 test with section extracts section name", "[parser][catch2]") {
    std::string json = R"({
        "version": 1,
        "test-run": {
            "test-cases": [{
                "test-info": {
                    "name": "Calculator",
                    "tags": ["calculator"]
                },
                "runs": [{
                    "path": [{
                        "kind": "section",
                        "name": "Calculator",
                        "path": [{
                            "kind": "section",
                            "name": "should add numbers correctly",
                            "path": [{
                                "kind": "assertion",
                                "status": true
                            }]
                        }]
                    }]
                }],
                "totals": {
                    "assertions": {"passed": 1, "failed": 0}
                }
            }]
        }
    })";

    tdd_guard::Parser parser;
    REQUIRE(parser.parse(json));

    auto events = parser.events();
    REQUIRE(events.size() == 1);
    CHECK(events[0].name == "should add numbers correctly");
    CHECK(events[0].full_name == "Calculator/should add numbers correctly");
    CHECK(events[0].state == tdd_guard::TestEvent::State::Passed);
}
