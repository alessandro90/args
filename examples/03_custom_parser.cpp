// NOTE: this example is enabled by default, but it can be enabled/disabled with the cmake option
// `BUILD_CUSTOM_PARSER_EXAMPLE`

#include <expected>
#include <nlohmann/json.hpp>
#include <print>
#include "args/args.hpp"
#include "args/parsers.hpp"
#include "args/types.hpp"

using namespace args::literals;
using json = nlohmann::json;

namespace {
struct IntWrapper {
    int x;
    IntWrapper() = delete;

    explicit IntWrapper(int x_)
        : x{x_} {}
};

// Flags/Positionals can have arbitrary types as long as they are default constructible
// If your type is not default constructible, use `std::optional<your_type>`. The library
// supports `Parser<std::optional<T>>` is `Parser<T>` exists. The main inconvenience is that
// if the value is required you get an std::optional anyway, even though it cannot be empty

// A flag with non default constructible argument
constexpr auto c_int_wrapper = args::flag_with_value<std::optional<IntWrapper>>()
                                   .Long("wrapper")
                                   .Help("Integer to be wrapped inside a custom type.")
                                   .Short('w');

constexpr auto c_json = args::flag_with_value<json>()
                            .Long("json")
                            .Help("A json object provided as a string.")
                            .Short('j')
                            .Required(true);
constexpr auto options = args::options<
    "03_custom_parser (--json|-j) JSON"_str,
    "Example implementation of a custom parser using the nlohmann json library"_str,
    c_json,
    c_int_wrapper>;
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

    // If the target type is default constructible, the inplace version can be defined. This version
    // will be selected if present. This is useful only for values constructed using multiple flags,
    // like variadics or repeated values
    // static auto parse(json &value, std::string_view j) -> std::expected<void, std::string>;
};

template <>
struct Parser<IntWrapper> {
    [[nodiscard]] static auto parse(std::string_view v) -> std::expected<IntWrapper, std::string> {
        return Parser<int>::parse(v).transform([](int i) {
            return IntWrapper{i};
        });
    }
};
}  // namespace args::parsers

auto main(int argc, char **argv) -> int {
    auto const commands = args::parse_or_exit(argc, argv, options);
    // args::Args is std::print-compatible only if all its contents are. json is not
    // std::print-compatible
    std::println("JSON:");
    std::println("{}", commands.get<c_json>().dump());
    auto const w = commands.get<c_int_wrapper>();
    if (w.has_value()) {
        std::println("Non default constructible flag: IntWrapper{{{}}}", w.value().x);
    }
}
