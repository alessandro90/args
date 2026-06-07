#include <print>
#include <vector>
#include "args/args.hpp"
#include "args/types.hpp"

using namespace args::literals;

namespace {
constexpr auto toggle = args::flag()
                            .Long("toggle")
                            .Required(false)                    // optional
                            .Short('t')                         // optional
                            .Help("A simple boolean toggle.");  // optional

constexpr auto counter =
    args::flag_with_value<int>()
        .Long("counter")
        .Short('c')      // optional
        .Default(-1)     // optional
        .Required(true)  // optional
        // .Repeatable(false)                   // optional - valid only for std::vector
        .Help("Integer counter argument.");  // optional

constexpr auto repeated_values = args::flag_with_value<std::vector<std::string_view>>()
                                     .Long("repeated-values")
                                     .Short('r')        // optional
                                     .Repeatable(true)  // Can be used only with std::vector
                                     .Help("Accumulator of std::string_views");  // optional

constexpr auto text =
    args::positional<std::string_view>()
        .Name("pos-arg")
        .Required(false)  // optional
        // .Variadic(false)                             // optional - valid only for std::vector
        .Help("An optional string_view argument.");  // optional

constexpr auto variadic_positional =
    args::positional<std::vector<std::string_view>>()
        .Name("variadic-pos-arg")
        .Variadic(true)  // Can be used only with std::vector (default is true)
        .Help("A variadic list of positional arguments.");  // optional

constexpr auto opts = args::options<
    "00_basic [-t|--toggle] (-c|--counter) <int> [(-r|--repeated-values) <string>]... [<pos-arg>] [<variadic-pos-arg>...]"_str,
    "00_basic: A minimal introductory example demonstrating basic command-line argument parsing."_str,
    toggle,
    counter,
    text,
    repeated_values,
    variadic_positional>;
}  // namespace

auto main(int argc, char **argv) -> int {
    // Commands has the right shape based on the options provided
    // No cast is performed when retrieving the data, the struct args::Args
    // already contains the correct types
    auto const commands = args::parse_or_exit(argc, argv, opts);
    std::println("Provided commands:");
    std::println("{}", commands);  // args::Args is printable provided all the parsed types are too

    // NOTE: if you just want the value of the argument and are not interested if
    // the user actually provided it, you can use `commands.get<your_command>()`
    auto const &toggle_info = commands.get_with_info<toggle>();
    std::println(
        "toggle: {}, used: {}, repeated: {} times",
        commands.get<toggle>(),  // or `toggle_info.value`
        toggle_info.is_used,
        toggle_info.count);

    auto const &counter_info = commands.get_with_info<counter>();
    std::println("counter: {}, used: {}", counter_info.value, counter_info.is_used);

    auto const &repeated_values_info = commands.get_with_info<repeated_values>();
    std::println(
        "repeated-values: {}, used: {}", repeated_values_info.value, repeated_values_info.is_used);

    auto const &text_info = commands.get_with_info<text>();
    std::println("provided text: '{}', used: {}", text_info.value, text_info.is_used);

    auto const &variadic_positional_info = commands.get_with_info<variadic_positional>();
    std::println("{}", variadic_positional_info);  // args::ArgValue is printable provided the type
                                                   // it contains is printable too
}
