#include "transformer.hpp"
#include <algorithm>
#include <map>
#include <nlohmann/json.hpp>

namespace tdd_guard {

using json = nlohmann::json;

namespace {

template<typename T>
void set_if_present(json& obj, const char* key, const std::optional<T>& value) {
    if (value.has_value()) {
        obj[key] = *value;
    }
}

auto format_compilation_error(const CompilationError& error) -> TestError {
    std::string location;
    if (error.file.has_value()) {
        location = *error.file;
        if (error.line.has_value()) {
            location += ":" + std::to_string(*error.line);
            if (error.column.has_value()) {
                location += ":" + std::to_string(*error.column);
            }
        }
    }

    return TestError{
        .message = error.message,
        .location = location.empty() ? std::nullopt : std::make_optional(location),
        .code = error.code,
        .help = error.help,
        .note = error.note,
        .expected = std::nullopt,
        .actual = std::nullopt
    };
}

auto state_to_string(TestEvent::State state) -> std::string {
    switch (state) {
        case TestEvent::State::Passed: return "passed";
        case TestEvent::State::Failed: return "failed";
        case TestEvent::State::Skipped: return "skipped";
        case TestEvent::State::Unknown: return "unknown";
    }
    return "unknown";
}

} // anonymous namespace

auto TddGuardOutput::to_json() const -> std::string {
    json output;

    json modules_array = json::array();
    for (const auto& module : test_modules) {
        json module_obj;
        module_obj["moduleId"] = module.module_id;

        json tests_array = json::array();
        for (const auto& test : module.tests) {
            json test_obj;
            test_obj["name"] = test.name;
            test_obj["fullName"] = test.full_name;
            test_obj["state"] = test.state;

            if (!test.errors.empty()) {
                json errors_array = json::array();
                for (const auto& error : test.errors) {
                    json error_obj;
                    error_obj["message"] = error.message;
                    set_if_present(error_obj, "location", error.location);
                    set_if_present(error_obj, "code", error.code);
                    set_if_present(error_obj, "help", error.help);
                    set_if_present(error_obj, "note", error.note);
                    set_if_present(error_obj, "expected", error.expected);
                    set_if_present(error_obj, "actual", error.actual);
                    errors_array.push_back(error_obj);
                }
                test_obj["errors"] = errors_array;
            }

            tests_array.push_back(test_obj);
        }
        module_obj["tests"] = tests_array;
        modules_array.push_back(module_obj);
    }

    output["testModules"] = modules_array;
    if (reason.has_value()) {
        output["reason"] = *reason;
    }

    return output.dump();
}

auto transform_events(
    const std::vector<TestEvent>& events,
    const std::vector<CompilationError>& compilation_errors
) -> TddGuardOutput {
    std::map<std::string, TestModule> modules;
    bool has_failure = false;

    if (!compilation_errors.empty()) {
        auto& module = modules["compilation"];
        module.module_id = "compilation";

        std::vector<TestError> errors;
        for (const auto& error : compilation_errors) {
            errors.push_back(format_compilation_error(error));
        }

        module.tests.push_back(TestResult{
            .name = "build",
            .full_name = "compilation::build",
            .state = "failed",
            .errors = std::move(errors)
        });
        has_failure = true;
    }

    for (const auto& event : events) {
        std::string module_name = Parser::extract_module(event.full_name);
        auto& module = modules[module_name];
        module.module_id = module_name;

        std::string state = state_to_string(event.state);
        if (state == "failed") has_failure = true;

        std::vector<TestError> errors;
        if (event.state == TestEvent::State::Failed) {
            auto error_msg = event.error_message();
            if (error_msg.has_value()) {
                errors.push_back(TestError{.message = *error_msg});
            }
        }

        module.tests.push_back(TestResult{
            .name = Parser::extract_simple_name(event.full_name),
            .full_name = event.full_name,
            .state = state,
            .errors = std::move(errors)
        });
    }

    std::vector<TestModule> sorted_modules;
    for (auto& [_, module] : modules) {
        sorted_modules.push_back(std::move(module));
    }
    std::sort(sorted_modules.begin(), sorted_modules.end(),
              [](const auto& a, const auto& b) {
                  return a.module_id < b.module_id;
              });

    std::string reason_str = has_failure ? "failed" : "passed";

    return TddGuardOutput{
        .test_modules = std::move(sorted_modules),
        .reason = reason_str
    };
}

} // namespace tdd_guard
