#include <catch2/catch_test_macros.hpp>
#include <nonexistent_header.hpp>

TEST_CASE("test with import error") {
    NonExistentType value;
    REQUIRE(value.method() == 42);
}
