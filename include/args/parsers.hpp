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

namespace args::parsers {

template <typename>
struct Parser;

namespace detail {
template <args::detail::VecLikeContainer Container>
auto container_push_value(Container &c, typename Container::value_type v) {
    c.push_back(v);
}

template <args::detail::SetLikeContainer Container>
auto container_push_value(Container &c, typename Container::value_type v) {
    c.insert(v);
}

template <args::detail::VecLikeContainerExtendableWithRange Container>
auto container_push_range(Container &c, Container v) {
    c.append_range(std::move(v));
}

template <args::detail::SetLikeContainerExtendableWithRange Container>
auto container_push_range(Container &c, Container v) {
    c.insert_range(std::move(v));
}

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

enum class SeparatorType : std::uint8_t {
    Comma,
    Space,
};

template <args::detail::PushContainer C>
struct ContainerParser {
    explicit ContainerParser(C &vs)
        : values{vs} {}

    C &values;
    std::optional<SeparatorType> separator{};

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
            auto const parsed =
                Parser<typename C::value_type>::parse(std::string_view(first_char, closing_quote));
            if (parsed.has_value()) {
                container_push_value(values, std::move(parsed).value());
                return std::next(closing_quote);
            }
            return std::unexpected{std::move(parsed).error()};
        }
        // look for the separator
        auto const sep = std::ranges::find_if(begin, end, [&](char c) {
            return check_separator(c);
        });
        auto const parsed = Parser<typename C::value_type>::parse(std::string_view(begin, sep));
        if (parsed.has_value()) {
            container_push_value(values, std::move(parsed).value());
            return sep;
        }
        return std::unexpected{std::move(parsed).error()};
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
                separator.emplace(SeparatorType::Space);
                return true;
            }
            if (is_comma) {
                separator.emplace(SeparatorType::Comma);
                return true;
            }
            return false;
        }
        switch (separator.value()) {
        case SeparatorType::Comma:
            return is_comma;
        case SeparatorType::Space:
            return is_space;
        }
        std::unreachable();
    }

    template <typename It>
    [[nodiscard]] auto skip_separator(SeparatorType sep, It begin, It end) -> It {
        switch (sep) {
        case SeparatorType::Comma:
            return std::next(begin);
        case SeparatorType::Space: {
            return skip_space(begin, end);
        }
        }
        std::unreachable();
    }
};

template <typename Out, typename It>
[[nodiscard]] auto parse_container(Out &item, It begin, It end)
    -> std::expected<void, std::string> {
    auto parser = ContainerParser{item};
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
};

template <>
struct Parser<std::string> {
    [[nodiscard]] static auto parse(std::string_view v) -> std::expected<std::string, std::string> {
        return std::string(std::ranges::begin(v), std::ranges::end(v));
    }
};

template <args::detail::PushContainer C>
struct Parser<C> {
    [[nodiscard]] static auto parse(std::string_view v) -> std::expected<C, std::string> {
        auto container = C{};
        auto result =
            detail::parse_container(container, std::ranges::begin(v), std::ranges::end(v));
        if (result.has_value()) {
            return container;
        }
        return std::unexpected{std::move(result).error()};
    }

    [[nodiscard]] static auto parse_inplace(C &container, std::string_view v)
        -> std::expected<void, std::string> {
        auto result =
            detail::parse_container(container, std::ranges::begin(v), std::ranges::end(v));
        if (result.has_value()) {
            return {};
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
};

}  // namespace args::parsers

#endif
