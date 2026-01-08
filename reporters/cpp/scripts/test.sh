#!/bin/bash
# TDD Guard C++ Test Runner
# Runs tests and captures results for TDD Guard validation

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
REPORTER="$BUILD_DIR/tdd-guard-cpp"
TESTS="$BUILD_DIR/tdd-guard-cpp-tests"

# Change to project root for security validation
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Running cmake..."
    cmake -B "$BUILD_DIR" -S "$PROJECT_ROOT"
fi

# Ensure reporter binary exists before piping
if [ ! -x "$REPORTER" ]; then
    echo "Building reporter..."
    cmake --build "$BUILD_DIR" --target tdd-guard-cpp
fi

# Build tests
echo "Building tests..."
cmake --build "$BUILD_DIR" --target tdd-guard-cpp-tests 2>&1 | \
    "$REPORTER" --project-root "$PROJECT_ROOT" --passthrough || true

# Run tests with Catch2 JSON reporter and capture for TDD Guard
echo "Running tests..."
"$TESTS" --reporter json 2>&1 | \
    "$REPORTER" --project-root "$PROJECT_ROOT" --passthrough

echo "Test results saved to .claude/tdd-guard/data/test.json"
