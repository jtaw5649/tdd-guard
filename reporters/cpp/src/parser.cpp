#include "parser.hpp"
#include <nlohmann/json.hpp>

namespace tdd_guard {

using json = nlohmann::json;

auto TestEvent::error_message() const -> std::optional<std::string> {
    std::string msg;

    if (stdout_output.has_value()) {
        msg = *stdout_output;
    }

    if (stderr_output.has_value()) {
        if (!msg.empty()) {
            msg += "\n";
        }
        msg += *stderr_output;
    }

    for (const auto& failure : failure_messages) {
        if (!msg.empty()) {
            msg += "\n";
        }
        msg += failure;
    }

    if (msg.empty()) {
        return std::nullopt;
    }

    return msg;
}

auto Parser::detect_framework(std::string_view json) -> Framework {
    if (json.find("\"testsuites\"") != std::string_view::npos) {
        return Framework::GoogleTest;
    }
    if (json.find("\"test-run\"") != std::string_view::npos ||
        json.find("\"test-cases\"") != std::string_view::npos) {
        return Framework::Catch2;
    }
    return Framework::Unknown;
}

auto Parser::extract_module(std::string_view test_name) -> std::string {
    if (auto dot_pos = test_name.find('.'); dot_pos != std::string_view::npos) {
        return std::string(test_name.substr(0, dot_pos));
    }

    if (auto slash_pos = test_name.find('/'); slash_pos != std::string_view::npos) {
        return std::string(test_name.substr(0, slash_pos));
    }

    return "tests";
}

auto Parser::extract_simple_name(std::string_view test_name) -> std::string {
    if (auto dot_pos = test_name.rfind('.'); dot_pos != std::string_view::npos) {
        return std::string(test_name.substr(dot_pos + 1));
    }

    if (auto slash_pos = test_name.rfind('/'); slash_pos != std::string_view::npos) {
        return std::string(test_name.substr(slash_pos + 1));
    }

    return std::string(test_name);
}

auto Parser::extract_json(std::string_view content) -> std::string_view {
    auto json_start = content.find("\n{");
    if (json_start == std::string_view::npos) {
        json_start = content.find("{");
    } else {
        json_start += 1;
    }

    if (json_start == std::string_view::npos) {
        return {};
    }

    auto json_end = content.rfind('}');
    if (json_end == std::string_view::npos || json_end < json_start) {
        return {};
    }

    return content.substr(json_start, json_end - json_start + 1);
}

auto Parser::parse(std::string_view content) -> bool {
    auto json = extract_json(content);
    if (json.empty()) {
        return false;
    }

    events_.clear();
    detected_framework_ = Framework::Unknown;
    detected_framework_ = detect_framework(json);

    switch (detected_framework_) {
        case Framework::GoogleTest:
            return parse_googletest(json);
        case Framework::Catch2:
            return parse_catch2(json);
        case Framework::Unknown:
            return false;
    }

    return false;
}

auto Parser::events() const -> const std::vector<TestEvent>& {
    return events_;
}

auto Parser::parse_googletest(std::string_view json_str) -> bool {
    try {
        auto data = json::parse(json_str);

        if (!data.contains("testsuites") || !data["testsuites"].is_array()) {
            return false;
        }

        for (const auto& suite : data["testsuites"]) {
            if (!suite.is_object()) {
                continue;
            }
            if (!suite.contains("testsuite") || !suite["testsuite"].is_array()) {
                continue;
            }

            std::string suite_name = suite.value("name", "");

            for (const auto& test : suite["testsuite"]) {
                if (!test.is_object()) {
                    continue;
                }
                TestEvent event;
                event.name = test.value("name", "");
                event.full_name = suite_name.empty()
                    ? event.name
                    : suite_name + "." + event.name;

                std::string status = test.value("status", "");
                if (status == "NOTRUN") {
                    event.state = TestEvent::State::Skipped;
                } else if (test.contains("failures") && test["failures"].is_array() &&
                           !test["failures"].empty()) {
                    event.state = TestEvent::State::Failed;
                    for (const auto& failure : test["failures"]) {
                        if (!failure.is_object()) {
                            continue;
                        }
                        if (failure.contains("message") && failure["message"].is_string()) {
                            event.failure_messages.push_back(failure["message"].get<std::string>());
                        }
                    }
                } else {
                    event.state = TestEvent::State::Passed;
                }

                events_.push_back(std::move(event));
            }
        }

        return true;
    } catch (const json::exception&) {
        return false;
    }
}

auto Parser::parse_catch2(std::string_view json_str) -> bool {
    try {
        auto data = json::parse(json_str);

        if (!data.contains("test-run") || !data["test-run"].is_object()) {
            return false;
        }

        auto& test_run = data["test-run"];
        if (!test_run.contains("test-cases") || !test_run["test-cases"].is_array()) {
            return false;
        }

        for (const auto& test_case : test_run["test-cases"]) {
            if (!test_case.is_object()) {
                continue;
            }
            TestEvent event;

            std::string test_case_name;
            if (test_case.contains("test-info") && test_case["test-info"].is_object()) {
                test_case_name = test_case["test-info"].value("name", "");
            }

            std::vector<std::string> section_names;
            if (test_case.contains("runs") && test_case["runs"].is_array() &&
                !test_case["runs"].empty()) {
                const auto& run = test_case["runs"][0];
                if (!run.is_object()) {
                    continue;
                }
                const json* current_path = run.contains("path") && run["path"].is_array()
                    ? &run["path"]
                    : nullptr;
                while (current_path != nullptr && current_path->is_array()) {
                    const json* next_path = nullptr;
                    for (const auto& path_item : *current_path) {
                        if (!path_item.is_object()) {
                            continue;
                        }
                        if (path_item.value("kind", "") == "section") {
                            section_names.push_back(path_item.value("name", ""));
                            if (path_item.contains("path")) {
                                next_path = &path_item["path"];
                            }
                        }
                    }
                    current_path = next_path;
                }
            }

            if (section_names.empty()) {
                event.name = test_case_name;
                event.full_name = test_case_name;
            } else {
                event.name = section_names.back();
                if (!test_case_name.empty() && section_names.front() != test_case_name) {
                    event.full_name = test_case_name + "/" + section_names[0];
                } else {
                    event.full_name = section_names[0];
                }
                for (size_t i = 1; i < section_names.size(); ++i) {
                    event.full_name += "/" + section_names[i];
                }
            }

            if (test_case.contains("totals") && test_case["totals"].is_object() &&
                test_case["totals"].contains("assertions") &&
                test_case["totals"]["assertions"].is_object()) {
                auto& assertions = test_case["totals"]["assertions"];
                int failed = assertions.value("failed", 0);
                int skipped = assertions.value("skipped", 0);
                int passed = assertions.value("passed", 0);

                if (skipped > 0 && failed == 0 && passed == 0) {
                    event.state = TestEvent::State::Skipped;
                } else if (failed > 0) {
                    event.state = TestEvent::State::Failed;
                } else {
                    event.state = TestEvent::State::Passed;
                }
            } else {
                event.state = TestEvent::State::Unknown;
            }

            if (event.state == TestEvent::State::Failed && test_case.contains("runs") &&
                test_case["runs"].is_array()) {
                for (const auto& run : test_case["runs"]) {
                    if (!run.is_object()) {
                        continue;
                    }
                    if (run.contains("path") && run["path"].is_array()) {
                        for (const auto& path_item : run["path"]) {
                            if (!path_item.is_object()) {
                                continue;
                            }
                            if (path_item.value("kind", "") == "assertion" &&
                                !path_item.value("status", true) &&
                                path_item.contains("expression") &&
                                path_item["expression"].is_object()) {
                                auto& expr = path_item["expression"];
                                std::string expanded = expr.value("expanded", "");
                                if (!expanded.empty()) {
                                    event.failure_messages.push_back(expanded);
                                }
                            }
                        }
                    }
                }
            }

            events_.push_back(std::move(event));
        }

        return true;
    } catch (const json::exception&) {
        return false;
    }
}

} // namespace tdd_guard
