#pragma once

#include "error_parser.hpp"
#include "parser.hpp"
#include <optional>
#include <string>
#include <vector>

namespace tdd_guard {

struct TestError {
    std::string message;
    std::optional<std::string> location = std::nullopt;
    std::optional<std::string> code = std::nullopt;
    std::optional<std::string> help = std::nullopt;
    std::optional<std::string> note = std::nullopt;
    std::optional<std::string> expected = std::nullopt;
    std::optional<std::string> actual = std::nullopt;
};

struct TestResult {
    std::string name;
    std::string full_name;
    std::string state;
    std::vector<TestError> errors;
};

struct TestModule {
    std::string module_id;
    std::vector<TestResult> tests;
};

struct TddGuardOutput {
    std::vector<TestModule> test_modules;
    std::optional<std::string> reason;

    [[nodiscard]] auto to_json() const -> std::string;
};

[[nodiscard]] auto transform_events(
    const std::vector<TestEvent>& events,
    const std::vector<CompilationError>& compilation_errors
) -> TddGuardOutput;

} // namespace tdd_guard
