#ifndef CPP_ARGS_PARSERS
#define CPP_ARGS_PARSERS

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include "types.hpp"

namespace args::parsers {

consteval auto type_name(args::detail::Typetag<float>) -> std::string_view {
    return "float";
}

consteval auto type_name(args::detail::Typetag<double>) -> std::string_view {
    return "float";
}

consteval auto type_name(args::detail::Typetag<std::uint8_t>) -> std::string_view {
    return "u8";
}

consteval auto type_name(args::detail::Typetag<std::int8_t>) -> std::string_view {
    return "i8";
}

consteval auto type_name(args::detail::Typetag<std::uint16_t>) -> std::string_view {
    return "u16";
}

consteval auto type_name(args::detail::Typetag<std::int16_t>) -> std::string_view {
    return "i16";
}

consteval auto type_name(args::detail::Typetag<std::uint32_t>) -> std::string_view {
    return "u32";
}

consteval auto type_name(args::detail::Typetag<std::int32_t>) -> std::string_view {
    return "i32";
}

consteval auto type_name(args::detail::Typetag<std::uint64_t>) -> std::string_view {
    return "u64";
}

consteval auto type_name(args::detail::Typetag<std::int64_t>) -> std::string_view {
    return "i64";
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
}  // namespace args::parsers
#endif
