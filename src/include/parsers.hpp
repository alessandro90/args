#ifndef CPP_ARGS_PARSERS
#define CPP_ARGS_PARSERS

#include <charconv>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "types.hpp"

namespace args::parsers {

namespace details {
template <typename T>
struct IsVector: std::false_type {};

template <typename T>
struct IsVector<std::vector<T>>: std::true_type {};
}  // namespace details

consteval auto type_name(Typetag<float>) -> std::string_view {
    return "float";
}

consteval auto type_name(Typetag<double>) -> std::string_view {
    return "float";
}

consteval auto type_name(Typetag<std::uint8_t>) -> std::string_view {
    return "u8";
}

consteval auto type_name(Typetag<std::int8_t>) -> std::string_view {
    return "i8";
}

consteval auto type_name(Typetag<std::uint16_t>) -> std::string_view {
    return "u16";
}

consteval auto type_name(Typetag<std::int16_t>) -> std::string_view {
    return "i16";
}

consteval auto type_name(Typetag<std::uint32_t>) -> std::string_view {
    return "u32";
}

consteval auto type_name(Typetag<std::int32_t>) -> std::string_view {
    return "i32";
}

consteval auto type_name(Typetag<std::uint64_t>) -> std::string_view {
    return "u64";
}

consteval auto type_name(Typetag<std::int64_t>) -> std::string_view {
    return "i64";
}

template <typename T>
auto type_name(Typetag<std::vector<T>>) -> std::string {
    return std::format("[{}]", type_name(Typetag<T>{}));
}

template <typename Out, typename It>
requires std::is_arithmetic_v<Out>
[[nodiscard]] auto parse(It begin, It end) -> std::optional<Out> {
    Out value{};
    auto const parsed = std::from_chars(begin, end, value);
    if (parsed.ec == std::errc{}) {
        return value;
    }
    return {};
}

template <typename Out, typename It>
requires details::IsVector<Out>::value
[[nodiscard]] auto parse(It, It) -> std::optional<Out> {
    // TODO:
    return {};
}
}  // namespace args::parsers
#endif
