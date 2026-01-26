// NOLINTBEGIN(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
#include "include/compiler.hpp"
#include <array>
#include <span>
#include <variant>
#include <vector>
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "include/tokenizer.hpp"
#include "include/types.hpp"
#include "include/typetag.hpp"

using namespace args;
using namespace args::tokenizer;

TEST_CASE("boolean-short-flag", "[compiler]") {
    static constexpr auto option = Flag{.long_form = "value"_flag, .short_form = "v"_short_flag};
    static constexpr auto rules = Rules<empty, empty, option>{};

    SECTION("providing-value-gets-true") {
        auto const flag = std::array{token_t{ShortFlag{.flag = 'v'}}};
        auto const out = compiler::compile(std::span{flag}, rules);

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value);
    }
    SECTION("default-value-is-false") {
        auto const flag = std::array<token_t, 0>{};
        auto const out = compiler::compile(std::span{flag}, rules);

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(!value);
    }
    SECTION("not-providing-a-required-value-is-an-error") {
        static constexpr auto required_option =
            Flag{.long_form = "value"_flag, .short_form = "v"_short_flag, .required = true};
        static constexpr auto required_rules =
            Rules<"usage description"_str, empty, required_option>{};
        auto const flag = std::array<token_t, 0>{};
        auto const out = compiler::compile(std::span{flag}, required_rules);

        REQUIRE(std::holds_alternative<Error>(out));
    }
}

TEST_CASE("short-flag-with-arithmetic-value", "[compiler]") {
    SECTION("providing-value-gets-an-int") {
        static constexpr auto option = FlagWithValue{
            .long_form = "value"_flag, .short_form = "v"_short_flag, .default_value = 0};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens =
            std::array{token_t{ShortFlag{.flag = 'v'}}, token_t{Argument{.value = "10"}}};
        auto const out = compiler::compile(std::span{tokens}, rules);

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value == 10);
    }
    SECTION("providing-value-gets-a-float") {
        static constexpr auto option = FlagWithValue{
            .long_form = "value"_flag, .short_form = "v"_short_flag, .default_value = 0.f};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens =
            std::array{token_t{ShortFlag{.flag = 'v'}}, token_t{Argument{.value = "10.5"}}};
        auto const out = compiler::compile(std::span{tokens}, rules);

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE_THAT(static_cast<double>(value), Catch::Matchers::WithinAbsMatcher(10.5, 0.000001));
    }
}

TEST_CASE("positional-with-arithmetic-value", "[compiler]") {
    SECTION("with-integer-value") {
        static constexpr auto option = Positional{.type = tag<int>, .name = "pos-name"_str};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens = std::array{token_t{Argument{.value = "10"}}};
        auto const out = compiler::compile(std::span{tokens}, rules);

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value == 10);
    }
    SECTION("with-integer-value-not-provided") {
        static constexpr auto option = Positional{.type = tag<int>, .name = "pos-name"_str};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens = std::array<token_t, 0>{};
        auto const out = compiler::compile(std::span{tokens}, rules);

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
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
    auto const tokens = std::array<token_t, 0>{};
    auto const out = compiler::compile(std::span{tokens}, rules);

    REQUIRE(std::holds_alternative<Args<option>>(out));
    auto const &value = std::get<Args<option>>(out).get<option>();
    REQUIRE(value == std::vector{1, 2, 3});
}

TEST_CASE("subcommand", "[compiler]") {
    static constexpr auto suboption =
        Positional{.type = tag<int>, .name = "pos-name"_str, .required = true};
    static constexpr auto sub_rules = Rules<empty, empty, suboption>{};
    static constexpr auto option = Subcommand{
        .name = "subcommand-name"_str, .help = "help message for value"_str, .rules = sub_rules};
    static constexpr auto rules = Rules<empty, empty, option>{};


    auto const tokens =
        std::array{token_t{Argument{.value = "subcommand-name"}}, token_t{Argument{.value = "10"}}};
    auto const out = compiler::compile(std::span{tokens}, rules);

    REQUIRE(has_args(out));
    auto const value = get_args(out).get<option, suboption>();
    REQUIRE(value == 10);
}

TEST_CASE("subcommand-nested", "[compiler]") {
    static constexpr auto argument =
        Positional{.type = tag<int>, .name = "pos-name"_str, .required = true};
    static constexpr auto nested_rules = Rules<empty, empty, argument>{};
    static constexpr auto nested_subcommand =
        Subcommand{.name = "nested-command"_str, .rules = nested_rules};
    static constexpr auto subcommand = Subcommand{
        .name = "subcommand-name"_str, .rules = Rules<empty, empty, nested_subcommand>{}};
    static constexpr auto rules = Rules<empty, empty, subcommand>{};

    auto const tokens = std::array{
        token_t{Argument{.value = "subcommand-name"}},
        token_t{Argument{.value = "nested-command"}},
        token_t{Argument{.value = "10"}}};
    auto const out = compiler::compile(std::span{tokens}, rules);

    REQUIRE(has_args(out));
    auto const value = get_args(out).get_with_info<subcommand, nested_subcommand, argument>().value;
    REQUIRE(value == 10);
}

TEST_CASE("positional", "[compiler]") {
    static constexpr auto option =
        Positional{.type = tag<std::vector<int>>, .name = "pos-name"_str, .variadic = true};
    // Positional{.type = tag<vec_t<int>>, .name = "pos-name"_str, .variadic = true};
    static constexpr auto rules = Rules<empty, empty, option>{};
    auto const tokens = std::array{
        token_t{Argument{.value = "1"}},
        token_t{Argument{.value = "10"}},
        token_t{Argument{.value = "100"}}};
    auto const out = compiler::compile(std::span{tokens}, rules);

    REQUIRE(has_args(out));
    auto const value = get_args(out).get<option>();
    REQUIRE(value == std::vector{1, 10, 100});
}

// NOLINTEND(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
