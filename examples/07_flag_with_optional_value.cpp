#include <optional>
#include <print>
#include "args/args.hpp"
#include "args/types.hpp"

using namespace args::literals;

namespace {
constexpr auto j = args::flag_with_value<std::optional<int>>()
                       .Long("optint")
                       .Help("A flag with an optional integer value")
                       .Short('j')
                       .Required();

constexpr auto options = args::options<
    "07_flag_with_optional_value (--optint|-j [INT])"_str,
    "Example of a flag with an optional integer value"_str,
    j>;
}  // namespace

auto main(int argc, char **argv) -> int {
    auto const commands = args::parse_or_exit(argc, argv, options);
    auto const flag = commands.get_with_info<j>();
    std::println("flag is used: {}", flag.is_used);
    std::println("flag has a value: {}", flag.value.has_value());
    if (flag.value.has_value()) {
        std::println("flag has value: '{}'", flag.value.value());
    }
}
