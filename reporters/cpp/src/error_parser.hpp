#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tdd_guard {

struct CompilationError {
    std::optional<std::string> code = std::nullopt;
    std::optional<std::string> file = std::nullopt;
    std::optional<uint32_t> line = std::nullopt;
    std::optional<uint32_t> column = std::nullopt;
    std::string message;
    std::optional<std::string> help = std::nullopt;
    std::optional<std::string> note = std::nullopt;
};

[[nodiscard]] auto parse_error_buffer(const std::vector<std::string>& lines)
    -> std::vector<CompilationError>;

} // namespace tdd_guard
