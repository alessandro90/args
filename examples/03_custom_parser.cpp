// NOTE: this example is enabled by default, but it can be enabled/disabled with the cmake option
// `BUILD_CUSTOM_PARSER_EXAMPLE`

#include <format>
#include <nlohmann/json.hpp>
#include <print>
// #include <unordered_set>
#include <vector>
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

    explicit constexpr IntWrapper(int x_)
        : x{x_} {}
};

// A flag with non default constructible argument
constexpr auto c_int_wrapper = args::flag_with_value<IntWrapper>()
                                   .Long("wrapper")
                                   .Default([] {
                                       return IntWrapper{0};
                                   })
                                   .Help("Integer to be wrapped inside a custom type.")
                                   .Short('w');

// set-like containers are supported
// WARN: defaults values are checked at compile time only if the container can be *default*
// constructed in a consteval context. std::vector can for example. As of C++26,
// std::unordered_set cannot. For such types the default validation is skipped and the
// library will blindly use the default you provide (or not provide) without any check.
// Replace std::vector with std::unordered_set and you will see a warning about this behaviour
constexpr auto c_set = args::flag_with_value<std::vector<int>>()
                           .Long("set")
                           .Default([] {
                               return std::vector{0, 1, 2};  // to satify the validator
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

namespace std {

template <>
struct formatter<IntWrapper>: formatter<int> {  // NOLINT(cert-dcl58-cpp)

    template <typename Ctx>
    auto format(IntWrapper const &w, Ctx &ctx) const {  // NOLINT
        return formatter<int>::format(w.x, ctx);
    }
};
}  // namespace std

auto main(int argc, char **argv) -> int {
    auto const commands = args::parse_or_exit(argc, argv, options);
    std::println("json: {}", commands.get<c_json>().dump());
    // It can always be printed anyway, but it will use a placeholder non printable types
    std::println("{}", commands);
}
