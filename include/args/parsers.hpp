#ifndef ARGS_PARSERS
#define ARGS_PARSERS

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
#include "helpers.hpp"
#include "types.hpp"

namespace args::parsers {

template <typename P, typename Item>
concept InPlaceParser = requires(Item &item, std::string_view rn) {
    { P::parse(item, rn) } -> std::same_as<std::optional<void>>;
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

    std::vector<T> &values;
    std::optional<VecSeparatorType> separator{};

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
            if constexpr (InPlaceParser<Parser<T>, std::vector<T, A>>) {
                auto const parsed =
                    Parser<T>::parse(values, std::string_view(first_char, closing_quote));
                if (parsed) {
                    return std::next(closing_quote);
                }
            } else {
                auto parsed = Parser<T>::parse(std::string_view(first_char, closing_quote));
                if (parsed.has_value()) {
                    values.push_back(std::move(parsed).value());
                    return std::next(closing_quote);
                }
            }
            return {};
        }
        // look for the separator
        auto const sep = std::ranges::find_if(begin, end, [&](char c) {
            return check_separator(c);
        });
        if constexpr (InPlaceParser<Parser<T>, std::vector<T, A>>) {
            auto const parsed = Parser<T>::parse(values, std::string_view(begin, sep));
            if (parsed) {
                return sep;
            }
        } else {
            auto parsed = Parser<T>::parse(std::string_view(begin, sep));
            if (parsed.has_value()) {
                values.push_back(std::move(parsed).value());
                return sep;
            }
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
[[nodiscard]] auto parse_vector(Out &item, It begin, It end) -> bool {
    auto parser = VecParser{item};
    while (true) {
        auto it = detail::skip_space(begin, end);
        if (it == end) {
            return true;
        }
        auto maybe_it = parser.parse_value(it, end);
        if (!maybe_it.has_value()) {
            return false;
        }
        it = maybe_it.value();
        if (it == end) {
            return true;
        }
        maybe_it = parser.consume_separator(it, end);
        if (!maybe_it.has_value()) {
            // missing separator allowed only if last entry
            it = skip_space(it, end);
            return it == end;
        }
        begin = maybe_it.value();
    }
}
}  // namespace detail

template <typename Out>
requires std::is_arithmetic_v<Out>
[[nodiscard]] auto parse_arithmetic(std::string_view v) -> std::optional<Out> {
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

template <typename T>
struct Parser {};

template <typename T>
requires std::is_arithmetic_v<T>
struct Parser<T> {
    [[nodiscard]] static auto parse(std::string_view v) -> std::optional<T> {
        return parse_arithmetic<T>(v);
    }
};

template <typename T>
requires args::detail::is_string_view_v<T>
struct Parser<T> {
    [[nodiscard]] static auto parse(std::string_view v) -> std::optional<std::string_view> {
        return std::string_view(std::ranges::begin(v), std::ranges::end(v));
    }
};

template <typename T>
requires args::detail::is_string_v<T>
struct Parser<T> {
    [[nodiscard]] static auto parse(std::string_view v) -> std::optional<std::string> {
        return std::string(std::ranges::begin(v), std::ranges::end(v));
    }
};

template <typename T>
requires args::detail::is_vector_v<T>
struct Parser<T> {
    [[nodiscard]] static auto parse(T &item, std::string_view v) -> bool {
        return detail::parse_vector<T>(item, std::ranges::begin(v), std::ranges::end(v));
    }

    [[nodiscard]] static auto parse(std::string_view v) -> std::optional<T> {
        auto vec = T{};
        if (Parser<T>::parse(vec, v)) {
            return vec;
        }
        return std::nullopt;
    }
};
}  // namespace args::parsers
#endif
