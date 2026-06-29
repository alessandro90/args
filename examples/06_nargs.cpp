#include <print>
#include "args/args.hpp"
#include "args/types.hpp"

using namespace args::literals;

namespace {
constexpr auto nargs = args::flag_with_value<std::vector<int>>()
                           .Long("ints")
                           .Help("A series of integer values")
                           .Short('i')
                           .Required()
                           .Nargs(args::NargsOpt::exactly(2));

constexpr auto options =
    args::options<"06_nargs (--ints|-i VALUES)"_str, "Example of nargs usage"_str, nargs>;
}  // namespace

auto main(int argc, char **argv) -> int {
    auto const commands = args::parse_or_exit(argc, argv, options);
    std::println("{}", commands);
}
