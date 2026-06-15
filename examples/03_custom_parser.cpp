// NOTE: this example is enabled by default, but it can be enabled/disabled with the cmake option
// `BUILD_CUSTOM_PARSER_EXAMPLE`

#include <format>
#include <nlohmann/json.hpp>
#include <print>
#include <unordered_set>
// NOTE: this prevents the warning about std::unordered_set's
// default not being checked at compile time
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "args/args.hpp"
#pragma GCC diagnostic pop
#include "args/parsers.hpp"
#include "args/types.hpp"
#include "args/validators.hpp"

using namespace args::literals;
using json = nlohmann::json;

namespace {

constexpr auto c_set = args::flag_with_value<std::unordered_set<int>>()
                           .Long("set")
                           .Default([] {
                               return std::unordered_set{0, 1, 2};  // to satify the validator
                           })
                           .Validator(args::Pipe<args::len, args::greater_than<2>>)
                           .Help("Unordered set of integers")
                           .Short('s');

constexpr auto c_json = args::flag_with_value<json>()
                            .Long("json")
                            .Help("A json object provided as a string.")
                            .Short('j')
                            .Required(true);


constexpr auto options = args::options<
    "03_custom_parser (--json|-j) JSON"_str,
    "Example implementation of a custom parser using the nlohmann json library"_str,
    c_json,
    c_set>;
}  // namespace

// Define your custom parsers as specializations inside `args::parsers` namespace.
// The parsing functions are always static.
namespace args::parsers {
template <>
struct Parser<json> {
    [[nodiscard]] static auto parse(std::string_view j) -> std::expected<json, std::string> {
        try {
            return json::parse(j);
        } catch (nlohmann::json::parse_error const &e) {
            return std::unexpected{std::string{e.what()}};
        }
    }
};
}  // namespace args::parsers

auto main(int argc, char **argv) -> int {
    auto const commands = args::parse_or_exit(argc, argv, options);
    std::println("json: {}", commands.get<c_json>().dump());
    // It can always be printed anyway, but it will use a placeholder non printable types
    std::println("{}", commands);
}
