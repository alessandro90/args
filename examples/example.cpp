#include "include/types.hpp"

using args::operator""_str;
using args::operator""_flag;
using args::operator""_short_flag;

auto main() -> int {
    static constexpr auto version_subcommand_verbose = args::Flag{
        .long_form = "verbose"_str,
        .short_form = "v"_short_flag,
        .help = "Add verbose information"_str};
    static constexpr auto version_subcommand = args::Subcommand{
        .name = "version"_str,
        .help = "Print the version of the program"_str,
        .rules = args::Rules<args::empty, args::empty, version_subcommand_verbose>{},
        .is_flag = true};
}
