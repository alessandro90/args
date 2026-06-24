// NOLINTBEGIN(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)

#include "catch2/catch_test_macros.hpp"

#include <array>
#include <cstddef>
#include <format>
#include <numeric>
#include <ranges>
#include <string>
#include "args/validators.hpp"

using namespace args;

namespace {
template <int K>
constexpr auto sum_validator = Validator{
    .fn = [](std::ranges::range auto const &container) -> bool {
        return std::accumulate(std::ranges::begin(container), std::ranges::end(container), 0) >= K;
    },
    .err_fn = [](auto const &) -> std::string {
        return std::format("Sum of elements must be at least {}", K);
    }};

template <std::size_t M>
constexpr auto mult = ValidatorTransformer{
    .fn = [](std::size_t x) -> std::size_t {
        return x * M;
    },
    .err_fn = [](std::string msg) -> std::string {
        return msg;
    }};
}  // namespace

TEST_CASE("less_than", "[validators]") {
    REQUIRE(less_than<10>.fn(9));
    REQUIRE(!less_than<10>.fn(10));
}

TEST_CASE("greater_than", "[validators]") {
    REQUIRE(greater_than<10>.fn(11));
    REQUIRE(!greater_than<10>.fn(10));
}

TEST_CASE("equal_to", "[validators]") {
    REQUIRE(equal_to<10>.fn(10));
    REQUIRE(!equal_to<10>.fn(9));
    REQUIRE(!equal_to<10>.fn(11));
}

TEST_CASE("choices", "[validators]") {
    constexpr auto cs = choices<0, 2, 5>;

    REQUIRE(cs.fn(0));
    REQUIRE(cs.fn(2));
    REQUIRE(cs.fn(5));

    REQUIRE(!cs.fn(1));
    REQUIRE(!cs.fn(3));
    REQUIRE(!cs.fn(4));
}

TEST_CASE("All", "[validators]") {
    constexpr auto all = All<greater_than<0>>;

    auto const ok = std::array{1, 2, 3};
    REQUIRE(all.fn(ok));

    auto const ko = std::array{1, 0, 3};
    REQUIRE(!all.fn(ko));
}

TEST_CASE("Or", "[validators]") {
    constexpr auto test = Or<equal_to<0>, greater_than<10>>;

    REQUIRE(test.fn(0));
    REQUIRE(test.fn(11));

    REQUIRE(!test.fn(1));
    REQUIRE(!test.fn(9));
}

TEST_CASE("And", "[validators]") {
    constexpr auto test = And<greater_than<1>, less_than<10>>;

    REQUIRE(test.fn(2));
    REQUIRE(test.fn(9));

    REQUIRE(!test.fn(1));
    REQUIRE(!test.fn(10));
}

TEST_CASE("Not", "[validators]") {
    constexpr auto test = Not<equal_to<0>>;

    REQUIRE(test.fn(1));
    REQUIRE(test.fn(-1));

    REQUIRE(!test.fn(0));
}

TEST_CASE("Xor", "[validators]") {
    constexpr auto test = Xor<All<greater_than<0>>, sum_validator<5>>;

    SECTION("all-greater-than-zero") {
        auto const ok = std::array{1, 1};
        REQUIRE(test.fn(ok));
    }
    SECTION("sum-geq-5") {
        auto const ok = std::array{0, 5};
        REQUIRE(test.fn(ok));
    }
    SECTION("all-greater-than-zero-and-sum-geq-5") {
        auto const ko = std::array{1, 5};
        REQUIRE(!test.fn(ko));
    }
    SECTION("not-greater-than-zero-and-sum-not-geq-5") {
        auto const ko = std::array{0, 4};
        REQUIRE(!test.fn(ko));
    }
}

TEST_CASE("Pipe", "[validators]") {
    constexpr auto test = Pipe<len, equal_to<3>>;

    SECTION("len-ok") {
        auto const ok = std::array{1, 2, 3};
        REQUIRE(test.fn(ok));
    }
    SECTION("len-not-ok-low") {
        auto const ok = std::array{1, 2};
        REQUIRE(!test.fn(ok));
    }
    SECTION("len-not-ok-high") {
        auto const ok = std::array{1, 2, 3, 4};
        REQUIRE(!test.fn(ok));
    }
}

TEST_CASE("Compose", "[validators]") {
    constexpr auto test = Pipe<Compose<len, mult<2>>, greater_than<5>>;

    SECTION("len-ok") {
        auto const ok = std::array{1, 2, 3};
        REQUIRE(test.fn(ok));
    }
    SECTION("len-not-ok") {
        auto const ok = std::array{1, 2};
        REQUIRE(!test.fn(ok));
    }
}

// NOLINTEND(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
