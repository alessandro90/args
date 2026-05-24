#include <cstdlib>
#include <format>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <string_view>
#include "args/args.hpp"
#include "args/parsers.hpp"
#include "args/types.hpp"
#include "args/validators.hpp"

namespace {
struct CustomData {
    int a{};
    int b{};
    int c{};
};
}  // namespace

// Define custom parsers by specializing the (stateless) struct args::parsers::Parser.
namespace args::parsers {
template <>
struct Parser<CustomData> {
    /// this function can return anything 'printable'
    [[nodiscard]] static constexpr auto type_name() -> std::string_view {
        return "CustomData";
    }

    template <std::ranges::range R>
    [[nodiscard]] static auto parse(R v) -> std::optional<CustomData> {
        namespace rng = std::ranges;
        auto components = std::views::split(v, ',');
        auto it = rng::begin(components);
        auto components_end = rng::end(components);
        if (rng::distance(it, components_end) != 3) {
            return std::nullopt;
        }
        auto const p = [&it] {
            return Parser<int>::parse(*it);
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
};

}  // namespace args::parsers

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
    .long_form = "verbose"_flag,
    .short_form = "v"_short_flag,
    .help = "Add verbose information"_str};

static constexpr auto c_version = args::Subcommand{
    .name = "version"_str,
    .help = "Print the version of the program"_str,
    .rules = args::Rules<args::empty, args::empty, c_verbose>{},
    .is_flag = true};

// A positional argument expecting our custom data type
static constexpr auto c_custom_data =
    args::Positional{.type = args::tag<CustomData>, .name = "custom"_str};

static constexpr auto c_custom = args::Subcommand{
    .name = "custom_data"_str, .rules = args::Rules<args::empty, args::empty, c_custom_data>{}};

static constexpr auto c_count = args::Flag{
    .long_form = "count"_flag,
    .short_form = "c"_short_flag,
    .default_value = false,
    .help = "Just a counter flag"_str};

static constexpr auto c_name = args::FlagWithValue{
    .long_form = "name"_flag,
    .short_form = "n"_short_flag,
    // clang-format off
    .default_value = [] { return ""sv; },
    // clang-format on
    .required = true,
    .repeatable = false,
    .help = "Your name"_str,
    .validator = args::And<args::Pipe<args::len, args::greater_than<1>>, capitalized>};

// All the information to parse the command lines into the desired structures
// is specified in the template arguments of this type
static constexpr auto rules = args::rules<
    "example usage description"_str,
    "A simple example program"_str,
    c_custom,
    c_name,
    c_count,
    c_version>;

using rules_t = decltype(rules);

auto main(int argc, char **argv) -> int {
    // Commands has the right shape based on the rules provided
    // No cast is performed when retrieving the data, the struct Args
    // already contains the correct types
    auto const commands = args::parse_or_exit(argc, argv, rules);

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
