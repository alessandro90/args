// NOLINTBEGIN(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)

#include <catch2/generators/catch_generators.hpp>
#include "catch2/catch_test_macros.hpp"

#include <span>
#include <string_view>
#include <variant>
#include "args/tokenizer.hpp"


using namespace args::tokenizer;

namespace {
auto assert_token(token_t tok, auto expected) -> void {
    REQUIRE(std::holds_alternative<decltype(expected)>(tok));
    REQUIRE(std::get<decltype(expected)>(tok) == expected);
}
}  // namespace

#define ASSERT_TOKEN(tok, ...)                \
    do {                                      \
        INFO(#tok << " == " << #__VA_ARGS__); \
        assert_token(tok, __VA_ARGS__);       \
    } while (false)

TEST_CASE("empty-input", "[tokenizer]") {
    auto input = std::array<char const *, 0>{};
    auto const toks = tokenize(input);
    REQUIRE(toks.has_value());
    REQUIRE(toks.value().empty());
}

TEST_CASE("integer-positional", "[tokenizer]") {
    auto input = std::array{"10"};
    auto const toks = tokenize(input);
    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 1);
    auto const value = toks.value()[0];
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const argument = std::get<Argument>(value);
    REQUIRE(argument.value == "10");
}

TEST_CASE("integer-short-flag-no-equal", "[tokenizer]") {
    auto input = std::array{"-v", "10"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<ShortFlag>(key));
    auto const k = std::get<ShortFlag>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == "-v");
    REQUIRE(k.flag == 'v');
    REQUIRE(!k.has_equal);

    REQUIRE(v.value == "10");
}

TEST_CASE("integer-short-flag-clumped", "[tokenizer]") {
    auto input = std::array{"-v10"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<ShortFlag>(key));
    auto const k = std::get<ShortFlag>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == "-v10");
    REQUIRE(k.flag == 'v');
    REQUIRE(!k.has_equal);

    REQUIRE(v.value == "10");
}

TEST_CASE("string-short-flag-clumped", "[tokenizer]") {
    auto const input_str = GENERATE("-v\"hellp\"", "-v'hellp'");

    auto input = std::array{input_str};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<ShortFlag>(key));
    auto const k = std::get<ShortFlag>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == input_str);
    REQUIRE(k.flag == 'v');
    REQUIRE(v.value == std::string_view{&input_str[2]});
    REQUIRE(!k.has_equal);
}

TEST_CASE("integer-short-flag-with-equal", "[tokenizer]") {
    auto input = std::array{"-v=10"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<ShortFlag>(key));
    auto const k = std::get<ShortFlag>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == "-v=10");
    REQUIRE(k.flag == 'v');
    REQUIRE(k.has_equal);

    REQUIRE(v.value == "10");
}

TEST_CASE("integer-long-flag-no-equal", "[tokenizer]") {
    auto input = std::array{"--value", "10"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<LongFlag>(key));
    auto const k = std::get<LongFlag>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == "--value");
    REQUIRE(k.flag == "value");
    REQUIRE(!k.has_equal);

    REQUIRE(v.value == "10");
}

TEST_CASE("integer-long-flag-with-equal", "[tokenizer]") {
    auto input = std::array{"--value=10"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<LongFlag>(key));
    auto const k = std::get<LongFlag>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == "--value=10");
    REQUIRE(k.flag == "value");
    REQUIRE(k.has_equal);

    REQUIRE(v.value == "10");
}

TEST_CASE("integer-group-flag-no-equal", "[tokenizer]") {
    auto input = std::array{"-abc", "10"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<FlagGroup>(key));
    auto const k = std::get<FlagGroup>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == "-abc");
    REQUIRE(k.group == "abc");
    REQUIRE(!k.has_equal);

    REQUIRE(v.value == "10");
}

TEST_CASE("integer-group-flag-with-equal", "[tokenizer]") {
    auto input = std::array{"-abc=10"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<FlagGroup>(key));
    auto const k = std::get<FlagGroup>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == "-abc=10");
    REQUIRE(k.group == "abc");
    REQUIRE(k.has_equal);

    REQUIRE(v.value == "10");
}

TEST_CASE("integer-group-flag-clumped", "[tokenizer]") {
    auto input = std::array{"-abc10"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<FlagGroup>(key));
    auto const k = std::get<FlagGroup>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == "-abc10");
    REQUIRE(k.group == "abc");
    REQUIRE(!k.has_equal);

    REQUIRE(v.value == "10");
}

TEST_CASE("string-group-flag-clumped", "[tokenizer]") {
    auto const input_str = GENERATE("-abc'hello'", "-abc\"hello\"");
    auto input = std::array{input_str};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 2);

    auto const key = toks.value()[0];
    auto const value = toks.value()[1];

    REQUIRE(std::holds_alternative<FlagGroup>(key));
    auto const k = std::get<FlagGroup>(key);
    REQUIRE(std::holds_alternative<Argument>(value));
    auto const v = std::get<Argument>(value);

    REQUIRE(k.raw == input_str);
    REQUIRE(k.group == "abc");
    REQUIRE(!k.has_equal);

    REQUIRE(v.value == std::string_view{&input_str[4]});
}

TEST_CASE("double-dash", "[tokenizer]") {
    auto input = std::array{"--"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 1);

    auto const tok = toks.value()[0];

    REQUIRE(std::holds_alternative<DoubleDash>(tok));
}

TEST_CASE("complex-example", "[tokenizer]") {
    auto input =
        std::array{"-v", "10", "--version", "--full=\"no\"", "-xyz=0", "--", "-f", "--no-flag=0"};
    auto const toks = tokenize(input);

    REQUIRE(toks.has_value());
    REQUIRE(toks.value().size() == 11);

    auto const ts = toks.value();

    ASSERT_TOKEN(ts[0], ShortFlag{.raw = "-v", .flag = 'v', .has_equal = false});
    ASSERT_TOKEN(ts[1], Argument{.value = "10"});
    ASSERT_TOKEN(ts[2], LongFlag{.raw = "--version", .flag = "version", .has_equal = false});
    ASSERT_TOKEN(ts[3], LongFlag{.raw = "--full=\"no\"", .flag = "full", .has_equal = true});
    ASSERT_TOKEN(ts[4], Argument{.value = "\"no\""});
    ASSERT_TOKEN(ts[5], FlagGroup{.raw = "-xyz=0", .group = "xyz", .has_equal = true});
    ASSERT_TOKEN(ts[6], Argument{.value = "0"});
    ASSERT_TOKEN(ts[7], DoubleDash{});
    ASSERT_TOKEN(ts[8], ShortFlag{.raw = "-f", .flag = 'f', .has_equal = false});
    ASSERT_TOKEN(ts[9], LongFlag{.raw = "--no-flag=0", .flag = "no-flag", .has_equal = true});
    ASSERT_TOKEN(ts[10], Argument{.value = "0"});
}

// TODO: invalid tokens

// NOLINTEND(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
