#include <catch2/catch_test_macros.hpp>

int add(int a, int b) { return a + b; }

TEST_CASE("Calculator", "[calculator]") {
    SECTION("should add numbers correctly") {
        REQUIRE(add(2, 3) == 6);
    }
}
