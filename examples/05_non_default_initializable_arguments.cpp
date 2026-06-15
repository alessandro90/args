#include <expected>
#include <format>
#include <print>
#include <string_view>
#include "args/args.hpp"
#include "args/parsers.hpp"
#include "args/types.hpp"

using namespace args::literals;

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

constexpr auto options = args::options<
    "05_non_default_initializable_arguments (--wrapper|-w)"_str,
    "Example usage of a non default initializable type"_str,
    c_int_wrapper>;
}  // namespace

// custom parser to directly build the object from inside the library
namespace args::parsers {
template <>
struct Parser<IntWrapper> {
    [[nodiscard]] static auto parse(std::string_view v) -> std::expected<IntWrapper, std::string> {
        return Parser<int>::parse(v).transform([](int i) {
            return IntWrapper{i};
        });
    }
};
}  // namespace args::parsers

// Make the custom type printable

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
    std::println("{}", commands);
}
