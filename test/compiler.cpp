// NOLINTBEGIN(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
#include "args/compiler.hpp"
#include <array>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>
#include "args/tokenizer.hpp"
#include "args/types.hpp"
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"

using namespace args;
using namespace args::tokenizer;
using namespace args::literals;
using namespace std::string_view_literals;

TEST_CASE("boolean-short-flag", "[compiler]") {
    static constexpr auto option = flag().Long("value").Short('v');
    static constexpr auto opts = Options<empty, empty, option>{};

    SECTION("providing-value-gets-true") {
        auto const flag = std::array{token_t{ShortFlag{.raw = "-v", .flag = 'v'}}};
        auto const out = compiler::compile(std::span{flag}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value);
    }
    SECTION("default-value-is-false") {
        auto const flag = std::array<token_t, 0>{};
        auto const out = compiler::compile(std::span{flag}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(!value);
    }
    SECTION("not-providing-a-required-value-is-an-error") {
        static constexpr auto required_option = flag().Long("value").Short('v').Required(true);
        static constexpr auto required_opts =
            Options<"usage description"_str, empty, required_option>{};
        auto const flag = std::array<token_t, 0>{};
        auto const out =
            compiler::compile(std::span{flag}, required_opts, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Error>(out));
    }
}

TEST_CASE("boolean-short-flag-count", "[compiler]") {
    static constexpr auto option = flag().Long("value").Short('v');
    static constexpr auto opts = Options<empty, empty, option>{};

    SECTION("count-0") {
        auto const flag = std::array<token_t, 0>{};
        auto const out = compiler::compile(std::span{flag}, opts, MutuallyExclusiveGroups<>{});
        REQUIRE(has_args(out));
        REQUIRE(get_args(out).get_with_info<option>().count == 0);
    }
    SECTION("count-1") {
        auto const flag = std::array{token_t{ShortFlag{.raw = "-v", .flag = 'v'}}};
        auto const out = compiler::compile(std::span{flag}, opts, MutuallyExclusiveGroups<>{});
        REQUIRE(has_args(out));
        REQUIRE(get_args(out).get_with_info<option>().count == 1);
    }
    SECTION("count-2") {
        auto const flag = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}},
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}};
        auto const out = compiler::compile(std::span{flag}, opts, MutuallyExclusiveGroups<>{});
        REQUIRE(has_args(out));
        REQUIRE(get_args(out).get_with_info<option>().count == 2);
    }
}

TEST_CASE("short-flag-with-arithmetic-value", "[compiler]") {
    SECTION("providing-value-gets-an-int") {
        static constexpr auto option = flag_with_value<int>().Long("value").Short('v');
        static constexpr auto opts = Options<empty, empty, option>{};
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}, token_t{Argument{.value = "10"}}};
        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value == 10);
    }
    SECTION("providing-value-gets-a-float") {
        static constexpr auto option = flag_with_value<float>().Long("value").Short('v');
        static constexpr auto opts = Options<empty, empty, option>{};
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}, token_t{Argument{.value = "10.5"}}};
        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE_THAT(static_cast<double>(value), Catch::Matchers::WithinAbsMatcher(10.5, 0.000001));
    }
}

TEST_CASE("positional-with-arithmetic-value", "[compiler]") {
    SECTION("with-integer-value") {
        static constexpr auto option = positional<int>().Name("pos-name");
        static constexpr auto opts = Options<empty, empty, option>{};
        auto const tokens = std::array{token_t{Argument{.value = "10"}}};
        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value == 10);
    }
    SECTION("with-integer-value-not-provided") {
        static constexpr auto option = positional<int>().Name("pos-name");
        static constexpr auto opts = Options<empty, empty, option>{};
        auto const tokens = std::array<token_t, 0>{};
        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value == 0);
    }
}

TEST_CASE("short-flag-with-vec-value-default", "[compiler]") {
    static constexpr auto option = flag_with_value<std::vector<int>>()
                                       .Long("value")
                                       .Short('v')
                                       .Default([] {
                                           return std::vector{1, 2, 3};
                                       })
                                       .Help("help message for value");
    static constexpr auto opts = Options<empty, empty, option>{};
    auto const tokens = std::array<token_t, 0>{};
    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(std::holds_alternative<Args<option>>(out));
    auto const &value = std::get<Args<option>>(out).get<option>();
    REQUIRE(value == std::vector{1, 2, 3});
}

TEST_CASE("repeatable-flag", "[compiler]") {
    static constexpr auto option = flag_with_value<std::vector<int>>().Long("value").Short('v');
    static constexpr auto opts = Options<empty, empty, option>{};
    auto const tokens = std::array{
        token_t{tokenizer::ShortFlag{.raw = "-v", .flag = 'v'}},
        token_t{tokenizer::Argument{.value = "1,2,3"}},
        token_t{tokenizer::ShortFlag{.raw = "-v", .flag = 'v'}},
        token_t{tokenizer::Argument{.value = "4"}},
        token_t{tokenizer::LongFlag{.raw = "--value", .flag = "value"}},
        token_t{tokenizer::Argument{.value = "5,6,7"}}};
    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const &value = get_args(out).get<option>();
    REQUIRE(value == std::vector{1, 2, 3, 4, 5, 6, 7});
}

TEST_CASE("subcommand", "[compiler]") {
    static constexpr auto suboption = positional<int>().Name("pos-name").Required(true);
    static constexpr auto sub_opts = Options<empty, empty, suboption>{};
    static constexpr auto option =
        subcommand().Name("subcommand-name").Help("help message for value").Opts(sub_opts);
    static constexpr auto opts = Options<empty, empty, option>{};


    auto const tokens =
        std::array{token_t{Argument{.value = "subcommand-name"}}, token_t{Argument{.value = "10"}}};
    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const value = get_args(out).get<option, suboption>();
    REQUIRE(value == 10);
}

TEST_CASE("subcommand-flagged", "[compiler]") {
    static constexpr auto suboption = positional<int>().Name("pos-name").Required(true);
    static constexpr auto sub_opts = Options<empty, empty, suboption>{};
    static constexpr auto option = subcommand()
                                       .Name("subcommand-name")
                                       .Help("help message for value")
                                       .Flag(true)
                                       .Opts(sub_opts);
    static constexpr auto opts = Options<empty, empty, option>{};


    auto const tokens = std::array{
        token_t{LongFlag{.raw = "--subcommand-name", .flag = "subcommand-name"}},
        token_t{Argument{.value = "10"}}};
    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const value = get_args(out).get<option, suboption>();
    REQUIRE(value == 10);
}

TEST_CASE("subcommand-nested", "[compiler]") {
    static constexpr auto argument = positional<int>().Name("pos-name").Required(true);
    static constexpr auto nested_opts = Options<empty, empty, argument>{};
    static constexpr auto nested_subcommand = subcommand().Name("nested-command").Opts(nested_opts);
    static constexpr auto subc =
        subcommand().Name("subcommand-name").Opts(options<empty, empty, nested_subcommand>);
    static constexpr auto opts = Options<empty, empty, subc>{};

    auto const tokens = std::array{
        token_t{Argument{.value = "subcommand-name"}},
        token_t{Argument{.value = "nested-command"}},
        token_t{Argument{.value = "10"}}};
    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const value = get_args(out).get_with_info<subc, nested_subcommand, argument>().value;
    REQUIRE(value == 10);
}

TEST_CASE("positional", "[compiler]") {
    static constexpr auto option = positional<std::vector<int>>().Name("pos-name").Variadic(true);
    static constexpr auto opts = Options<empty, empty, option>{};
    auto const tokens = std::array{
        token_t{Argument{.value = "1"}},
        token_t{Argument{.value = "10"}},
        token_t{Argument{.value = "100"}}};
    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const value = get_args(out).get<option>();
    REQUIRE(value == std::vector{1, 10, 100});
}

TEST_CASE("forced-positional-variadic", "[compiler]") {
    static constexpr auto dummy_flag = flag().Long("dummy-flag").Required(true);
    static constexpr auto forced_positionals =
        positional<std::vector<std::string_view>>().Name("forced-positionals").Variadic(true);
    static constexpr auto opts = Options<empty, empty, dummy_flag, forced_positionals>{};
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
    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

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
    static constexpr auto option =
        flag_with_value<int>().Long("value").Short('v').Validator(less_than<10>);
    static constexpr auto opts = Options<empty, empty, option>{};

    SECTION("validation-correct") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}, token_t{Argument{.value = "9"}}};
        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(std::holds_alternative<Args<option>>(out));
        auto const value = std::get<Args<option>>(out).get<option>();
        REQUIRE(value == 9);
    }
    SECTION("validation-incorrect") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-v", .flag = 'v'}}, token_t{Argument{.value = "10"}}};
        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_error(out));
    }
}

TEST_CASE("mutually-exclusive-single-group", "[compiler]") {
    static constexpr auto option_a = flag_with_value<int>().Long("option_a");

    static constexpr auto option_b = flag().Long("option_b");
    static constexpr auto option_c = positional<int>().Name("option_c");
    static constexpr auto opts = Options<empty, empty, option_a, option_b, option_c>{};

    static constexpr auto mutually_exclusive = MutuallyExclusive<option_a, option_c>{};
    static constexpr auto mutually_exclusive_at_least_one =
        MutuallyExclusive<option_a, option_c>{.at_least_one = true};

    SECTION("ok-case-with-args") {
        auto const tokens = std::array{
            token_t{Argument{.value = "10"}},
            token_t{LongFlag{.raw = "--option_b", .flag = "option_b"}}};

        auto const out = compiler::compile(
            std::span{tokens}, opts, MutuallyExclusiveGroups<mutually_exclusive>{});
        REQUIRE(has_args(out));
    }

    SECTION("ok-case-no-args") {
        auto const tokens = std::array<token_t, 0>{};
        auto const out = compiler::compile(
            std::span{tokens}, opts, MutuallyExclusiveGroups<mutually_exclusive>{});
        REQUIRE(has_args(out));
    }

    SECTION("not-ok-case-because-multiple-args") {
        auto const tokens = std::array{
            token_t{Argument{.value = "10"}},
            token_t{LongFlag{.raw = "--option_a 4", .flag = "option_a"}},
            token_t{Argument{.value = "4"}},
            token_t{LongFlag{.raw = "--option_b", .flag = "option_b"}}};

        auto const out = compiler::compile(
            std::span{tokens}, opts, MutuallyExclusiveGroups<mutually_exclusive>{});
        REQUIRE(has_error(out));
    }

    SECTION("not-ok-case-because-no-args") {
        auto const tokens = std::array<token_t, 0>{};

        auto const out = compiler::compile(
            std::span{tokens}, opts, MutuallyExclusiveGroups<mutually_exclusive_at_least_one>{});
        REQUIRE(has_error(out));
    }
}

TEST_CASE("mutually-exclusive-multiple-groups", "[compiler]") {
    static constexpr auto option_a = flag_with_value<int>().Long("option_a");

    static constexpr auto option_b = flag().Long("option_b");
    static constexpr auto option_c = positional<int>().Name("option_c");
    static constexpr auto option_d = flag_with_value<int>().Long("option_d");
    static constexpr auto opts = Options<empty, empty, option_a, option_b, option_c, option_d>{};

    static constexpr auto mutually_exclusive_group_0 = MutuallyExclusive<option_a, option_c>{};
    static constexpr auto mutually_exclusive_group_1 = MutuallyExclusive<option_b, option_d>{};

    SECTION("ok-case-with-args-1") {
        auto const tokens = std::array{
            token_t{LongFlag{.raw = "--option_a 2", .flag = "option_a"}},
            token_t{Argument{.value = "2"}},
            token_t{LongFlag{.raw = "--option_b", .flag = "option_b"}}};

        auto const out = compiler::compile(
            std::span{tokens},
            opts,
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
            opts,
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
            opts,
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
            opts,
            MutuallyExclusiveGroups<mutually_exclusive_group_0, mutually_exclusive_group_1>{});
        REQUIRE(has_error(out));
    }
}

TEST_CASE("nargs-simple-case-at-least", "[compiler]") {
    static constexpr auto f =
        flag_with_value<std::vector<std::string>>().Short('f').Nargs(NargsOpt::at_least(2));
    static constexpr auto opts = options<""_str, ""_str, f>;


    SECTION("pass-2-values-succeeds-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs[0] == "hello"sv);
        REQUIRE(fs[1] == "world"sv);
    }
    SECTION("pass-3-values-succeeds-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}},
            token_t{Argument{.value = "extra"sv}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs[0] == "hello"sv);
        REQUIRE(fs[1] == "extra"sv);
        REQUIRE(fs[2] == "world"sv);
    }

    SECTION("pass-1-value-fails-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_error(out));
    }
}

TEST_CASE("nargs-simple-case-at-most", "[compiler]") {
    static constexpr auto f =
        flag_with_value<std::vector<std::string>>().Short('f').Nargs(NargsOpt::at_most(2));
    static constexpr auto opts = options<""_str, ""_str, f>;


    SECTION("pass-2-values-succeeds-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs[0] == "hello"sv);
        REQUIRE(fs[1] == "world"sv);
    }
    SECTION("pass-3-values-fails-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}},
            token_t{Argument{.value = "wrong"sv}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_error(out));
    }

    SECTION("pass-1-value-succeeds-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs[0] == "hello"sv);
    }
}

TEST_CASE("nargs-simple-case-exactly", "[compiler]") {
    static constexpr auto f =
        flag_with_value<std::vector<std::string>>().Short('f').Nargs(NargsOpt::exactly(2));
    static constexpr auto opts = options<""_str, ""_str, f>;


    SECTION("pass-2-values-succeeds-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs[0] == "hello"sv);
        REQUIRE(fs[1] == "world"sv);
    }
    SECTION("pass-3-values-fails-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}},
            token_t{Argument{.value = "wrong"sv}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_error(out));
    }

    SECTION("pass-1-value-fails-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_error(out));
    }
}

TEST_CASE("nargs-simple-case-in-range", "[compiler]") {
    static constexpr auto f =
        flag_with_value<std::vector<std::string>>().Short('f').Nargs(NargsOpt::in_range(1, 3));
    static constexpr auto opts = options<""_str, ""_str, f>;


    SECTION("pass-2-values-succeeds-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs[0] == "hello"sv);
        REQUIRE(fs[1] == "world"sv);
    }
    SECTION("pass-3-values-succeeds-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}},
            token_t{Argument{.value = "extra"sv}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs[0] == "hello"sv);
        REQUIRE(fs[1] == "extra"sv);
        REQUIRE(fs[2] == "world"sv);
    }

    SECTION("pass-1-value-succeeds-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs[0] == "hello"sv);
    }

    SECTION("pass-0-values-fails-parsing") {
        auto const tokens =
            std::array{token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_error(out));
    }

    SECTION("pass-4-values-fails-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}},
            token_t{Argument{.value = "extra"sv}},
            token_t{Argument{.value = "wow"sv}},
            token_t{Argument{.value = "world"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_error(out));
    }
}

TEST_CASE("nargs-in-range-optional-value", "[compiler]") {
    static constexpr auto f =
        flag_with_value<std::vector<std::string>>().Short('f').Nargs(NargsOpt::in_range(0, 1));
    static constexpr auto opts = options<""_str, ""_str, f>;


    SECTION("pass-0-values-succeeds-parsing") {
        auto const tokens =
            std::array{token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs.size() == 0);
    }

    SECTION("pass-1-value-succeeds-parsing") {
        auto const tokens = std::array{
            token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
            token_t{Argument{.value = "hello"sv}}};

        auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

        REQUIRE(has_args(out));
        auto const &fs = get_args(out).get<f>();

        REQUIRE(fs.size() == 1);
    }
}

TEST_CASE("nargs-works-when-there-are-further-flags", "[compiler]") {
    static constexpr auto f =
        flag_with_value<std::vector<std::string>>().Short('f').Nargs(NargsOpt::at_least(1));
    static constexpr auto g = flag_with_value<int>().Short('g');
    static constexpr auto opts = options<""_str, ""_str, f, g>;

    // We can provide many values, when the new flag is found the system should handle that
    auto const tokens = std::array{
        token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
        token_t{Argument{.value = "hello"sv}},
        token_t{Argument{.value = "world"sv}},
        token_t{ShortFlag{.raw = "-g", .flag = 'g', .has_equal = false}},
        token_t{Argument{.value = "1"sv}}};

    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const &cmds = get_args(out);
    auto const &fs = cmds.get<f>();

    REQUIRE(fs[0] == "hello"sv);
    REQUIRE(fs[1] == "world"sv);

    auto const gs = cmds.get<g>();
    REQUIRE(gs == 1);
}

TEST_CASE(
    "nargs-works-when-there-are-further-positionals-only-if-nargs-is-exhausted", "[compiler]") {
    static constexpr auto f =
        flag_with_value<std::vector<std::string>>().Short('f').Nargs(NargsOpt::at_most(2));
    static constexpr auto g = positional<int>().Name("integer");
    static constexpr auto opts = options<""_str, ""_str, f, g>;

    // We can provide many values, when the new flag is found the system should handle that
    auto const tokens = std::array{
        token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
        token_t{Argument{.value = "hello"sv}},
        token_t{Argument{.value = "world"sv}},
        token_t{Argument{.value = "1"sv}}};

    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const &cmds = get_args(out);
    auto const &fs = cmds.get<f>();

    REQUIRE(fs[0] == "hello"sv);
    REQUIRE(fs[1] == "world"sv);

    auto const gs = cmds.get<g>();
    REQUIRE(gs == 1);
}

TEST_CASE("nargs-swallows-further-positionals-if-nargs-is-not-exhausted", "[compiler]") {
    static constexpr auto f =
        flag_with_value<std::vector<std::string>>().Short('f').Nargs(NargsOpt::at_least(2));
    static constexpr auto g = positional<int>().Name("integer");
    static constexpr auto opts = options<""_str, ""_str, f, g>;

    // We can provide many values, when the new flag is found the system should handle that
    auto const tokens = std::array{
        token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
        token_t{Argument{.value = "hello"sv}},
        token_t{Argument{.value = "world"sv}},
        token_t{Argument{.value = "1"sv}}};

    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));
    auto const &cmds = get_args(out);
    auto const &fs = cmds.get<f>();

    REQUIRE(fs[0] == "hello"sv);
    REQUIRE(fs[1] == "world"sv);
    REQUIRE(fs[2] == "1"sv);

    auto const gs = cmds.get<g>();
    REQUIRE(gs == 0);  // default value has been used
}

TEST_CASE("optional-flag-accepts-value", "[compiler]") {
    static constexpr auto f = flag_with_value<std::optional<int>>().Short('f');
    static constexpr auto opts = options<""_str, ""_str, f>;

    auto const tokens = std::array{
        token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
        token_t{Argument{.value = "1"sv}}};

    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));

    auto const &cmds = get_args(out);

    auto const info = cmds.get_with_info<f>();

    REQUIRE(info.is_used);
    REQUIRE(info.value.has_value());
    REQUIRE(info.value.value() == 1);
}

TEST_CASE("optional-flag-accepts-value-with-equal", "[compiler]") {
    static constexpr auto f = flag_with_value<std::optional<int>>().Short('f');
    static constexpr auto opts = options<""_str, ""_str, f>;

    auto const tokens = std::array{
        token_t{ShortFlag{.raw = "-f=1", .flag = 'f', .has_equal = true}},
        token_t{Argument{.value = "1"}}};

    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));

    auto const &cmds = get_args(out);

    auto const info = cmds.get_with_info<f>();

    REQUIRE(info.is_used);
    REQUIRE(info.value.has_value());
    REQUIRE(info.value.value() == 1);
}

TEST_CASE("optional-flag-works-with-no-value", "[compiler]") {
    static constexpr auto f = flag_with_value<std::optional<int>>().Short('f');
    static constexpr auto opts = options<""_str, ""_str, f>;

    auto const tokens =
        std::array{token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}}};

    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));

    auto const &cmds = get_args(out);

    auto const info = cmds.get_with_info<f>();

    REQUIRE(info.is_used);
    REQUIRE(!info.value.has_value());
}

TEST_CASE("optional-flag-works-with-no-value-before-another-flag", "[compiler]") {
    static constexpr auto f = flag_with_value<std::optional<int>>().Short('f');
    static constexpr auto g = flag().Short('g');
    static constexpr auto opts = options<""_str, ""_str, f, g>;

    auto const tokens = std::array{
        token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
        token_t{ShortFlag{.raw = "-g", .flag = 'g', .has_equal = false}}};

    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));

    auto const &cmds = get_args(out);

    auto const info = cmds.get_with_info<f>();

    REQUIRE(info.is_used);
    REQUIRE(!info.value.has_value());

    REQUIRE(cmds.get<g>());
}

TEST_CASE("optional-flag-works-with-no-value-before-another-flag-with-value", "[compiler]") {
    static constexpr auto f = flag_with_value<std::optional<int>>().Short('f');
    static constexpr auto g = flag_with_value<std::string_view>().Short('g');
    static constexpr auto opts = options<""_str, ""_str, f, g>;

    auto const tokens = std::array{
        token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
        token_t{ShortFlag{.raw = "-g", .flag = 'g', .has_equal = false}},
        token_t{Argument{.value = "hello"sv}}};

    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_args(out));

    auto const &cmds = get_args(out);

    auto const info = cmds.get_with_info<f>();

    REQUIRE(info.is_used);
    REQUIRE(!info.value.has_value());

    REQUIRE(cmds.get<g>() == "hello"sv);
}

TEST_CASE("optional-flag-fails-with-no-value-before-another-positional", "[compiler]") {
    static constexpr auto f = flag_with_value<std::optional<int>>().Short('f');
    static constexpr auto g = positional<std::string_view>().Name("positional");
    static constexpr auto opts = options<""_str, ""_str, f, g>;

    auto const tokens = std::array{
        token_t{ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false}},
        token_t{Argument{.value = "hello"sv}}};

    auto const out = compiler::compile(std::span{tokens}, opts, MutuallyExclusiveGroups<>{});

    REQUIRE(has_error(out));
}

// NOLINTEND(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
