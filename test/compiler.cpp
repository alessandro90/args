// NOLINTBEGIN(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
#include "args/compiler.hpp"
#include <array>
#include <span>
#include <variant>
#include <vector>
#include "args/tokenizer.hpp"
#include "args/types.hpp"
#include "args/typetag.hpp"
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"

using namespace args;
using namespace args::tokenizer;
using namespace args::literals;

TEST_CASE("boolean-short-flag", "[compiler]") {
    static constexpr auto option = Flag{.long_form = "value"_flag, .short_form = "v"_short_flag};
    static constexpr auto rules = Rules<empty, empty, option>{};

    SECTION("providing-value-gets-true") {
        auto const flag = std::array{token_t{ShortFlag{.raw = "-v", .flag = 'v'}}};
        auto const out = compiler::compile(std::span{flag}, rules, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value);
    }
    SECTION("default-value-is-false") {
        auto const flag = std::array<token_t, 0>{};
        auto const out = compiler::compile(std::span{flag}, rules, MutuallyExclusiveGroups<>{});

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
        auto const out =
            compiler::compile(std::span{flag}, required_rules, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Error>(out));
    }
}

TEST_CASE("boolean-short-flag-count", "[compiler]") {
    static constexpr auto option = Flag{.long_form = "value"_flag, .short_form = "v"_short_flag};
    static constexpr auto rules = Rules<empty, empty, option>{};

    SECTION("count-0") {
        auto const flag = std::array<token_t, 0>{};
        auto const out = compiler::compile(std::span{flag}, rules, MutuallyExclusiveGroups<>{});
        REQUIRE(has_args(out));
        REQUIRE(get_args(out).get_with_info<option>().count == 0);
    }
    SECTION("count-1") {
        auto const flag = std::array{token_t{ShortFlag{.raw = "-v", .flag = 'v'}}};
        auto const out = compiler::compile(std::span{flag}, rules, MutuallyExclusiveGroups<>{});
        REQUIRE(has_args(out));
        REQUIRE(get_args(out).get_with_info<option>().count == 1);
    }
    SECTION("count-2") {
        auto const flag = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}},
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}};
        auto const out = compiler::compile(std::span{flag}, rules, MutuallyExclusiveGroups<>{});
        REQUIRE(has_args(out));
        REQUIRE(get_args(out).get_with_info<option>().count == 2);
    }
}

TEST_CASE("short-flag-with-arithmetic-value", "[compiler]") {
    SECTION("providing-value-gets-an-int") {
        static constexpr auto option = FlagWithValue{
            .long_form = "value"_flag, .short_form = "v"_short_flag, .default_value = 0};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}, token_t{Argument{.value = "10"}}};
        auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value == 10);
    }
    SECTION("providing-value-gets-a-float") {
        static constexpr auto option = FlagWithValue{
            .long_form = "value"_flag, .short_form = "v"_short_flag, .default_value = 0.f};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}, token_t{Argument{.value = "10.5"}}};
        auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

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
        auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value == 10);
    }
    SECTION("with-integer-value-not-provided") {
        static constexpr auto option = Positional{.type = tag<int>, .name = "pos-name"_str};
        static constexpr auto rules = Rules<empty, empty, option>{};
        auto const tokens = std::array<token_t, 0>{};
        auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

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
    auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

    REQUIRE(std::holds_alternative<Args<option>>(out));
    auto const &value = std::get<Args<option>>(out).get<option>();
    REQUIRE(value == std::vector{1, 2, 3});
}

TEST_CASE("repeatable-flag", "[compiler]") {
    static constexpr auto option = FlagWithValue{
        .long_form = "value"_flag,
        .short_form = "v"_short_flag,
        .default_value = args::Lazy<std::vector<int>>};
    static constexpr auto rules = Rules<empty, empty, option>{};
    auto const tokens = std::array{
        token_t{tokenizer::ShortFlag{.raw = "-v", .flag = 'v'}},
        token_t{tokenizer::Argument{.value = "1,2,3"}},
        token_t{tokenizer::ShortFlag{.raw = "-v", .flag = 'v'}},
        token_t{tokenizer::Argument{.value = "4"}},
        token_t{tokenizer::LongFlag{.raw = "--value", .flag = "value"}},
        token_t{tokenizer::Argument{.value = "5,6,7"}}};
    auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const &value = get_args(out).get<option>();
    REQUIRE(value == std::vector{1, 2, 3, 4, 5, 6, 7});
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
    auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const value = get_args(out).get<option, suboption>();
    REQUIRE(value == 10);
}

TEST_CASE("subcommand-flagged", "[compiler]") {
    static constexpr auto suboption =
        Positional{.type = tag<int>, .name = "pos-name"_str, .required = true};
    static constexpr auto sub_rules = Rules<empty, empty, suboption>{};
    static constexpr auto option = Subcommand{
        .name = "subcommand-name"_str,
        .help = "help message for value"_str,
        .rules = sub_rules,
        .is_flag = true};
    static constexpr auto rules = Rules<empty, empty, option>{};


    auto const tokens = std::array{
        token_t{LongFlag{.raw = "--subcommand-name", .flag = "subcommand-name"}},
        token_t{Argument{.value = "10"}}};
    auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

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
    auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const value = get_args(out).get_with_info<subcommand, nested_subcommand, argument>().value;
    REQUIRE(value == 10);
}

TEST_CASE("positional", "[compiler]") {
    static constexpr auto option =
        Positional{.type = tag<std::vector<int>>, .name = "pos-name"_str, .variadic = true};
    static constexpr auto rules = Rules<empty, empty, option>{};
    auto const tokens = std::array{
        token_t{Argument{.value = "1"}},
        token_t{Argument{.value = "10"}},
        token_t{Argument{.value = "100"}}};
    auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const value = get_args(out).get<option>();
    REQUIRE(value == std::vector{1, 10, 100});
}

TEST_CASE("forced-positional-variadic", "[compiler]") {
    static constexpr auto dummy_flag = Flag{.long_form = "dummy-flag"_str, .required = true};
    static constexpr auto forced_positionals = Positional{
        .type = tag<std::vector<std::string_view>>,
        .name = "forced-positionals"_str,
        .variadic = true};
    static constexpr auto rules = Rules<empty, empty, dummy_flag, forced_positionals>{};
    auto const tokens = std::array{
        token_t{LongFlag{.raw = "--dummy-flag", .flag = "dummy-flag"}},
        token_t{DoubleDash{}},
        token_t{LongFlag{
            .raw = "--positional-flag-with-value=10",
            .flag = "positional-flag-with-value",
            .has_equal = true}},
        token_t{Argument{.value = "10"}},
        token_t{LongFlag{.raw = "--positional-flag", .flag = "positional-flag"}},
    };
    auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const &args = get_args(out);
    auto const dummy = args.get<dummy_flag>();
    REQUIRE(dummy);

    auto const &positionals = args.get<forced_positionals>();

    REQUIRE(positionals.size() == 2);
    REQUIRE(positionals[0] == "--positional-flag-with-value=10");
    REQUIRE(positionals[1] == "--positional-flag");
}

TEST_CASE("short-flag-with-arithmetic-value-validation", "[compiler]") {
    static constexpr auto option = FlagWithValue{
        .long_form = "value"_flag,
        .short_form = "v"_short_flag,
        .default_value = 0,
        .validator = less_than<10>};
    static constexpr auto rules = Rules<empty, empty, option>{};

    SECTION("validation-correct") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}, token_t{Argument{.value = "9"}}};
        auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value == 9);
    }
    SECTION("validation-incorrect") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}, token_t{Argument{.value = "10"}}};
        auto const out = compiler::compile(std::span{tokens}, rules, MutuallyExclusiveGroups<>{});

        REQUIRE(has_error(out));
    }
}

TEST_CASE("mutually-exclusive-single-group", "[compiler]") {
    static constexpr auto option_a =
        FlagWithValue{.long_form = "option_a"_flag, .default_value = 0};

    static constexpr auto option_b = Flag{.long_form = "option_b"_flag};
    static constexpr auto option_c = Positional{.type = tag<int>, .name = "option_c"_str};
    static constexpr auto rules = Rules<empty, empty, option_a, option_b, option_c>{};

    static constexpr auto mutually_exclusive = MutuallyExclusive<option_a, option_c>{};
    static constexpr auto mutually_exclusive_at_least_one =
        MutuallyExclusive<option_a, option_c>{.at_least_one = true};

    SECTION("ok-case-with-args") {
        auto const tokens = std::array{
            token_t{Argument{.value = "10"}},
            token_t{LongFlag{.raw = "--option_b", .flag = "option_b"}}};

        auto const out = compiler::compile(
            std::span{tokens}, rules, MutuallyExclusiveGroups<mutually_exclusive>{});
        REQUIRE(has_args(out));
    }

    SECTION("ok-case-no-args") {
        auto const tokens = std::array<token_t, 0>{};
        auto const out = compiler::compile(
            std::span{tokens}, rules, MutuallyExclusiveGroups<mutually_exclusive>{});
        REQUIRE(has_args(out));
    }

    SECTION("not-ok-case-because-multiple-args") {
        auto const tokens = std::array{
            token_t{Argument{.value = "10"}},
            token_t{LongFlag{.raw = "--option_a 4", .flag = "option_a"}},
            token_t{Argument{.value = "4"}},
            token_t{LongFlag{.raw = "--option_b", .flag = "option_b"}}};

        auto const out = compiler::compile(
            std::span{tokens}, rules, MutuallyExclusiveGroups<mutually_exclusive>{});
        REQUIRE(has_error(out));
    }

    SECTION("not-ok-case-because-no-args") {
        auto const tokens = std::array<token_t, 0>{};

        auto const out = compiler::compile(
            std::span{tokens}, rules, MutuallyExclusiveGroups<mutually_exclusive_at_least_one>{});
        REQUIRE(has_error(out));
    }
}

TEST_CASE("mutually-exclusive-multiple-groups", "[compiler]") {
    static constexpr auto option_a =
        FlagWithValue{.long_form = "option_a"_flag, .default_value = 0};

    static constexpr auto option_b = Flag{.long_form = "option_b"_flag};
    static constexpr auto option_c = Positional{.type = tag<int>, .name = "option_c"_str};
    static constexpr auto option_d = FlagWithValue{
        .long_form = "option_d"_flag,
        .default_value = 0,
    };
    static constexpr auto rules = Rules<empty, empty, option_a, option_b, option_c, option_d>{};

    static constexpr auto mutually_exclusive_group_0 = MutuallyExclusive<option_a, option_c>{};
    static constexpr auto mutually_exclusive_group_1 = MutuallyExclusive<option_b, option_d>{};

    SECTION("ok-case-with-args-1") {
        auto const tokens = std::array{
            token_t{LongFlag{.raw = "--option_a 2", .flag = "option_a"}},
            token_t{Argument{.value = "2"}},
            token_t{LongFlag{.raw = "--option_b", .flag = "option_b"}}};

        auto const out = compiler::compile(
            std::span{tokens},
            rules,
            MutuallyExclusiveGroups<mutually_exclusive_group_0, mutually_exclusive_group_1>{});
        REQUIRE(has_args(out));
    }
    SECTION("ok-case-with-args-1") {
        auto const tokens = std::array{
            token_t{Argument{.value = "10"}},
            token_t{LongFlag{.raw = "--option_d=10", .flag = "option_d", .has_equal = true}},
            token_t{Argument{.value = "10"}}};

        auto const out = compiler::compile(
            std::span{tokens},
            rules,
            MutuallyExclusiveGroups<mutually_exclusive_group_0, mutually_exclusive_group_1>{});
        REQUIRE(has_args(out));
    }
    SECTION("not-ok-case-with-args-1") {
        auto const tokens = std::array{
            token_t{LongFlag{.raw = "--option_a 2", .flag = "option_a"}},
            token_t{Argument{.value = "2"}},
            token_t{LongFlag{.raw = "--option_b", .flag = "option_b"}},
            token_t{Argument{.value = "8"}},
        };

        auto const out = compiler::compile(
            std::span{tokens},
            rules,
            MutuallyExclusiveGroups<mutually_exclusive_group_0, mutually_exclusive_group_1>{});
        REQUIRE(has_error(out));
    }
    SECTION("not-ok-case-with-args-2") {
        auto const tokens = std::array{
            token_t{LongFlag{.raw = "--option_a 2", .flag = "option_a"}},
            token_t{Argument{.value = "2"}},
            token_t{LongFlag{.raw = "--option_b", .flag = "option_b"}},
            token_t{LongFlag{.raw = "--option_d=10", .flag = "option_d", .has_equal = true}},
            token_t{Argument{.value = "10"}}};

        auto const out = compiler::compile(
            std::span{tokens},
            rules,
            MutuallyExclusiveGroups<mutually_exclusive_group_0, mutually_exclusive_group_1>{});
        REQUIRE(has_error(out));
    }
}

// NOLINTEND(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
