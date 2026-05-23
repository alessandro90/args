#ifndef CPP_ARGS_PARSERS
#define CPP_ARGS_PARSERS

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include "include/types.hpp"
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

template <typename Out, std::ranges::range R>
requires std::is_arithmetic_v<Out>
[[nodiscard]] auto parse(R v) -> std::optional<Out> {
    Out value{};
    auto const begin = std::ranges::begin(v);
    auto const end = std::ranges::end(v);
    auto const parsed = std::from_chars(begin, end, value);
    // If we did not parse the whole range, we allow only for unparsed whitespaces
    // otherwise the parsing is considered to have failed
    if (parsed.ptr != end && !std::all_of(parsed.ptr, end, [](char c) {
            return std::isspace(c) != 0;
        })) {
        return {};
    }
    if (parsed.ec == std::errc{}) {
        return value;
    }
    return {};
}

template <typename Out, std::ranges::range R>
requires detail::IsStringView<Out>::value
[[nodiscard]] auto parse(R v) -> std::optional<Out> {
    return std::string_view(std::ranges::begin(v), std::ranges::end(v));
}

template <typename Out, std::ranges::range R>
requires detail::IsString<Out>::value
[[nodiscard]] auto parse(R v) -> std::optional<Out> {
    return std::string(std::ranges::begin(v), std::ranges::end(v));
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

[[nodiscard]] constexpr auto is_quote(char c) -> bool {
    return c == '\"' || c == '\'';
}

enum class VecSeparatorType : std::uint8_t {
    Comma,
    Space,
};

template <typename T>
struct VecParser {
    std::optional<VecSeparatorType> separator;
    std::vector<T> values;

    template <typename It>
    [[nodiscard]] auto parse_value(It begin, It end) -> std::optional<It> {
        // string-like parse
        if (is_quote(*begin)) {
            auto const closing = *begin;
            auto const first_char = std::next(begin);
            if (first_char == end) {
                return {};
            }
            auto const closing_quote = std::ranges::find_if(first_char, end, [=](char c) {
                return c == closing;
            });
            if (closing_quote == end) {
                return {};
            }
            auto parsed = parse<T>(std::ranges::subrange(first_char, closing_quote));
            if (parsed.has_value()) {
                values.push_back(std::move(parsed).value());
                return std::next(closing_quote);
            }
            return {};
        }
        // look for the separator
        auto const sep = std::ranges::find_if(begin, end, [&](char c) {
            return check_separator(c);
        });
        auto parsed = parse<T>(std::ranges::subrange(begin, sep));
        if (parsed.has_value()) {
            values.push_back(std::move(parsed).value());
            return sep;
        }
        return {};
    }

    template <typename It>
    [[nodiscard]] auto consume_separator(It begin, It end) -> std::optional<It> {
        while (begin != end) {
            auto const got_separator = check_separator(*begin);
            // didn't get a separator, but may have whitespace to skip
            if (!got_separator) {
                if (std::isspace(*begin)) {
                    begin = std::next(begin);
                } else {
                    return {};
                }
            } else {
                break;
            }
        }
        if (begin == end) {
            return {};
        }
        return skip_separator(separator.value(), begin, end);
    }

private:
    [[nodiscard]] auto check_separator(char c) -> bool {
        auto const is_space = std::isspace(c) != 0;
        auto const is_comma = c == ',';
        if (!separator.has_value()) {
            if (is_space) {
                separator.emplace(VecSeparatorType::Space);
                return true;
            }
            if (is_comma) {
                separator.emplace(VecSeparatorType::Comma);
                return true;
            }
            return false;
        }
        switch (separator.value()) {
        case VecSeparatorType::Comma:
            return is_comma;
        case VecSeparatorType::Space:
            return is_space;
        }
        std::unreachable();
    }

    template <typename It>
    [[nodiscard]] auto skip_separator(VecSeparatorType sep, It begin, It end) -> It {
        switch (sep) {
        case VecSeparatorType::Comma:
            return std::next(begin);
        case VecSeparatorType::Space: {
            return skip_space(begin, end);
        }
        }
        std::unreachable();
    }
};

template <typename Out, typename It>
[[nodiscard]] auto parse_vector(It begin, It end)
    -> std::optional<std::vector<typename Out::value_type>> {
    auto parser = VecParser<typename Out::value_type>{};
    while (true) {
        auto it = detail::skip_space(begin, end);
        if (it == end) {
            return std::move(parser).values;
        }
        auto maybe_it = parser.parse_value(it, end);
        if (!maybe_it.has_value()) {
            return {};
        }
        it = maybe_it.value();
        if (it == end) {
            return std::move(parser).values;
        }
        maybe_it = parser.consume_separator(it, end);
        if (!maybe_it.has_value()) {
            // missing separator allowed only if last entry
            it = skip_space(it, end);
            if (it == end) {
                return std::move(parser).values;
            }
            return {};
        }
        begin = maybe_it.value();
    }
}
}  // namespace detail

template <typename Out, std::ranges::range R>
requires args::detail::IsVector<Out>::value
[[nodiscard]] auto parse(R v) -> std::optional<Out> {
    return detail::parse_vector<Out>(std::ranges::begin(v), std::ranges::end(v));
}

template <typename Out, std::ranges::range R>
requires args::detail::IsRepeatableParseType<Out>::value
[[nodiscard]] auto parse(R v) -> std::optional<Out> {
    // Is this really better than the old style implementation below?
    // first try a parse for a single argument
    // return parse<args::detail::repeatable_single_type_t<Out>>(begin, end)
    //     .transform([](auto p) {
    //         return Out{std::move(p)};
    //     })
    //     // otherwise try to parse a vector
    //     .or_else([=] {
    //         return parse<std::vector<args::detail::repeatable_single_type_t<Out>>>(begin, end)
    //             .transform([](auto p) {
    //                 return Out{std::move(p)};
    //             });
    //     });
    auto parsed = parse<args::detail::repeatable_single_type_t<Out>>(v);
    if (parsed.has_value()) {
        return Out{std::move(parsed).value()};
    }
    // otherwise try to parse a vector
    auto parsed_v = parse<std::vector<args::detail::repeatable_single_type_t<Out>>>(v);
    return std::move(parsed_v).transform([](auto p) {
        return Out{std::move(p)};
    });
}
}  // namespace args::parsers
#endif
