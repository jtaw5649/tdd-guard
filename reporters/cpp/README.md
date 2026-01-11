# TDD Guard C++ Reporter

C++ test reporter that captures test results for TDD Guard validation from GoogleTest and Catch2.

**Note:** This reporter is only available in the [jtaw5649/tdd-guard](https://github.com/jtaw5649/tdd-guard) fork.

## Requirements

- C++26 compatible compiler (GCC 15+ or Clang 19+)
- CMake 3.25+
- TDD Guard installed globally

## Installation

### Step 1: Install TDD Guard

```bash
npm install -g tdd-guard
```

Or using Homebrew:

```bash
brew install tdd-guard
```

### Step 2: Install the C++ reporter

```bash
# Clone and build
git clone https://github.com/jtaw5649/tdd-guard.git
cd tdd-guard/reporters/cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build

# Install to ~/.local/bin (add to PATH if needed)
cmake --install build --prefix ~/.local
```

## Usage

The reporter works as a filter that processes test output while passing it through unchanged.

### With GoogleTest

```bash
./my_tests --gtest_output=json:- 2>&1 | tdd-guard-cpp --project-root /absolute/path/to/project --passthrough
```

### With Catch2

```bash
./my_tests --reporter json 2>&1 | tdd-guard-cpp --project-root /absolute/path/to/project --passthrough
```

The reporter automatically detects the framework from the JSON structure.

## Shell Script Integration

For projects using shell scripts to run tests:

```bash
#!/bin/bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S . -B build
cmake --build build

./build/my_tests --gtest_output=json:- 2>&1 | \
    tee /dev/stderr | \
    tdd-guard-cpp --project-root "$PROJECT_ROOT" --passthrough
```

The `tee /dev/stderr` ensures test output is visible in real-time while also being piped to the reporter.

## Makefile Integration

Add to your `Makefile`:

```makefile
PROJECT_ROOT := $(shell pwd)

.PHONY: test
test:
	cmake -S . -B build && cmake --build build
	./build/my_tests --gtest_output=json:- 2>&1 | tdd-guard-cpp --project-root $(PROJECT_ROOT) --passthrough

.PHONY: test-tdd
test-tdd:
	tdd-guard on && $(MAKE) test
```

## Configuration

### Project Root

The `--project-root` flag must be an absolute path to your project directory. Results are written to `.claude/tdd-guard/data/test.json` by default, or `.codex/tdd-guard/data/test.json` when a `.codex/config.toml` exists at the project root.

### Flags

- `--project-root`: Absolute path to project directory (required)
- `--passthrough`: Force passthrough mode even if stdin is a terminal

## Supported Frameworks

- **GoogleTest** - Parses JSON output from `--gtest_output=json:-`
- **Catch2** - Parses JSON output from `--reporter json`

## How It Works

The reporter:

1. Reads JSON-formatted test output from stdin
2. Passes the output through unchanged to stdout (with `--passthrough`)
3. Parses the JSON to extract test results
4. Saves TDD Guard-formatted results to `.claude/tdd-guard/data/test.json` by default (or `.codex/tdd-guard/data/test.json` when a `.codex/config.toml` exists at the project root)

### Output Format

```json
{
  "testModules": [
    {
      "moduleId": "Calculator",
      "tests": [
        {
          "name": "should add numbers correctly",
          "fullName": "Calculator/should add numbers correctly",
          "state": "passed"
        }
      ]
    }
  ],
  "reason": "Tests run"
}
```

## Development

Build with tests:

```bash
cmake -B build
cmake --build build
```

Run tests:

```bash
./build/tdd-guard-cpp-tests
```

Or use the test script:

```bash
./scripts/test.sh
```

## License

MIT - See LICENSE file in the repository root.
