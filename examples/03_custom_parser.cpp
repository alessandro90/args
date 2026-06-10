// NOTE: this example is enabled by default, but it can be enabled/disabled with the cmake option
// `BUILD_CUSTOM_PARSER_EXAMPLE`

#include <expected>
#include <nlohmann/json.hpp>
#include <print>
#include <unordered_set>
#include "args/args.hpp"
#include "args/parsers.hpp"
#include "args/types.hpp"
#include "args/validators.hpp"

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

// set-like containers are supported
// WARN: defaults values are checked at compile time only if the container can be constructed in a
// consteval context. std::vector can for example. As of C++26, std::unordered_set cannot. For such
// types the default validation is skipped and the library will blindly use the default you provide
// (or not provide) without any check
constexpr auto c_set = args::flag_with_value<std::unordered_set<int>>()
                           .Long("set")
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
    c_set,
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
    std::println("json: {}", commands.get<c_json>().dump());
    auto const w = commands.get<c_int_wrapper>();
    if (w.has_value()) {
        std::println("Non default constructible flag: IntWrapper{{{}}}", w.value().x);
    }
    std::println("set: {}", commands.get<c_set>());
}
