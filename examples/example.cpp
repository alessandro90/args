#include <cstdlib>
#include <print>
#include "include/cpp_args.hpp"
#include "include/types.hpp"
#include "include/validators.hpp"

using namespace args::literals;
using namespace std::string_view_literals;

auto main(int argc, char **argv) -> int {
    static constexpr auto c_verbose = args::Flag{
        .long_form = "verbose"_str,
        .short_form = "v"_short_flag,
        .help = "Add verbose information"_str};

    static constexpr auto c_version = args::Subcommand{
        .name = "version"_str,
        .help = "Print the version of the program"_str,
        .rules = args::Rules<args::empty, args::empty, c_verbose>{},
        .is_flag = true};

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
        .validator = args::Pipe<args::len, args::greater_than<1>>};

    static constexpr auto rules = args::Rules<
        "example usage description"_str,
        "A simple example program"_str,
        c_name,
        c_count,
        c_version>{};

    auto const commands = args::try_parse_or_exit_program(argc, argv, rules);

    auto const version = commands.get_with_info<c_version>();
    if (version.is_used) {
        std::println("You asked for the version");
        if (version.subcommands.get<c_verbose>()) {
            std::println("You specified the long version");
        }
        return EXIT_SUCCESS;
    }

    auto const count = commands.get_with_info<c_count>();
    if (count.is_used) {
        std::println("You used the 'count' command {} times", count.count);
    }

    auto const name = commands.get<c_name>();
    std::println("Goodbye, {}", name);

    return EXIT_SUCCESS;
}
