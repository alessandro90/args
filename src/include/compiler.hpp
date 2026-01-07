#ifndef CPP_ARGS_COMPILER_HEADER
#define CPP_ARGS_COMPILER_HEADER

#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include "parsers.hpp"
#include "tokenizer.hpp"
#include "types.hpp"
#include "typetag.hpp"

namespace args::compiler {
namespace detail {

template <auto S>
concept AShortFlag = Spec<decltype(S)> && S.short_form.has_value
                     && is_flag_v<decltype(S)> && !is_flag_with_value_v<decltype(S)>;

template <auto S>
concept AShortFlagWithValue =
    Spec<decltype(S)> && S.short_form.has_value && is_flag_with_value_v<decltype(S)>;

template <auto S>
concept ALongFlag =
    Spec<decltype(S)> && is_flag_v<decltype(S)> && !is_flag_with_value_v<decltype(S)>;

template <auto S>
concept ALongFlagWithValue = Spec<decltype(S)> && is_flag_with_value_v<decltype(S)>;

template <auto S>
concept APositional = Spec<decltype(S)> && is_positional_v<decltype(S)>;

template <Spec S>
[[nodiscard]] consteval auto is_required(S const &s) -> bool {
    if constexpr (is_positional_v<S>) {
        return true;
    } else {
        return s.required;
    }
}

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
        auto const handler = [this, short_flag]<Spec auto S>(ArgValue<S> &item)
                                 requires detail::AShortFlag<S>
        {
            if (item.spec.short_form.value != short_flag.flag) {
                return false;
            }
            item.is_used = true;
            item.value = true;
            return true;
        };

        auto const handler_with_value = [this, short_flag]<Spec auto S>(ArgValue<S> &item)
                                            requires detail::AShortFlagWithValue<S>
        {
            if (item.spec.short_form.value != short_flag.flag) {
                return false;
            }
            m_compiler_state = ParsingShortFlag{.short_flag = short_flag};
            return true;
        };
        bool const handled = try_handle_flag(short_flag.has_equal, handler_with_value, handler);
        if (!handled) {
            return std::format("Cannot find match for flag: '{}'", short_flag.flag);
        }
        return {};
    }

    [[nodiscard]] auto operator()(tokenizer::LongFlag long_flag) -> std::optional<std::string> {
        if (!std::holds_alternative<std::monostate>(m_compiler_state)) {
            return std::format("Cannot parse long flag: '{}'", long_flag.flag);
        }
        auto const handler = [this, long_flag]<Spec auto S>(ArgValue<S> &item)
                                 requires detail::ALongFlag<S>
        {
            if (item.spec.long_form.as_string_view() != long_flag.flag) {
                return false;
            }
            item.is_used = true;
            item.value = true;
            return true;
        };

        auto const handler_with_value = [this, long_flag]<Spec auto S>(ArgValue<S> &item)
                                            requires detail::ALongFlagWithValue<S>
        {
            if (item.spec.long_form.as_string_view() != long_flag.flag) {
                return false;
            }
            m_compiler_state = ParsingLongFlag{.long_flag = long_flag};
            return true;
        };
        bool const handled = try_handle_flag(long_flag.has_equal, handler_with_value, handler);
        if (!handled) {
            return std::format("Cannot find match for flag: '{}'", long_flag.flag);
        }
        return {};
    }

    [[nodiscard]] auto operator()(tokenizer::FlagGroup flag_group) -> std::optional<std::string> {
        auto const flags_nr = flag_group.group.size();
        for (auto const [idx, short_flag] : flag_group.group | std::views::enumerate) {
            bool const is_last =
                static_cast<std::size_t>(idx) == static_cast<std::size_t>(flags_nr - 1uz);
            auto res = (*this)(tokenizer::ShortFlag{
                .flag = short_flag, .has_equal = is_last && flag_group.has_equal});
            if (res.has_value()) {
                return res;
            }
        }
        return {};
    }

    [[nodiscard]] auto operator()(tokenizer::Argument argument) -> std::optional<std::string> {
        auto compile_argument = Overload{
            [&](std::monostate) -> std::optional<std::string> {
                auto error = std::optional<std::string>{};
                auto const handler = [&, counter = 0uz]<Spec auto S>(ArgValue<S> &item) mutable
                    requires detail::APositional<S>
                {
                    if (counter != m_current_positional_index) {
                        ++counter;
                        return false;
                    }
                    ++m_current_positional_index;
                    try_parse_argument(argument, item, error);
                    return true;
                };

                if (!handle_token(handler)) {
                    return std::format(
                        "Cannot find match for positional argument number: '{}'",
                        m_current_positional_index);
                }
                return error;
            },
            [&](ParsingShortFlag short_flag_state) -> std::optional<std::string> {
                auto error = std::optional<std::string>{};
                auto const handler = [&]<Spec auto S>(ArgValue<S> &item)
                                         requires detail::AShortFlagWithValue<S>
                {
                    if (item.spec.short_form.value != short_flag_state.short_flag.flag) {
                        return false;
                    }
                    try_parse_argument(argument, item, error);
                    return true;
                };
                if (!handle_token(handler)) {
                    return std::format(
                        "Cannot find match for flag: '{}'", short_flag_state.short_flag.flag);
                }
                return error;
            },
            [&](ParsingLongFlag long_flag_state) -> std::optional<std::string> {
                auto error = std::optional<std::string>{};
                auto const handler = [&]<Spec auto S>(ArgValue<S> &item)
                                         requires detail::ALongFlagWithValue<S>

                {
                    if (item.spec.long_form.as_string_view() != long_flag_state.long_flag.flag) {
                        return false;
                    }

                    try_parse_argument(argument, item, error);
                    return true;
                };

                if (!handle_token(handler)) {
                    return std::format(
                        "Cannot find match for flag: '{}'", long_flag_state.long_flag.flag);
                }
                return error;
            },
        };
        return std::visit(compile_argument, m_compiler_state);
    }

    /// Verify that the state of the compiler is 'monostate'. If not return an error string
    [[nodiscard]] auto check_correct_final_state() const -> std::optional<std::string> {
        auto const state_checker = Overload{
            [](std::monostate) -> std::optional<std::string> {
                return {};
            },
            [](ParsingLongFlag state) -> std::optional<std::string> {
                return std::format("Missing value for flag: '{}'", state.long_flag.flag);
            },
            [](ParsingShortFlag state) -> std::optional<std::string> {
                return std::format("Missing value for flag: '{}'", state.short_flag.flag);
            }};
        return std::visit(state_checker, m_compiler_state);
    }

private:
    template <typename Handler>
    [[nodiscard]] auto handle_token(Handler handler) -> bool {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (... || [&]() {  // 'or' will execute until the first 'true'
                using arg_type_t = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
                if constexpr (std::is_invocable_v<Handler, arg_type_t &>) {
                    auto &item = std::get<Is>(results);
                    return handler(item);
                } else {
                    return false;  // not callable, keep looping
                }
            }());
        }(std::make_index_sequence<sizeof...(Specs)>());
    }

    template <Spec auto S>
    auto try_parse_argument(
        tokenizer::Argument argument, ArgValue<S> &item, std::optional<std::string> &error)
        -> void {
        using namespace args::detail;
        auto parsed_value =
            parsers::parse<result_type_t<S>>(argument.value.begin(), argument.value.end());
        if (parsed_value.has_value()) {
            item.is_used = true;
            item.value = std::move(parsed_value).value();
            m_compiler_state = std::monostate{};
            return;
        }
        error = std::format(
            "Cannot parse '{}' into '{}'",
            argument.value,
            parsers::type_name(Typetag<result_type_t<S>>{}));
    }

    [[nodiscard]] auto try_handle_flag(
        bool has_equal, auto handler_with_value, auto handler_without_value) -> bool {
        return has_equal ? handle_token(handler_with_value)
                         : handle_token(handler_without_value) || handle_token(handler_with_value);
    }

    std::variant<std::monostate, ParsingShortFlag, ParsingLongFlag> m_compiler_state{};
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
            using arg_type_t = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
            std::size_t positional_argument_count = 0;
            if constexpr (is_positional_v<decltype(arg_type_t::spec)>) {
                ++positional_argument_count;
                if (!r.is_used) {
                    v.push_back(
                        std::format(
                            "Missing positional argument number {}", positional_argument_count));
                }
            } else if constexpr (
                args::detail::IsAFlag<arg_type_t::spec> && is_required(arg_type_t::spec)) {
                if (!r.is_used) {
                    auto err = std::format(
                        "Missing required flag. Long form: '{}'.",
                        arg_type_t::spec.long_form.as_string_view());
                    if (arg_type_t::spec.short_form.has_value) {
                        err += std::format(" Short form: '{}'.", arg_type_t::spec.short_form.value);
                    }
                    v.push_back(std::move(err));
                }
            }
        }());
    }(std::make_index_sequence<sizeof...(Specs)>());
    return v;
}

/// Applies only to flags with values for which the default is a callable.
///
/// If the flag was not used, set its value to the result of the invocation
/// of `default_value`
template <Spec auto... Specs>
auto assign_callable_defaults(std::tuple<ArgValue<Specs>...> &results) -> void {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (..., [&]() {
            auto &r = std::get<Is>(results);
            using arg_type_t = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
            using S = decltype(arg_type_t::spec);
            if constexpr (is_flag_with_value_v<S> && std::is_invocable_v<typename S::value_t>) {
                if (!r.is_used) {
                    r.value = arg_type_t::spec.default_value();
                }
            }
        }());
    }(std::make_index_sequence<sizeof...(Specs)>());
}

}  // namespace detail

template <std::size_t Extent, Str Usage, Str Description, Spec auto... Specs>
[[nodiscard]] constexpr auto compile(
    std::span<tokenizer::token_t const, Extent> tokens, Rules<Usage, Description, Specs...>)
    -> std::expected<Args<Specs...>, std::string> {
    auto token_compiler = detail::TokenCompiler<Specs...>{};
    for (auto const token : tokens) {
        auto err_msg = std::visit(token_compiler, token);
        if (err_msg.has_value()) {
            return std::unexpected(std::move(err_msg).value());
        }
    }
    auto state_error = token_compiler.check_correct_final_state();
    if (state_error.has_value()) {
        return std::unexpected(std::move(state_error).value());
    }
    auto const missing_args = detail::verify_required_args(token_compiler.results);
    if (!missing_args.empty()) {
        return std::unexpected(
            missing_args | std::views::join_with('\n') | std::ranges::to<std::string>());
    }
    detail::assign_callable_defaults(token_compiler.results);
    return Args{std::move(token_compiler).results};
}

}  // namespace args::compiler

#endif
