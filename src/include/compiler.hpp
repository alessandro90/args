#ifndef CPP_ARGS_COMPILER_HEADER
#define CPP_ARGS_COMPILER_HEADER

#include <charconv>
#include <concepts>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include "tokenizer.hpp"
#include "types.hpp"

namespace args::compiler {
namespace detail {

template <auto S>
concept ShortFlagCompatible = Spec<decltype(S)> && S.short_form.has_value && is_flag_v<decltype(S)>;

template <auto S>
concept ShortFlagWithValueCompatible =
    Spec<decltype(S)> && S.short_formhas_value && is_flag_with_value_v<decltype(S)>;

template <auto S>
concept LongFlagCompatible = Spec<decltype(S)> && is_flag_v<decltype(S)>;

template <auto S>
concept LongFlagWithValueCompatible = Spec<decltype(S)> && is_flag_with_value_v<decltype(S)>;

struct [[nodiscard]] ParsingPositional {};

struct [[nodiscard]] ParsingShortFlag {
    tokenizer::ShortFlag short_flag;
};

struct [[nodiscard]] ParsingLongFlag {
    tokenizer::LongFlag long_flag;
};

template <typename... F>
struct [[nodiscard]] Overload: F... {
    using F::operator()...;
};

template <Spec auto... Specs>
struct [[nodiscard]] TokenCompiler {
    std::tuple<ArgValue<Specs>...> results{};

    [[nodiscard]] auto operator()(tokenizer::ShortFlag short_flag) -> std::optional<std::string> {
        if (!std::holds_alternative<std::monostate>(m_compiler_state)) {
            return std::format("Cannot parse short flag: '{}'", short_flag.flag);
        }
        auto const short_flag_selector = [&]<Spec auto S>(ArgValue<S> const &x)
                                             requires detail::ShortFlagCompatible<S>
        {
            return x.spec.short_form.value == short_flag.flag;
        };
        auto const short_flag_action = [this]<Spec auto S>(ArgValue<S> &item)
                                           requires detail::ShortFlagCompatible<S>
        {
            item.is_used = true;
            item.value = true;
        };

        auto const short_flag_with_value_selector =
            [&]<Spec auto S>(ArgValue<S> const &x) requires detail::ShortFlagWithValueCompatible<S>
        {
            return x.spec.short_form.value == short_flag.flag;
        };
        auto const short_flag_with_value_action =
            [this, short_flag]<Spec auto S>(ArgValue<S> &)
                requires detail::ShortFlagWithValueCompatible<S>
        {
            m_compiler_state = ParsingShortFlag{.short_flag = short_flag};
        };
        bool const handled =
            handle_token(short_flag_selector, short_flag_action)
            || handle_token(short_flag_with_value_selector, short_flag_with_value_action);
        if (!handled) {
            return std::format("Cannot find match for flag: '{}'", short_flag.flag);
        }
        return {};
    }

    [[nodiscard]] auto operator()(tokenizer::LongFlag long_flag) -> std::optional<std::string> {
        if (!std::holds_alternative<std::monostate>(m_compiler_state)) {
            return std::format("Cannot parse long flag: '{}'", long_flag.flag);
        }
        auto const long_flag_selector = [&]<Spec auto S>(ArgValue<S> const &x)
                                            requires detail::LongFlagCompatible<S>
        {
            return x.spec.long_form == long_flag.flag;
        };
        auto const long_flag_action = [this]<Spec auto S>(ArgValue<S> &item)
                                          requires detail::ShortFlagCompatible<S>
        {
            item.is_used = true;
            item.value = true;
        };

        auto const long_flag_with_value_selector =
            [&]<Spec auto S>(ArgValue<S> const &x) requires detail::LongFlagWithValueCompatible<S>
        {
            return x.spec.long_form.value == long_flag.flag;
        };
        auto const long_flag_with_value_action = [this, long_flag]<Spec auto S>(ArgValue<S> &)
                                                     requires detail::LongFlagWithValueCompatible<S>
        {
            m_compiler_state = ParsingLongFlag{.long_flag = long_flag};
        };
        bool const handled =
            handle_token(long_flag_selector, long_flag_action)
            || handle_token(long_flag_with_value_selector, long_flag_with_value_action);
        if (!handled) {
            return std::format("Cannot find match for flag: '{}'", long_flag.flag);
        }
        return {};
    }

    [[nodiscard]] auto operator()(tokenizer::FlagGroup flag_group) -> std::optional<std::string> {
        for (auto const short_flag : flag_group.group) {
            auto res = (*this)(tokenizer::ShortFlag{.flag = short_flag});
            if (res.has_value()) {
                return res;
            }
        }
        return {};
    }

    [[nodiscard]] auto operator()(tokenizer::Argument argument) -> std::optional<std::string> {
        auto compile_argument = Overload{
            [&](std::monostate) -> std::optional<std::string> {
                // TODO: compile single argument
                m_compiler_state = ParsingPositional{};
                return "not implemented yet";
            },
            [&](ParsingShortFlag short_flag_state) -> std::optional<std::string> {
                auto const selector =
                    [&]<Spec auto S>(ArgValue<S> const &x)
                        requires detail::ShortFlagWithValueCompatible<S>
                                 && std::same_as<typename decltype(S)::value_t, int>
                {
                    return x.spec.short_form.value == short_flag_state.short_flag;
                };
                auto error = std::optional<std::string>{};
                auto const action = [&]<Spec auto S>(ArgValue<S> &item)
                                        requires detail::ShortFlagWithValueCompatible<S>
                                                 && std::same_as<typename decltype(S)::value_t, int>
                {
                    // assume it is always an integer for now
                    int value{};
                    auto const parsed =
                        std::from_chars(argument.value.begin(), argument.value.end(), value);
                    if (parsed.ec == std::errc{}) {
                        item.is_used = true;
                        item.value = value;
                        m_compiler_state = std::monostate{};
                    } else {
                        error = std::format("Cannot parse '{}' into int", argument.value);
                    }
                };
                if (!handle_token(selector, action)) {
                    return std::format(
                        "Cannot find match for flag: '{}'", short_flag_state.short_flag.flag);
                }
                return error;
            },
            [this](ParsingLongFlag) -> std::optional<std::string> {
                return "not implemented yet";
            },
            [this](ParsingPositional) -> std::optional<std::string> {
                // TODO: signal error
                return "not implemented yet";
            },
        };
        return std::visit(compile_argument, m_compiler_state);
    }

private:
    template <typename Selector, typename Action>
    [[nodiscard]] auto handle_token(Selector selector, Action action) -> bool {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (... || [&]() {  // 'or' will execute until the first 'true'
                using ArgType = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
                if constexpr (std::is_invocable_v<Selector, ArgType const &>) {
                    auto &item = std::get<Is>(results);
                    if (!selector(item)) {
                        return false;  // no match, keep looping
                    }
                    action(item);
                    return true;
                } else {
                    return false;  // not callable, keep looping
                }
            }());
        }(std::make_index_sequence<sizeof...(Specs)>());
    }

    std::variant<std::monostate, ParsingShortFlag, ParsingLongFlag, ParsingPositional>
        m_compiler_state{};
    std::size_t m_current_positional_index{};
};

template <Spec auto... Specs>
[[nodiscard]] auto verify_required_args(std::tuple<ArgValue<Specs>...> const &results)
    -> std::vector<std::string> {
    // NOTE: an empty vec (meaning no errors) does not allocate, so we are good
    auto v = std::vector<std::string>{};
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (..., [&]() {
            auto const &r = std::get<Is>(results);
            using ArgType = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
            std::size_t positional_argument_count = 0;
            if constexpr (is_positional_v<decltype(ArgType::spec)>) {
                ++positional_argument_count;
                if (!r.is_used) {
                    v.push_back(
                        std::format(
                            "Missing positional argument number {}", positional_argument_count));
                }
            } else if constexpr (args::detail::IsAnyFlag<ArgType::spec> && ArgType::spec.required) {
                if (!r.is_used) {
                    auto err = std::format(
                        "Missing required flag. Long form: '{}'.",
                        ArgType::spec.long_form.as_string_view());
                    if (ArgType::spec.short_form.has_value) {
                        err += std::format(" Short form: '{}'.", ArgType::spec.short_form.value);
                    }
                    v.push_back(std::move(err));
                }
            }
        }());
    }(std::make_index_sequence<sizeof...(Specs)>());
    return v;
}

}  // namespace detail

// NOTE: span may be just a range. Would allow to process tokens as a stream without dynamic
// allocation

template <std::size_t Extent, Spec auto... Specs>
[[nodiscard]] constexpr auto compile(
    std::span<tokenizer::Token const, Extent> tokens, Rules<Specs...>)
    -> std::expected<Args<Specs...>, std::string> {
    auto token_compiler = detail::TokenCompiler<Specs...>{};
    for (auto const token : tokens) {
        auto const err_msg = std::visit(token_compiler, token);
        if (err_msg.has_value()) {
            return std::unexpected(err_msg.value());
        }
    }
    auto const missing_args = detail::verify_required_args(token_compiler.results);
    if (!missing_args.empty()) {
        return std::unexpected(
            missing_args | std::views::join_with('\n') | std::ranges::to<std::string>());
    }
    return Args{std::move(token_compiler).results};
}

}  // namespace args::compiler

#endif
