#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tdd_guard {

enum class Framework {
    GoogleTest,
    Catch2,
    Unknown
};

struct TestEvent {
    enum class State { Passed, Failed, Skipped, Unknown };

    std::string name;
    std::string full_name;
    State state;
    std::optional<std::string> stdout_output;
    std::optional<std::string> stderr_output;
    std::vector<std::string> failure_messages;

    [[nodiscard]] auto error_message() const -> std::optional<std::string>;
};

class Parser {
public:
    static auto detect_framework(std::string_view json) -> Framework;
    static auto extract_module(std::string_view test_name) -> std::string;
    static auto extract_simple_name(std::string_view test_name) -> std::string;

    auto parse(std::string_view json) -> bool;
    [[nodiscard]] auto events() const -> const std::vector<TestEvent>&;

private:
    std::vector<TestEvent> events_;
    Framework detected_framework_ = Framework::Unknown;

    static auto extract_json(std::string_view content) -> std::string_view;
    auto parse_googletest(std::string_view json) -> bool;
    auto parse_catch2(std::string_view json) -> bool;
};

} // namespace tdd_guard
