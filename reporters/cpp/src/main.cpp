#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include "error_parser.hpp"
#include "parser.hpp"
#include "transformer.hpp"

namespace fs = std::filesystem;

struct Args {
    std::string project_root;
    bool passthrough = false;
};

auto parse_args(int argc, char* argv[]) -> Args {
    Args args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--project-root" && i + 1 < argc) {
            args.project_root = argv[++i];
        } else if (arg == "--passthrough") {
            args.passthrough = true;
        }
    }

    return args;
}

auto save_results(const fs::path& project_root, const tdd_guard::TddGuardOutput& output) -> bool {
    auto output_dir = project_root / ".claude" / "tdd-guard" / "data";

    std::error_code ec;
    fs::create_directories(output_dir, ec);
    if (ec) {
        std::cerr << "Error creating directory: " << ec.message() << "\n";
        return false;
    }

    auto output_file = output_dir / "test.json";
    auto temp_file = output_dir / "test.json.tmp";

    std::ofstream ofs(temp_file);
    if (!ofs) {
        std::cerr << "Error opening temp file for writing\n";
        return false;
    }

    ofs << output.to_json();

    // Check for write errors before closing and renaming
    if (!ofs) {
        std::cerr << "Error writing to temp file\n";
        return false;
    }

    ofs.close();
    if (!ofs) {
        std::cerr << "Error closing temp file\n";
        return false;
    }

    fs::remove(output_file, ec);
    ec.clear();

    fs::rename(temp_file, output_file, ec);
    if (ec) {
        std::cerr << "Error renaming temp file: " << ec.message() << "\n";
        return false;
    }

    return true;
}

auto process_passthrough(const fs::path& project_root) -> int {
    std::vector<std::string> all_lines;
    std::string all_content;
    std::string line;

    while (std::getline(std::cin, line)) {
        std::cout << line << "\n";
        std::cout.flush();
        all_lines.push_back(line);
        all_content += line + "\n";
    }

    tdd_guard::Parser parser;
    std::vector<tdd_guard::TestEvent> events;

    const bool parsed = parser.parse(all_content);
    if (parsed) {
        events = parser.events();
    }

    auto is_json_syntax = [](std::string_view line) {
        auto trimmed = line | std::views::drop_while([](unsigned char c) {
            return std::isspace(c);
        });
        if (trimmed.empty()) return false;
        char first = *trimmed.begin();
        return first == '{' || first == '}' || first == '[' || first == ']' || first == '"';
    };

    std::vector<std::string> stderr_lines;
    for (const auto& line : all_lines) {
        if (!is_json_syntax(line)) {
            stderr_lines.push_back(line);
        }
    }

    auto compilation_errors = tdd_guard::parse_error_buffer(stderr_lines);
    if (!parsed && compilation_errors.empty() && !all_content.empty()) {
        compilation_errors.push_back(tdd_guard::CompilationError{
            .message = "Failed to parse test output",
            .note = "No JSON test output detected"
        });
    }

    auto output = tdd_guard::transform_events(events, compilation_errors);

    if (!save_results(project_root, output)) {
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    auto args = parse_args(argc, argv);

    if (args.project_root.empty()) {
        std::cerr << "Error: --project-root is required\n";
        return 1;
    }

    fs::path project_root(args.project_root);

    if (!project_root.is_absolute()) {
        std::cerr << "Error: project-root must be an absolute path\n";
        return 1;
    }

    if (!fs::exists(project_root)) {
        std::cerr << "Error: project-root does not exist: " << project_root.string() << "\n";
        return 1;
    }

    auto validated_root = fs::canonical(project_root);

    if (args.passthrough) {
        return process_passthrough(validated_root);
    }

    std::cerr << "Error: only --passthrough mode is currently supported\n";
    return 1;
}
