#include "args/tokenizer.hpp"
#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>
#include "args/colors.hpp"
#include "args/types.hpp"

using namespace args::tokenizer;

static_assert(
    std::is_trivially_copyable_v<token_t>,
    "The code in this file assumes tokens are cheap to copy");

static_assert(
    sizeof(token_t) < sizeof(void *) * 10,  // NOLINT
    "The code in this file assumes tokens are cheap to copy");

namespace {

[[nodiscard]] auto index_of(std::string_view arg, char const *group_it) -> std::size_t {
    return static_cast<std::size_t>(std::ranges::distance(std::ranges::begin(arg), group_it));
}

[[nodiscard]] auto is_digit(char x) -> bool {
    return x >= '0' && x <= '9';
}

[[nodiscard]] auto is_quote(char x) -> bool {
    return x == '"' || x == '\'';
}

[[nodiscard]] auto is_clumped(char x) -> bool {
    return is_digit(x) || is_quote(x);
}

struct [[nodiscard]] ParseResult {
    token_t token;
    std::string_view remaining;
};

/// c++ lambda notation is so verbose I cannot refrain myself
template <std::optional<ParseResult> (&f)(std::string_view)>
[[nodiscard]] auto lambda(std::string_view arg) -> auto {
    return [=] [[nodiscard]] {
        return f(arg);
    };
}

// The first function to try
[[nodiscard]] auto try_arg(std::string_view arg) -> std::optional<ParseResult> {
    if (arg.starts_with('-')) {
        return std::nullopt;
    }
    return ParseResult{.token = Argument{.value = arg}, .remaining = ""};
}

// From now on we assume the first char is '-'
// The functions below are expected to be called in order, each one assumes the previous one
// has failed its checks

[[nodiscard]] auto try_short_flag(std::string_view arg) -> std::optional<ParseResult> {
    if (arg[1] == '-') {
        return std::nullopt;
    }
    if (!args::detail::rule_assertions::is_valid_first_char(arg[1])) {
        return std::nullopt;
    }
    auto is_long = arg.size() > 2;

    if (is_long && arg[2] != '=') {
        if (is_clumped(arg[2])) {
            // short flag with something != '=' after the char -> clumped case like '-c6' or
            // '-c"some-string"'
            return ParseResult{
                .token = ShortFlag{.raw = arg, .flag = arg[1], .has_equal = false},
                .remaining = std::string_view(&arg[2])
            };
        }
        return std::nullopt;
    }
    auto const remaining = is_long ? std::string_view(&arg[3]) : std::string_view{};
    return ParseResult{
        .token = ShortFlag{.raw = arg, .flag = arg[1], .has_equal = is_long},
        .remaining = remaining
    };
}

[[nodiscard]] auto try_flag_group(std::string_view arg) -> std::optional<ParseResult> {
    if (arg.size() < 3) {
        return std::nullopt;
    }
    if (arg[1] == '-') {
        return std::nullopt;
    }
    auto const is_valid_second_letter = args::detail::rule_assertions::is_valid_first_char(arg[2]);
    auto const *group_it =
        !is_valid_second_letter
            ? std::ranges::end(arg)
            : std::ranges::find_if_not(
                  arg.substr(2), args::detail::rule_assertions::is_valid_first_char);
    if (group_it == std::ranges::end(arg)) {
        return ParseResult{
            .token = GroupFlag{.raw = arg, .group = arg.substr(1), .has_equal = false},
            .remaining = ""
        };
    }
    auto const group_idx = index_of(arg, group_it);
    if (arg[group_idx] != '=') {
        if (is_clumped(arg[group_idx])) {
            // short flag with something != '=' after the char -> clumped case like '-c6' or
            // '-c"some-string"'
            return ParseResult{
                .token =
                    GroupFlag{
                              .raw = arg, .group = arg.substr(1, group_idx - 1), .has_equal = false},
                .remaining = std::string_view(&arg[group_idx])
            };
        }
    }

    return ParseResult{
        .token = GroupFlag{.raw = arg, .group = arg.substr(1, group_idx - 1), .has_equal = true},
        .remaining = std::string_view(&arg[group_idx + 1])
    };
}

[[nodiscard]] auto try_long_flag(std::string_view arg) -> std::optional<ParseResult> {
    if (arg.size() < 3) {
        return std::nullopt;
    }
    if (!args::detail::rule_assertions::is_valid_first_char(arg[2])) {
        return std::nullopt;
    }

    static constexpr auto ddash = 2uz;

    auto const *group_it = std::ranges::find_if_not(
        arg.substr(ddash + 1), args::detail::rule_assertions::is_valid_non_first_char);
    if (group_it == std::ranges::end(arg)) {
        return ParseResult{
            .token = LongFlag{.raw = arg, .flag = arg.substr(ddash), .has_equal = false},
            .remaining = ""
        };
    }
    auto const group_idx = index_of(arg, group_it);
    if (arg[group_idx] != '=') {
        return std::nullopt;
    }
    return ParseResult{
        .token =
            LongFlag{.raw = arg, .flag = arg.substr(ddash, group_idx - ddash), .has_equal = true},
        .remaining = std::string_view(&arg[group_idx + 1])
    };
}

[[nodiscard]] auto try_double_dash(std::string_view arg) -> std::optional<ParseResult> {
    if (arg.size() == 2 && arg[1] == '-') {
        return ParseResult{.token = DoubleDash{}, .remaining = ""};
    }
    return std::nullopt;
}

[[nodiscard]] auto invalid_token_msg(std::string_view arg)
    -> std::expected<std::vector<token_t>, std::string> {
    return std::unexpected(std::format("Invalid token: '{}'", args::color::cyan("{}", arg)));
}

}  // namespace

namespace args::tokenizer {

auto tokenize(std::span<char const *const> src)
    -> std::expected<std::vector<token_t>, std::string> {
    auto tokens = std::vector<token_t>{};
    for (char const *arg : src) {
        auto sp_arg = std::string_view{arg};
        if (sp_arg.empty()) {
            // we allow empty arguments, we just skip them
            continue;
        }
        while (true) {
            // A single '-' is not a valid token. Also this checks ensure
            // the view is exactly one char (non '-') or multiple chars
            if (sp_arg.size() == 1 && sp_arg[0] == '-') {
                return invalid_token_msg(sp_arg);
            }

            auto const parse_result = try_arg(sp_arg)
                                          .or_else(lambda<try_short_flag>(sp_arg))
                                          .or_else(lambda<try_flag_group>(sp_arg))
                                          .or_else(lambda<try_long_flag>(sp_arg))
                                          .or_else(lambda<try_double_dash>(sp_arg));

            if (!parse_result.has_value()) {
                return invalid_token_msg(sp_arg);
            }

            auto const [tok, rem] = parse_result.value();
            tokens.push_back(tok);

            if (rem.empty()) {
                break;
            }
            // If there is still stuff to parse, loop
            sp_arg = rem;
        }
    }
    return tokens;
}

}  // namespace args::tokenizer
