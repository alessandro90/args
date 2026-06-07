#include <cstdio>
#include <cstdlib>
#include <print>
#include "args/args.hpp"
#include "args/types.hpp"

using namespace args::literals;

namespace {
// 'version' can be expressed as a subcommand
constexpr auto c_verbose =
    args::flag().Long("verbose").Short('v').Help("Add a verbose description");

constexpr auto c_version = args::subcommand()
                               .Flag(true)
                               .Name("version")
                               .Help("Version subcommand")
                               .Opts(
                                   args::options<
                                       "01_subcommand --version [--verbose|-v]"_str,
                                       "Display version information"_str,
                                       c_verbose>);

// Subcommands can be nested
constexpr auto c_nested_subcommand_flag =
    args::flag().Long("nested-sub-flag").Help("A flag for the nested subcommand");

constexpr auto c_subcommand_nest =
    args::subcommand()
        .Name("nested-subcommand")
        .Help("A nested subcommand (1 level of nesting)")
        .Opts(args::options<args::empty, args::empty, c_nested_subcommand_flag>);

constexpr auto c_subcommand_flag = args::flag().Long("sub-flag").Help("A flag for the subcommand");

constexpr auto c_subcommand =
    args::subcommand()
        .Name("subcommand")
        .Help("An unnested subcommand")
        .Opts(args::options<args::empty, args::empty, c_subcommand_flag, c_subcommand_nest>);

constexpr auto options = args::options<
    "01_subcommand [--version [--verbose|-v]] | [subcommand [nested-subcommand [--nested-sub-flag]] | [--sub-flag]]"_str,
    "Example displaying how a subcommand works"_str,
    c_version,
    c_subcommand>;
}  // namespace

auto main(int argc, char **argv) -> int {
    auto const result = args::try_parse(argc, argv, options);
    if (args::has_error(result)) {
        std::println(stderr, "{}", result);
        return EXIT_FAILURE;
    }
    if (args::is_empty(result)) {
        std::println("{}", result);
        std::println("{}", options.help());  // NOLINT
        return EXIT_SUCCESS;
    }

    if (args::has_help(result)) {
        std::println("{}", result);
        return EXIT_SUCCESS;
    }

    auto const &commands = args::get_args(result);

    std::println("{}", commands);

    std::println("----------------------------");

    if (commands.get_with_info<c_version>().is_used) {
        std::println("Subcommand invoked");
    }
    if (commands.get<c_version, c_verbose>()) {
        std::println("verbose version required");
    }
    auto const &subc = commands.get_with_info<c_subcommand>();
    if (subc.is_used) {
        std::println("Subcommand invoked:");
        std::println("{}", subc);
    }
    if (subc.subcommands.get<c_subcommand_nest, c_nested_subcommand_flag>()) {
        std::println("nested subcommand flag invoked");
    }
}
