#include <print>
#include "args/args.hpp"
#include "args/types.hpp"

using namespace args::literals;

namespace {
constexpr auto a = args::flag().Long("a-flag").Short('a');
constexpr auto b = args::flag().Long("b-flag").Short('b');
constexpr auto c = args::flag().Long("c-flag").Short('c');

constexpr auto x = args::flag().Long("x-flag").Short('x');
constexpr auto y = args::flag().Long("y-flag").Short('y');
constexpr auto z = args::flag().Long("z-flag").Short('z');

constexpr auto options = args::options<
    "04_mutually_exclusive_groups [--a-flag|-a] [--b-flag|-b] [--c-flag|-c] [--x-flag|-x] [--y-flag|-y] [--z-flag|-z]"_str,
    "Example of program with 2 groups of mutually exclusive flags: {a, b, c} and {x, y, z}. One of {x, y, z} must be provided."_str,
    a,
    b,
    c,
    x,
    y,
    z>;

constexpr auto group_0 = args::mutually_exclusive<a, b, c>;
constexpr auto group_1 = args::mutually_exclusive_required<x, y, z>;
constexpr auto groups = args::mutually_exclusive_groups<group_0, group_1>;
}  // namespace

auto main(int argc, char **argv) -> int {
    auto const commands = args::parse_or_exit(argc, argv, options, groups);
    std::println("{}", commands);
}
