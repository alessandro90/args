#include <format>
#include <print>
#include <string_view>
#include <vector>
#include "args/args.hpp"
#include "args/types.hpp"
#include "args/validators.hpp"

using namespace args::literals;

namespace {
constexpr auto capitalized = args::Validator{
    .fn =
        [](std::string_view value) {
            return !value.empty() && value[0] >= 'A' && value[0] <= 'Z';
        },
    .err_fn =
        [](std::string_view value) {
            return std::format("'{}' must be capitalized", value);
        }};

constexpr auto items_in_range = args::Pipe<args::len, args::inclusive_range<1, 5>>;

constexpr auto c_values = args::positional<std::vector<std::string_view>>()
                              .Name("VALUES")
                              .Variadic(true)
                              .Help("Arbitrary number of string arguments.")
                              .Required(true)
                              .Validator(args::And<items_in_range, args::All<capitalized>>);

constexpr auto options = args::options<
    "02_validation VALUES..."_str,
    "Example on combining validators and how to build a custom validator."
    " Provide a range in [1, 5] arguments. All arguments must be capitalized"_str,
    c_values>;
}  // namespace

auto main(int argc, char **argv) -> int {
    auto const commands = args::parse_or_exit(argc, argv, options);
    std::println("{}", commands);
}
