#include "error_parser.hpp"
#include <algorithm>
#include <ranges>
#include <regex>

namespace tdd_guard {

namespace {

// GCC/Clang format: file.cpp:10:5: error: message (also matches "fatal error:")
const std::regex GCC_ERROR_RE{R"(^(.+?):(\d+):(\d+):\s*(?:fatal\s+)?error:\s*(.+))"};
// GCC/Clang format without column: file.cpp:10: error: message (also matches "fatal error:")
const std::regex GCC_ERROR_NO_COL_RE{R"(^(.+?):(\d+):\s*(?:fatal\s+)?error:\s*(.+))"};
// MSVC format: file.cpp(10): error C2001: message
const std::regex MSVC_ERROR_RE{R"(^(.+?)\((\d+)\):\s*error\s+C(\d+):\s*(.+))"};
// Simple error: error: message (no location)
const std::regex SIMPLE_ERROR_RE{R"(^error:\s*(.+))"};
// Note lines
const std::regex NOTE_RE{R"(^\s*note:\s*(.+))"};
// ANSI escape codes for stripping
const std::regex ANSI_ESCAPE_RE{R"(\x1b\[[0-9;]*m)"};

auto strip_ansi_codes(std::string_view s) -> std::string {
    return std::regex_replace(std::string(s), ANSI_ESCAPE_RE, "");
}

auto append_to_field(std::optional<std::string>& field, std::string_view text) -> void {
    if (field.has_value()) {
        *field += "\n";
        *field += text;
    } else {
        field = std::string(text);
    }
}

auto is_boilerplate(std::string_view line) -> bool {
    return line.find("In file included from") != std::string_view::npos ||
           line.find("In instantiation of") != std::string_view::npos ||
           line.find("required from") != std::string_view::npos;
}

auto has_error_indicator(std::string_view line) -> bool {
    return line.find("error:") != std::string_view::npos ||
           line.find("fatal error:") != std::string_view::npos;
}

auto parse_error_line(std::string_view line) -> std::optional<CompilationError> {
    std::string line_str(line);
    std::smatch match;

    // GCC/Clang format with full location: file.cpp:10:5: error: message
    if (std::regex_search(line_str, match, GCC_ERROR_RE)) {
        return CompilationError{
            .file = match[1].str(),
            .line = static_cast<uint32_t>(std::stoul(match[2].str())),
            .column = static_cast<uint32_t>(std::stoul(match[3].str())),
            .message = match[4].str()
        };
    }

    // GCC/Clang format without column: file.cpp:10: error: message
    if (std::regex_search(line_str, match, GCC_ERROR_NO_COL_RE)) {
        return CompilationError{
            .file = match[1].str(),
            .line = static_cast<uint32_t>(std::stoul(match[2].str())),
            .message = match[3].str()
        };
    }

    // MSVC format: file.cpp(10): error C2001: message
    if (std::regex_search(line_str, match, MSVC_ERROR_RE)) {
        return CompilationError{
            .code = "C" + match[3].str(),
            .file = match[1].str(),
            .line = static_cast<uint32_t>(std::stoul(match[2].str())),
            .message = match[4].str()
        };
    }

    // Simple error format: error: message
    if (std::regex_search(line_str, match, SIMPLE_ERROR_RE)) {
        return CompilationError{.message = match[1].str()};
    }

    return std::nullopt;
}

} // anonymous namespace

auto parse_error_buffer(const std::vector<std::string>& lines)
    -> std::vector<CompilationError> {

    std::vector<std::string> cleaned;
    cleaned.reserve(lines.size());
    for (const auto& line : lines) {
        cleaned.push_back(strip_ansi_codes(line));
    }

    std::vector<CompilationError> errors;
    std::optional<CompilationError> current_error;

    for (const auto& line : cleaned) {
        if (is_boilerplate(line)) {
            continue;
        }

        if (auto error = parse_error_line(line); error.has_value()) {
            if (current_error.has_value()) {
                errors.push_back(std::move(*current_error));
            }
            current_error = std::move(*error);
            continue;
        }

        // Parse note lines and attach to current error
        if (current_error.has_value()) {
            std::smatch match;
            std::string line_str(line);

            if (std::regex_search(line_str, match, NOTE_RE)) {
                append_to_field(current_error->note, match[1].str());
            }
        }
    }

    if (current_error.has_value()) {
        errors.push_back(std::move(*current_error));
    }

    // Fallback: if no structured errors but error indicators exist, create generic error
    if (errors.empty()) {
        bool found_error = std::ranges::any_of(cleaned, has_error_indicator);
        if (found_error) {
            std::string all_output;
            for (const auto& line : cleaned) {
                all_output += line + "\n";
            }
            errors.push_back(CompilationError{
                .message = "Compilation failed",
                .note = all_output
            });
        }
    }

    return errors;
}

} // namespace tdd_guard
