// NOLINTBEGIN(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
#include "include/compiler.hpp"
#include <array>
#include <span>
#include <vector>
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "include/tokenizer.hpp"
#include "include/types.hpp"
#include "include/typetag.hpp"

using args::operator""_flag;
using args::operator""_short_flag;
using args::operator""_str;
using namespace args;

TEST_CASE("boolean-short-flag", "[compiler]") {
    static constexpr auto option = Flag{.long_form = "value"_flag, .short_form = "v"_short_flag};
    static constexpr auto rules = Rules<empty, empty, option>{};

    SECTION("providing-value-gets-true") {
        auto const flag = std::array{tokenizer::token_t{tokenizer::ShortFlag{.flag = 'v'}}};
        auto const out = compiler::compile(std::span{flag}, rules);

        REQUIRE(out.has_value());
        auto const value = out.value().get<option>();
        REQUIRE(value);
    }
    SECTION("default-value-is-false") {
        auto const flag = std::array<tokenizer::token_t, 0>{};
        auto const out = compiler::compile(std::span{flag}, rules);

        REQUIRE(out.has_value());
        auto const value = out.value().get<option>();
        REQUIRE(!value);
    }
    SECTION("not-providing-a-required-value-is-an-error") {
        static constexpr auto required_option =
            Flag{.long_form = "value"_flag, .short_form = "v"_short_flag, .required = true};
        static constexpr auto required_rules =
            Rules<"usage description"_str, empty, required_option>{};
        auto const flag = std::array<tokenizer::token_t, 0>{};
        auto const out = compiler::compile(std::span{flag}, required_rules);

        REQUIRE(!out.has_value());
    }
}

TEST_CASE("short-flag-with-arithmetic-value", "[compiler]") {
    SECTION("providing-value-gets-an-int") {
        static constexpr auto option = FlagWithValue{
            .long_form = "value"_flag, .short_form = "v"_short_flag, .default_value = 0};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens = std::array{
            tokenizer::token_t{tokenizer::ShortFlag{.flag = 'v'}},
            tokenizer::token_t{tokenizer::Argument{.value = "10"}}};
        auto const out = compiler::compile(std::span{tokens}, rules);

        REQUIRE(out.has_value());
        auto const value = out.value().get<option>();
        REQUIRE(value == 10);
    }
    SECTION("providing-value-gets-a-float") {
        static constexpr auto option = FlagWithValue{
            .long_form = "value"_flag, .short_form = "v"_short_flag, .default_value = 0.f};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens = std::array{
            tokenizer::token_t{tokenizer::ShortFlag{.flag = 'v'}},
            tokenizer::token_t{tokenizer::Argument{.value = "10.5"}}};
        auto const out = compiler::compile(std::span{tokens}, rules);

        REQUIRE(out.has_value());
        auto const value = out.value().get<option>();
        REQUIRE_THAT(static_cast<double>(value), Catch::Matchers::WithinAbsMatcher(10.5, 0.000001));
    }
}

TEST_CASE("positional-with-arithmetic-value", "[compiler]") {
    SECTION("with-integer-value") {
        static constexpr auto option = Positional{.type = tag<int>, .name = "pos-name"_str};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens = std::array{tokenizer::token_t{tokenizer::Argument{.value = "10"}}};
        auto const out = compiler::compile(std::span{tokens}, rules);

        REQUIRE(out.has_value());
        auto const value = out.value().get<option>();
        REQUIRE(value == 10);
    }
    SECTION("with-integer-value-not-provided") {
        static constexpr auto option = Positional{.type = tag<int>, .name = "pos-name"_str};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens = std::array<tokenizer::token_t, 0>{};
        auto const out = compiler::compile(std::span{tokens}, rules);

        REQUIRE(out.has_value());
        auto const value = out.value().get<option>();
        REQUIRE(value == 0);
    }
}

TEST_CASE("short-flag-with-vec-value-default", "[compiler]") {
    static constexpr auto option = FlagWithValue{
        .long_form = "value"_flag,
        .short_form = "v"_short_flag,
        .default_value = args::Lazy<std::vector<int>, 1, 2, 3>,
        .help = "help message for value"_str};
    static constexpr auto rules = Rules<empty, empty, option>{};
    auto const tokens = std::array<tokenizer::token_t, 0>{};
    auto const out = compiler::compile(std::span{tokens}, rules);

    REQUIRE(out.has_value());
    auto const &value = out.value().get<option>();
    REQUIRE(value == std::vector{1, 2, 3});
}

// NOLINTEND(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
