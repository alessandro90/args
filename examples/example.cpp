#include <concepts>
#include <cstdlib>
#include <format>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include "include/parsers.hpp"
#include "include/types.hpp"
#include "include/validators.hpp"

namespace args::parsers {
// Custom data parsing:
// 1. define the class in args::parsers namespace
// 2. Definitions must be *before* inclusion of the main header for ADL lookup
// 3. define type_name and parse_functions

struct CustomData {  // NOLINT(misc-use-internal-linkage)
    int a{};
    int b{};
    int c{};
};

[[nodiscard]] consteval auto type_name(Typetag<CustomData>) -> std::string_view {
    return "CustomData";
}

template <std::same_as<CustomData> Out, typename It>
[[nodiscard]] auto parse(It begin, It end) -> std::optional<CustomData> {  // NOLINT
    namespace rng = std::ranges;
    auto r = rng::subrange(begin, end);
    auto components = std::views::split(r, ',');
    auto it = rng::begin(components);
    auto components_end = rng::end(components);
    if (rng::distance(it, components_end) != 3) {
        return std::nullopt;
    }
    auto const p = [&it] {
        return parse<int>(rng::begin(*it), rng::end(*it));
    };
    return p().and_then([&](int a) {
        rng::advance(it, 1);
        return p().and_then([&, a](int b) {
            rng::advance(it, 1);
            return p().transform([a, b](int c) {
                return CustomData{.a = a, .b = b, .c = c};
            });
        });
    });
}
}  // namespace args::parsers

// Include this after your custom parsers so ADL works
#include "include/cpp_args.hpp"

using namespace args::literals;
using namespace std::string_view_literals;


// A custom validator checking if the first letter is capitalized
static constexpr auto capitalized = args::Validator{
    .fn =
        [](std::string_view s) {
            return s[0] >= 'A' && s[0] <= 'Z';
        },
    .err_fn =
        [](std::string_view s) {
            return std::format("name '{}' must be capitalized", s);
        }};

static constexpr auto c_verbose = args::Flag{
    .long_form = "verbose"_str,
    .short_form = "v"_short_flag,
    .help = "Add verbose information"_str};

static constexpr auto c_version = args::Subcommand{
    .name = "version"_str,
    .help = "Print the version of the program"_str,
    .rules = args::Rules<args::empty, args::empty, c_verbose>{},
    .is_flag = true};

static constexpr auto c_custom_data =
    args::Positional{.type = args::tag<args::parsers::CustomData>, .name = "custom"_str};

static constexpr auto c_custom = args::Subcommand{
    .name = "custom_data"_str, .rules = args::Rules<args::empty, args::empty, c_custom_data>{}};

static constexpr auto c_count = args::Flag{
    .long_form = "count"_str,
    .short_form = "c"_short_flag,
    .default_value = false,
    .help = "Just a counter flag"_str};

static constexpr auto c_name = args::FlagWithValue{
    .long_form = "name"_str,
    .short_form = "n"_short_flag,
    .default_value =
        [] {
            return ""sv;
        },
    .required = true,
    .repeatable = false,
    .help = "Your name"_str,
    .validator = args::And<args::Pipe<args::len, args::greater_than<1>>, capitalized>};

static constexpr auto rules = args::Rules<
    "example usage description"_str,
    "A simple example program"_str,
    c_custom,
    c_name,
    c_count,
    c_version>{};

auto main(int argc, char **argv) -> int {
    auto const commands = args::try_parse_or_exit_program(argc, argv, rules);

    auto const version = commands.get_with_info<c_version>();
    if (version.is_used) {
        std::println("You asked for the version");
        if (version.subcommands.get<c_verbose>()) {
            std::println("You specified the long version");
        }
        return EXIT_SUCCESS;
    }

    auto const custom = commands.get_with_info<c_custom>();
    if (custom.is_used) {
        std::println("You provided custom data");
        auto const data = custom.subcommands.get<c_custom_data>();
        std::println("CustomData {{a={}, b={}, c={}}}", data.a, data.b, data.c);
        return EXIT_SUCCESS;
    }

    auto const count = commands.get_with_info<c_count>();
    if (count.is_used) {
        std::println("You used the 'count' command {} times", count.count);
    }

    auto const name = commands.get<c_name>();  // name is required, no need to check usage first
    std::println("Goodbye, {}", name);
}
