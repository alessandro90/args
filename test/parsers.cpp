// NOLINTBEGIN(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)

#include "catch2/catch_test_macros.hpp"
// #include "catch2/matchers/catch_matchers_floating_point.hpp"

#include <string_view>
#include <vector>
#include "args/parsers.hpp"

using namespace args;
using namespace std::string_view_literals;

TEST_CASE("empty-int-vec", "[parsers]") {
    auto const to_parse = ""sv;
    auto const v = parsers::Parser<std::vector<int>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector<int>{});
}

TEST_CASE("one-int-vec", "[parsers]") {
    auto const to_parse = "10"sv;
    auto const v = parsers::Parser<std::vector<int>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{10});
}

TEST_CASE("multiple-int-vec-commas", "[parsers]") {
    auto const to_parse = "10,21,32"sv;
    auto const v = parsers::Parser<std::vector<int>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{10, 21, 32});
}

TEST_CASE("multiple-int-vec-spaces", "[parsers]") {
    auto const to_parse = "  10 11  12\t13"sv;
    auto const v = parsers::Parser<std::vector<int>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{10, 11, 12, 13});
}

TEST_CASE("multiple-int-vec-spaces-and-commas-weird", "[parsers]") {
    auto const to_parse = "  10, 11,12  ,13"sv;
    auto const v = parsers::Parser<std::vector<int>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{10, 11, 12, 13});
}

TEST_CASE("multiple-int-vec-spaces-and-commas-mixed", "[parsers]") {
    auto const to_parse = "  10, 11 12  ,13"sv;
    auto const v = parsers::Parser<std::vector<int>>::parse(to_parse);

    REQUIRE(!v.has_value());
}

///

TEST_CASE("empty-str-vec", "[parsers]") {
    auto const to_parse = ""sv;
    auto const v = parsers::Parser<std::vector<std::string_view>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector<std::string_view>{});
}

TEST_CASE("one-str-vec-no-quotes", "[parsers]") {
    auto const to_parse = "10"sv;
    auto const v = parsers::Parser<std::vector<std::string_view>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{"10"sv});
}

TEST_CASE("one-str-vec-quotes", "[parsers]") {
    auto const to_parse = "\"10\""sv;
    auto const v = parsers::Parser<std::vector<std::string_view>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{"10"sv});
}

TEST_CASE("multiple-str-vec-commas-no-quotes", "[parsers]") {
    auto const to_parse = "10,11,12"sv;
    auto const v = parsers::Parser<std::vector<std::string_view>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{"10"sv, "11"sv, "12"sv});
}

TEST_CASE("multiple-str-vec-commas-quotes", "[parsers]") {
    auto const to_parse = "\"10\",\"11\",\"12\""sv;
    auto const v = parsers::Parser<std::vector<std::string_view>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{"10"sv, "11"sv, "12"sv});
}

TEST_CASE("multiple-str-vec-spaces-no-quotes", "[parsers]") {
    auto const to_parse = "  10 11  12\t13"sv;
    auto const v = parsers::Parser<std::vector<std::string_view>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{"10"sv, "11"sv, "12"sv, "13"sv});
}

TEST_CASE("multiple-str-vec-spaces-quotes", "[parsers]") {
    auto const to_parse = "  \"10\" \"11\"  \"12\"\t\"13\""sv;
    auto const v = parsers::Parser<std::vector<std::string_view>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{"10"sv, "11"sv, "12"sv, "13"sv});
}

TEST_CASE("multiple-str-vec-spaces-and-commas-weird-no-quotes", "[parsers]") {
    auto const to_parse = "  10, 11,12  ,13"sv;
    auto const v = parsers::Parser<std::vector<std::string_view>>::parse(to_parse);

    REQUIRE(v.has_value());
    // the space after '12' is not an error
    REQUIRE(v.value() == std::vector{"10"sv, "11"sv, "12  "sv, "13"sv});
}

TEST_CASE("multiple-str-vec-spaces-and-commas-weird-quotes", "[parsers]") {
    auto const to_parse = "  \"10\", \"11\",\"12\"  ,\"13\""sv;
    auto const v = parsers::Parser<std::vector<std::string_view>>::parse(to_parse);

    REQUIRE(v.has_value());
    REQUIRE(v.value() == std::vector{"10"sv, "11"sv, "12"sv, "13"sv});
}

TEST_CASE("str-vec-inplace", "[parsers]") {
    auto const to_parse = "hello,world"sv;
    auto v = std::vector<std::string_view>{};
    auto const ok = parsers::Parser<std::vector<std::string_view>>::parse_inplace(v, to_parse);

    REQUIRE(ok.has_value());
    REQUIRE(v == std::vector{"hello"sv, "world"sv});
}

// NOLINTEND(cppcoreguidelines-avoid-do-while, misc-use-anonymous-namespace,
// readability-function-congnitive-complexity)
