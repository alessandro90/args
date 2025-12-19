// NOLINTBEGIN(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
#include "include/compiler.hpp"
#include <array>
#include <span>
#include "catch2/catch_test_macros.hpp"

#include "include/tokenizer.hpp"
#include "include/types.hpp"

using args::operator""_flag;
using args::operator""_short_flag;
using namespace args;

TEST_CASE("boolean-short-flag", "[compiler]") {
    static constexpr auto option = Flag{.long_form = "value"_flag, .short_form = "v"_short_flag};
    static constexpr auto rules = Rules<option>{};

    SECTION("providing-value-gets-true") {
        auto const flag = std::array{tokenizer::Token{tokenizer::ShortFlag{.flag = 'v'}}};
        auto const out = compiler::compile(std::span{flag}, rules);

        REQUIRE(out.has_value());
        auto const value = out.value().get<option>();
        REQUIRE(value);  // NOLINT
    }
    SECTION("default-value-is-false") {
        auto const flag = std::array<tokenizer::Token, 0>{};
        auto const out = compiler::compile(std::span{flag}, rules);

        REQUIRE(out.has_value());
        auto const value = out.value().get<option>();
        REQUIRE(!value);
    }
    SECTION("not-providing-a-required-value-is-an-error") {
        static constexpr auto required_option =
            Flag{.long_form = "value"_flag, .short_form = "v"_short_flag, .required = true};
        static constexpr auto required_rules = Rules<required_option>{};
        auto const flag = std::array<tokenizer::Token, 0>{};
        auto const out = compiler::compile(std::span{flag}, required_rules);

        REQUIRE(!out.has_value());
    }
}

// NOLINTEND(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
