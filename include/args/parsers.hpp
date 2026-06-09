#ifndef ARGS_PARSERS
#define ARGS_PARSERS

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <expected>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include "helpers.hpp"

#define IN_PLACE_PARSE(type, out, v)   \
    auto res = Parser<type>::parse(v); \
    if (res.has_value()) {             \
        out.push_back(res.value());    \
    }                                  \
    return std::unexpected {           \
        std::move(res).error()         \
    }

namespace args::parsers {

template <typename P, typename Item>
concept InPlaceParser = requires(Item &item, std::string_view rn) {
    { P::parse(item, rn) } -> std::same_as<std::expected<void, std::string>>;
};

template <typename>
struct Parser;

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

template <typename T, typename A>
struct VecParser {
    explicit VecParser(std::vector<T, A> &vs)
        : values{vs} {}

    std::vector<T, A> &values;
    std::optional<VecSeparatorType> separator{};

    template <typename It>
    [[nodiscard]] auto parse_value(It begin, It end) -> std::expected<It, std::string> {
        // string-like parse
        if (is_quote(*begin)) {
            auto const closing = *begin;
            auto const first_char = std::next(begin);
            if (first_char == end) {
                return std::unexpected{
                    std::format("Invalid string: '{}'", std::string_view(begin, end))};
            }
            auto const closing_quote = std::ranges::find_if(first_char, end, [=](char c) {
                return c == closing;
            });
            if (closing_quote == end) {
                return std::unexpected{
                    std::format("Missing closing quote: '{}'", std::string_view(begin, end))};
            }
            if constexpr (InPlaceParser<Parser<T>, std::vector<T, A>>) {
                auto const parsed =
                    Parser<T>::parse(values, std::string_view(first_char, closing_quote));
                if (parsed.has_value()) {
                    return std::next(closing_quote);
                }
                return std::unexpected{std::move(parsed).error()};
            } else {
                auto parsed = Parser<T>::parse(std::string_view(first_char, closing_quote));
                if (parsed.has_value()) {
                    values.push_back(std::move(parsed).value());
                    return std::next(closing_quote);
                }
                return std::unexpected{std::move(parsed).error()};
            }
        }
        // look for the separator
        auto const sep = std::ranges::find_if(begin, end, [&](char c) {
            return check_separator(c);
        });
        if constexpr (InPlaceParser<Parser<T>, std::vector<T, A>>) {
            auto const parsed = Parser<T>::parse(values, std::string_view(begin, sep));
            if (parsed.has_value()) {
                return sep;
            }
            return std::unexpected{std::move(parsed).error()};
        } else {
            auto parsed = Parser<T>::parse(std::string_view(begin, sep));
            if (parsed.has_value()) {
                values.push_back(std::move(parsed).value());
                return sep;
            }
            return std::unexpected{std::move(parsed).error()};
        }
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
[[nodiscard]] auto parse_vector(Out &item, It begin, It end) -> std::expected<void, std::string> {
    auto parser = VecParser{item};
    while (true) {
        auto it = detail::skip_space(begin, end);
        if (it == end) {
            return {};
        }
        auto maybe_it = parser.parse_value(it, end);
        if (!maybe_it.has_value()) {
            return std::unexpected{std::move(maybe_it).error()};
        }
        it = maybe_it.value();
        if (it == end) {
            return {};
        }
        auto after_sep = parser.consume_separator(it, end);
        if (!after_sep.has_value()) {
            // missing separator allowed only if last entry
            it = skip_space(it, end);
            if (it == end) {
                return {};
            }
            return std::unexpected{
                std::format("Missing separator at '{}'", std::string_view(it, end))};
        }
        begin = after_sep.value();
    }
}
}  // namespace detail

template <typename Out>
requires std::is_arithmetic_v<Out>
[[nodiscard]] auto parse_arithmetic(std::string_view v) -> std::expected<Out, std::string> {
    Out value{};
    auto const begin = std::ranges::begin(v);
    auto const end = std::ranges::end(v);
    auto const parsed = std::from_chars(begin, end, value);
    auto const fail = [v] {
        return std::format("failed to parse '{}' into '{}'", v, args::detail::name_of<Out>());
    };
    // If we did not parse the whole range, we allow only for unparsed whitespaces
    // otherwise the parsing is considered to have failed
    if (parsed.ptr != end && !std::all_of(parsed.ptr, end, [](char c) {
            return std::isspace(c) != 0;
        })) {
        return std::unexpected{fail()};
    }
    if (parsed.ec == std::errc{}) {
        return value;
    }
    return std::unexpected{fail()};
}

template <typename T>
struct Parser {};

template <typename T>
requires std::is_arithmetic_v<T>
struct Parser<T> {
    [[nodiscard]] static auto parse(std::string_view v) -> std::expected<T, std::string> {
        return parse_arithmetic<T>(v);
    }

    template <typename A>
    [[nodiscard]] static auto parse(std::vector<T, A> &out, std::string_view v)
        -> std::expected<void, std::string> {
        auto res = parse_arithmetic<T>(v);
        if (res.has_value()) {
            out.push_back(res.value());
        }
        return std::unexpected{std::move(res).error()};
    }
};

template <>
struct Parser<std::string_view> {
    [[nodiscard]] static auto parse(std::string_view v)
        -> std::expected<std::string_view, std::string> {
        return std::string_view(std::ranges::begin(v), std::ranges::end(v));
    }

    template <typename A>
    [[nodiscard]] static auto parse(std::vector<std::string_view, A> &out, std::string_view v)
        -> std::expected<void, std::string> {
        IN_PLACE_PARSE(std::string_view, out, v);
    }
};

template <>
struct Parser<std::string> {
    [[nodiscard]] static auto parse(std::string_view v) -> std::expected<std::string, std::string> {
        return std::string(std::ranges::begin(v), std::ranges::end(v));
    }

    template <typename A>
    [[nodiscard]] static auto parse(std::vector<std::string, A> &out, std::string_view v)
        -> std::expected<void, std::string> {
        IN_PLACE_PARSE(std::string, out, v);
    }
};

template <typename T, typename A>
struct Parser<std::vector<T, A>> {
    [[nodiscard]] static auto parse(std::vector<T, A> &item, std::string_view v)
        -> std::expected<void, std::string> {
        return detail::parse_vector<
            std::vector<T, A>>(item, std::ranges::begin(v), std::ranges::end(v));
    }

    [[nodiscard]] static auto parse(std::string_view v)
        -> std::expected<std::vector<T, A>, std::string> {
        auto vec = std::vector<T, A>{};
        auto result = Parser<std::vector<T, A>>::parse(vec, v);
        if (result.has_value()) {
            return vec;
        }
        return std::unexpected{std::move(result).error()};
    }
};

template <typename T>
struct Parser<std::optional<T>> {
    [[nodiscard]] static auto parse(std::string_view v)
        -> std::expected<std::optional<T>, std::string> {
        return Parser<T>::parse(v).transform([](T x) {
            return std::optional{x};
        });
    }

    template <typename A>
    [[nodiscard]] static auto parse(std::vector<std::optional<T>, A> &out, std::string_view v)
        -> std::expected<void, std::optional<T>> {
        IN_PLACE_PARSE(std::optional<T>, out, v);
    }
};

}  // namespace args::parsers

#undef IN_PLACE_PARSE
#endif
