#ifndef CPP_ARGS_PARSERS
#define CPP_ARGS_PARSERS

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "type_helpers.hpp"
#include "typetag.hpp"

namespace args::parsers {

[[nodiscard]] consteval auto type_name(Typetag<float>) -> std::string_view {
    return "float";
}

[[nodiscard]] consteval auto type_name(Typetag<double>) -> std::string_view {
    return "float";
}

[[nodiscard]] consteval auto type_name(Typetag<std::uint8_t>) -> std::string_view {
    return "u8";
}

[[nodiscard]] consteval auto type_name(Typetag<std::int8_t>) -> std::string_view {
    return "i8";
}

[[nodiscard]] consteval auto type_name(Typetag<std::uint16_t>) -> std::string_view {
    return "u16";
}

[[nodiscard]] consteval auto type_name(Typetag<std::int16_t>) -> std::string_view {
    return "i16";
}

[[nodiscard]] consteval auto type_name(Typetag<std::uint32_t>) -> std::string_view {
    return "u32";
}

[[nodiscard]] consteval auto type_name(Typetag<std::int32_t>) -> std::string_view {
    return "i32";
}

[[nodiscard]] consteval auto type_name(Typetag<std::uint64_t>) -> std::string_view {
    return "u64";
}

[[nodiscard]] consteval auto type_name(Typetag<std::int64_t>) -> std::string_view {
    return "i64";
}

[[nodiscard]] consteval auto type_name(Typetag<std::string_view>) -> std::string_view {
    return "str(view)";
}

[[nodiscard]] consteval auto type_name(Typetag<std::string>) -> std::string_view {
    return "str(owned)";
}

template <typename T>
[[nodiscard]] auto type_name(Typetag<std::vector<T>>) -> std::string {
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
requires detail::IsStringView<Out>::value
[[nodiscard]] auto parse(It begin, It end) -> std::optional<Out> {
    return std::string_view(begin, end);
}

template <typename Out, typename It>
requires detail::IsString<Out>::value
[[nodiscard]] auto parse(It begin, It end) -> std::optional<Out> {
    return std::string(begin, end);
}

namespace detail {
template <typename It>
[[nodiscard]] auto skip_space(It begin, It end) -> It {
    while (begin != end && std::isspace(*begin)) {
        ++begin;
    }
    return begin;
}

template <typename It>
[[nodiscard]] auto skip_sep(It begin, It end) -> It {
    if (begin != end && *begin == ',') {
        begin = std::next(begin);
    }
    begin = detail::skip_space(begin, end);
    if (begin != end && *begin == ',') {
        begin = std::next(begin);
    }
    return begin;
}

template <typename Out, typename It>
[[nodiscard]] auto parse_vec_recursive(std::vector<Out> &v, It begin, It end) -> bool {
    begin = skip_sep(begin, end);
    if (begin == end) {
        return true;
    }
    auto const is_quote = [](auto iter) {
        return *iter == '"' || *iter == '\'';
    };
    auto entry_end = [&]() {
        // we are parsing a string
        if (is_quote(begin)) {
            auto const closing = *begin == '"' ? '"' : '\'';
            begin = std::next(begin);
            return std::ranges::find_if(begin, end, [=](char c) {
                return c == closing;
            });
        }
        // we are parsing something else
        return std::ranges::find_if(begin, end, [](char c) {
            return std::isspace(c) || c == ',';
        });
    }();
    auto parsed = parse<Out>(begin, entry_end);
    if (parsed.has_value()) {
        v.push_back(std::move(parsed).value());
    } else {
        return false;
    }
    if (*entry_end == ',' || is_quote(entry_end)) {
        entry_end = std::next(entry_end);
    }
    if (entry_end == end) {
        return true;
    }
    return parse_vec_recursive(v, entry_end, end);
}
}  // namespace detail

template <typename Out, typename It>
requires args::detail::IsVector<Out>::value
[[nodiscard]] auto parse(It begin, It end) -> std::optional<Out> {
    if (begin == end || *begin == ',') {
        return {};
    }
    auto v = Out{};
    if (detail::parse_vec_recursive(v, begin, end)) {
        return std::optional{std::move(v)};
    }
    return {};
}
}  // namespace args::parsers
#endif
