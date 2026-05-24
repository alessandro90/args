#ifndef ARGS_COMPILER_HEADER
#define ARGS_COMPILER_HEADER

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include "parsers.hpp"
#include "tokenizer.hpp"
#include "type_helpers.hpp"
#include "types.hpp"
#include "validators.hpp"

namespace args::compiler {
namespace detail {

static constexpr auto help_str = std::string_view{"help"};
using TokenCompileResult = std::variant<std::monostate, Help, Error>;

template <auto S>
concept AShortFlag =
    S.short_form.has_value && is_flag_v<decltype(S)> && !is_flag_with_value_v<decltype(S)>;

template <auto S>
concept AShortFlagWithValue = S.short_form.has_value && is_flag_with_value_v<decltype(S)>;

template <auto S>
concept ALongFlag = is_flag_v<decltype(S)> && !is_flag_with_value_v<decltype(S)>;

template <auto S>
concept ALongFlagWithValue = is_flag_with_value_v<decltype(S)>;

template <auto S>
concept APositional = is_positional_v<decltype(S)>;

template <auto S>
concept ASubcommand = is_subcommand_v<decltype(S)>;

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

template <auto S>
requires args::detail::PositionalVariadic<S>
auto assign_parsed_value(ArgValue<S> &item, args::detail::parse_type_t<S> value) -> void {
    item.value.push_back(std::move(value));
}

template <auto S>
requires args::detail::is_repeatable_v<S>
auto assign_parsed_value(ArgValue<S> &item, args::detail::parse_type_t<S> value) -> void {
    std::visit(
        [&](auto arg) {
            if constexpr (args::detail::IsVector<decltype(arg)>::value) {
                item.value.append_range(std::move(arg));
            } else {
                item.value.push_back(std::move(arg));
            }
        },
        std::move(value));
}

template <auto S>
auto assign_parsed_value(ArgValue<S> &item, args::detail::parse_type_t<S> value) -> void {
    item.value = std::move(value);
}

template <Str Usage, Str Description, auto... Specs>
struct [[nodiscard]] TokenCompiler {
private:
    enum class [[nodiscard]] Mode : std::uint8_t {
        Normal,
        PositionalOnly,
        PositionalOnlySkipNext
    };

public:
    explicit TokenCompiler(Rules<Usage, Description, Specs...> compile_rules)
        : m_compile_rules{compile_rules} {}

    std::tuple<ArgValue<Specs>...> results{};

    [[nodiscard]] auto operator()(tokenizer::Argument argument) -> TokenCompileResult {
        if (m_mode == Mode::PositionalOnlySkipNext) {
            m_mode = Mode::PositionalOnly;
            return {};
        }
        auto const defer = args::detail::Defer{[this] {
            m_is_first_argument = false;
        }};
        auto compile_argument = Overload{
            compile_positional_argument(argument),
            compile_valued_short_flag(argument),
            compile_valued_long_flag(argument),
        };
        return std::visit(compile_argument, m_compiler_state);
    }

    [[nodiscard]] auto operator()(auto flag) -> TokenCompileResult {
        if (m_mode == Mode::Normal) {
            return compile_flag(flag);
        }
        auto result = (*this)(tokenizer::Argument{.value = flag.raw});
        if (flag.has_equal) {
            // if the flag is like --flag=3 or -f=3 we need to skip the next
            // token because it will be '3', but we already parsed that using 'raw'.
            // '--flag 3' is different because we treat it as 2 different positionals
            // if not quoted and just as a single value if quoted (such a value would be
            // an invalid flag but in this case we parse it like a string basically)
            m_mode = Mode::PositionalOnlySkipNext;
        }
        return result;
    }

    [[nodiscard]] auto operator()(tokenizer::DoubleDash) -> TokenCompileResult {
        auto const defer = args::detail::Defer{[this] {
            m_is_first_argument = false;
        }};
        if (!std::holds_alternative<std::monostate>(m_compiler_state)) {
            return Error{"Cannot begin a positional only mode"};
        }
        m_mode = Mode::PositionalOnly;
        return {};
    }

    [[nodiscard]] auto compile_flag(tokenizer::ShortFlag short_flag) -> TokenCompileResult {
        if (!std::holds_alternative<std::monostate>(m_compiler_state)) {
            return Error{std::format("Cannot parse short flag: '{}'", short_flag.flag)};
        }
        auto const handler = [short_flag]<auto S>(ArgValue<S> &item) requires detail::AShortFlag<S>
        {
            if (item.spec.short_form.value != short_flag.flag) {
                return false;
            }
            item.is_used = true;
            item.value = true;
            if constexpr (requires { item.count; }) {
                item.count += 1;
            }
            return true;
        };

        auto const handler_with_value = [this, short_flag]<auto S>(ArgValue<S> &item)
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
            return Error{std::format("Cannot find match for flag: '{}'", short_flag.flag)};
        }
        return {};
    }

    [[nodiscard]] auto compile_flag(tokenizer::LongFlag long_flag) -> TokenCompileResult {
        if (!std::holds_alternative<std::monostate>(m_compiler_state)) {
            return Error{std::format("Cannot parse long flag: '{}'", long_flag.flag)};
        }

        auto error = std::optional<std::string>{};
        auto const subcommand_handler =
            [this, &error, long_flag]<auto S>(ArgValue<S> &item, std::size_t tuple_index)
                requires(detail::ASubcommand<S> && S.is_flag)
        {
            if (item.spec.name.as_string_view() != long_flag.flag) {
                return false;
            }
            if (long_flag.has_equal) {
                error = std::format(
                    "Subcommand flag does not support '='. Flag name is: '{}'", long_flag.flag);
                return false;
            }
            if (!m_is_first_argument) {
                error = std::format(
                    "Subcommand flag must be the first parsed argument. Flag name is: '{}'",
                    long_flag.flag);
                return false;
            }
            parse_and_validate_argument(tokenizer::Argument{.value = long_flag.flag}, item, error);
            if (error.has_value()) {
                return false;
            }
            m_subcommand_tuple_index = tuple_index;
            return true;
        };
        // If this is a flagged subcommand, we return immediately
        if (handle_token(subcommand_handler)) {
            return {};
        }
        if (error.has_value()) {
            return Error{std::move(error).value()};
        }

        auto help = std::optional<std::string_view>{};
        auto const handler = [this, &help, long_flag]<auto S>(ArgValue<S> &item)
                                 requires detail::ALongFlag<S>
        {
            if (long_flag.flag == help_str) {
                // help requested: skip everything else and return
                help = m_compile_rules.help();
                return true;
            }
            if (item.spec.long_form.as_string_view() != long_flag.flag) {
                return false;
            }
            item.is_used = true;
            item.value = true;
            if constexpr (requires { item.count; }) {
                item.count += 1;
            }
            return true;
        };

        auto const handler_with_value = [this, long_flag]<auto S>(ArgValue<S> &item)
                                            requires detail::ALongFlagWithValue<S>
        {
            if (item.spec.long_form.as_string_view() != long_flag.flag) {
                return false;
            }
            m_compiler_state = ParsingLongFlag{.long_flag = long_flag};
            return true;
        };

        if (!try_handle_flag(long_flag.has_equal, handler_with_value, handler)) {
            return Error{std::format("Cannot find match for flag: '{}'", long_flag.flag)};
        }
        if (help.has_value()) {
            return Help{help.value()};
        }
        return {};
    }

    [[nodiscard]] auto compile_flag(tokenizer::FlagGroup flag_group) -> TokenCompileResult {
        auto const flags_nr = flag_group.group.size();
        for (auto const [idx, short_flag] : flag_group.group | std::views::enumerate) {
            bool const is_last =
                static_cast<std::size_t>(idx) == static_cast<std::size_t>(flags_nr - 1uz);
            auto res = (*this)(tokenizer::ShortFlag{
                .raw = flag_group.raw,
                .flag = short_flag,
                .has_equal = is_last && flag_group.has_equal});
            if (std::holds_alternative<Error>(res)) {
                return res;
            }
        }
        return {};
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

    template <typename Compiler, std::size_t Extent>
    [[nodiscard]] constexpr auto compile_subcommand(
        std::span<tokenizer::token_t const, Extent> tokens,
        Compiler compiler,
        std::size_t subcommand_tuple_index) -> TokenCompileResult {
        auto error_or_help = TokenCompileResult{};
        auto const handled = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (... || [&]() {  // 'or' will execute until the first 'true'
                if (Is != subcommand_tuple_index) {
                    return false;
                }
                using arg_value_t = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
                if constexpr (is_subcommand_v<decltype(arg_value_t::spec)>) {
                    auto &subcommand = std::get<Is>(results);
                    auto subcommand_result = compiler(
                        tokens, arg_value_t::spec.rules, arg_value_t::spec.mutually_exclusive);
                    if (has_args(subcommand_result)) {
                        subcommand.subcommands = get_args(std::move(subcommand_result));
                    } else if (has_error(subcommand_result)) {
                        error_or_help = get_error(std::move(subcommand_result));
                    } else {
                        error_or_help = get_help(std::move(subcommand_result));
                    }
                    return true;
                }
                return false;
            }());
        }(std::make_index_sequence<sizeof...(Specs)>());
        if (!handled) {
            return Error{.message = "Cannot found subcommand"};
        }
        return error_or_help;
    }

    [[nodiscard]] auto subcommand_index() -> std::optional<std::size_t> {
        return m_subcommand_tuple_index;
    }

private:
    template <typename Handler>
    [[nodiscard]] auto handle_token(Handler handler) -> bool {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (... || [&]() {  // 'or' will execute until the first 'true'
                using arg_type_t = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
                if constexpr (std::is_invocable_v<Handler, arg_type_t &>) {
                    return handler(std::get<Is>(results));
                } else if constexpr (std::is_invocable_v<Handler, arg_type_t &, std::size_t>) {
                    return handler(std::get<Is>(results), Is);
                } else {
                    return false;  // not callable, keep looping
                }
            }());
        }(std::make_index_sequence<sizeof...(Specs)>());
    }

    template <auto S>
    auto parse_and_validate_argument(
        tokenizer::Argument argument, ArgValue<S> &item, std::optional<std::string> &error)
        -> void {
        using namespace args::detail;
        using namespace args::parsers;
        auto parsed_value = Parser<parse_type_t<S>>::parse(argument.value);
        if (parsed_value.has_value()) {
            if constexpr (
                !is_positional_variadic_v<S> && !is_repeatable_v<S>
                && args::detail::HasValidator<S>) {
                auto validation = S.validator(parsed_value.value());
                if (!validation.has_value()) {
                    error = std::move(validation).error();
                    return;
                }
            }
            item.is_used = true;
            assign_parsed_value(item, std::move(parsed_value).value());
            m_compiler_state = std::monostate{};
            return;
        }
        error = std::format(
            "Cannot parse '{}' into '{}'", argument.value, Parser<parse_type_t<S>>::type_name());
    }

    [[nodiscard]] auto try_handle_flag(
        bool has_equal, auto handler_with_value, auto handler_without_value) -> bool {
        return has_equal ? handle_token(handler_with_value)
                         : handle_token(handler_without_value) || handle_token(handler_with_value);
    }

    [[nodiscard]] auto compile_positional_argument(tokenizer::Argument argument) {
        return [this, argument](std::monostate) -> TokenCompileResult {
            auto error = std::optional<std::string>{};
            auto const positional_handler = [&, counter = 0uz]<auto S>(ArgValue<S> &item) mutable
                requires detail::APositional<S>
            {
                if constexpr (!S.variadic) {
                    if (counter != m_current_positional_index) {
                        ++counter;
                        return false;
                    }
                    ++m_current_positional_index;
                }
                parse_and_validate_argument(argument, item, error);
                return !error.has_value();
            };

            auto const subcommand_handler =
                [&]<auto S>(ArgValue<S> &item, std::size_t tuple_index) mutable
                requires detail::ASubcommand<S>
            {
                if (!m_is_first_argument) {
                    return false;
                }
                auto cloned_item = item;
                parse_and_validate_argument(argument, cloned_item, error);
                if (error.has_value()) {
                    return false;
                }
                if (cloned_item.value != S.name.as_string_view()) {
                    return false;
                }
                item = std::move(cloned_item);
                m_subcommand_tuple_index = tuple_index;
                return true;
            };

            if (!handle_token(subcommand_handler) && !handle_token(positional_handler)) {
                auto
                        msg =
                            std::format(
                                "Cannot find match for positional argument number '{}' named '{}' "
                                "with " "provided '{}'",
                                m_current_positional_index,
                                nth_positional_argument_name<Specs...>(m_current_positional_index),
                                argument.value);
                return Error{std::move(msg)};
            }
            if (error.has_value()) {
                return Error{std::move(error).value()};
            }
            return {};
        };
    }

    [[nodiscard]] auto compile_valued_short_flag(tokenizer::Argument argument) {
        return [this, argument](ParsingShortFlag short_flag_state) -> TokenCompileResult {
            auto error = std::optional<std::string>{};
            auto const handler = [&]<auto S>(ArgValue<S> &item)
                                     requires detail::AShortFlagWithValue<S>
            {
                if (item.spec.short_form.value != short_flag_state.short_flag.flag) {
                    return false;
                }
                parse_and_validate_argument(argument, item, error);
                return true;
            };
            if (!handle_token(handler)) {
                return Error{std::format(
                    "Cannot find match for flag: '{}'", short_flag_state.short_flag.flag)};
            }

            if (error.has_value()) {
                return Error{std::move(error).value()};
            }
            return {};
        };
    }

    [[nodiscard]] auto compile_valued_long_flag(tokenizer::Argument argument) {
        return [this, argument](ParsingLongFlag long_flag_state) -> TokenCompileResult {
            auto error = std::optional<std::string>{};
            auto const handler = [&]<auto S>(ArgValue<S> &item)
                                     requires detail::ALongFlagWithValue<S>

            {
                if (item.spec.long_form.as_string_view() != long_flag_state.long_flag.flag) {
                    return false;
                }

                parse_and_validate_argument(argument, item, error);
                return true;
            };

            if (!handle_token(handler)) {
                return Error{std::format(
                    "Cannot find match for flag: '{}'", long_flag_state.long_flag.flag)};
            }
            if (error.has_value()) {
                return Error{std::move(error).value()};
            }
            return {};
        };
    }

    Rules<Usage, Description, Specs...> m_compile_rules;
    std::variant<std::monostate, ParsingShortFlag, ParsingLongFlag> m_compiler_state{};
    std::size_t m_current_positional_index{};
    bool m_is_first_argument{true};
    std::optional<std::size_t> m_subcommand_tuple_index{};
    Mode m_mode{Mode::Normal};
};

template <auto... Specs>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
[[nodiscard]] auto verify_required_args(std::tuple<ArgValue<Specs>...> const &results)
    -> std::vector<std::string> {
    // NOTE: an empty vec (meaning no errors) does not allocate, so we are good
    auto v = std::vector<std::string>{};
    std::size_t positional_argument_count = 0;
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (..., [&]() {
            auto const &r = std::get<Is>(results);
            using arg_type_t = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
            if constexpr (!is_subcommand_v<decltype(arg_type_t::spec)>) {
                if constexpr (is_positional_v<decltype(arg_type_t::spec)>) {
                    ++positional_argument_count;
                    if (!r.is_used && arg_type_t::spec.required) {
                        v.push_back(
                            std::format(
                                "Missing positional argument number {}",
                                positional_argument_count));
                    }
                } else if constexpr (args::detail::IsAFlag<arg_type_t::spec>) {
                    if constexpr (arg_type_t::spec.required) {
                        if (!r.is_used) {
                            auto err = std::format(
                                "Missing required flag. Long form: '{}'.",
                                arg_type_t::spec.long_form.as_string_view());
                            if (arg_type_t::spec.short_form.has_value) {
                                err += std::format(
                                    " Short form: '{}'.", arg_type_t::spec.short_form.value);
                            }
                            v.push_back(std::move(err));
                        }
                    }
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
template <auto... Specs>
auto assign_defaults_to_unused(std::tuple<ArgValue<Specs>...> &results) -> void {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (..., [&]() {
            auto &r = std::get<Is>(results);
            using arg_type_t = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
            using S_t = decltype(arg_type_t::spec);
            if constexpr (is_flag_with_value_v<S_t> && std::is_invocable_v<typename S_t::value_t>) {
                if (!r.is_used) {
                    r.value = arg_type_t::spec.default_value();
                }
            }
        }());
    }(std::make_index_sequence<sizeof...(Specs)>());
}

template <auto... Specs>
[[nodiscard]] auto val_var_pos_and_rep(std::tuple<ArgValue<Specs>...> &results)
    -> std::expected<void, std::vector<validator_error_t>> {
    auto validation_errors = std::vector<validator_error_t>{};
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (..., [&]() {
            auto &r = std::get<Is>(results);
            using arg_type_t = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
            if constexpr (
                args::detail::is_positional_variadic_v<arg_type_t::spec>
                || args::detail::is_repeatable_v<arg_type_t::spec>) {
                if (r.is_used) {
                    auto res = arg_type_t::spec.validator(r.value);
                    if (!res.has_value()) {
                        validation_errors.push_back(std::move(res).error());
                    }
                }
            }
        }());
    }(std::make_index_sequence<sizeof...(Specs)>());
    if (!validation_errors.empty()) {
        return std::unexpected{validation_errors};
    }
    return {};
}

}  // namespace detail

template <std::size_t Extent, Str Usage, Str Description, auto... Specs, auto... Gg>
[[nodiscard]] constexpr auto compile(
    std::span<tokenizer::token_t const, Extent> tokens,
    Rules<Usage, Description, Specs...> compile_rules,
    MutuallyExclusiveGroups<Gg...> mutually_exclusive) -> compile_result_t<Specs...> {
    auto token_compiler = detail::TokenCompiler{compile_rules};
    for (auto const [index, token] : std::views::enumerate(tokens)) {
        auto tok_compile_result = std::visit(token_compiler, token);
        if (std::holds_alternative<Help>(tok_compile_result)) {
            return compile_result_t<Specs...>{std::get<Help>(std::move(tok_compile_result))};
        }
        if (std::holds_alternative<Error>(tok_compile_result)) {
            return compile_result_t<Specs...>{std::get<Error>(std::move(tok_compile_result))};
        }
        if (token_compiler.subcommand_index().has_value()) {
            if (tokens.size() <= static_cast<std::size_t>(index)) {
                return Error{.message = "Missing tokens to parse subcommand"};
            }
            auto failed_subcommand = token_compiler.compile_subcommand(
                tokens.subspan(static_cast<std::size_t>(index + 1)),
                []<std::size_t SpanExtent>(
                    std::span<tokenizer::token_t const, SpanExtent> tks, auto rs, auto me) {
                    return compile(tks, rs, me);
                },
                token_compiler.subcommand_index().value());
            if (std::holds_alternative<Help>(failed_subcommand)) {
                return compile_result_t<Specs...>{std::get<Help>(std::move(failed_subcommand))};
            }
            if (std::holds_alternative<Error>(failed_subcommand)) {
                return compile_result_t<Specs...>{std::get<Error>(std::move(failed_subcommand))};
            }
            return Args{std::move(token_compiler).results};
        }
    }

    if (auto state_error = token_compiler.check_correct_final_state(); state_error.has_value()) {
        return Error{.message = std::move(state_error).value()};
    }

    if (auto const missing_args = detail::verify_required_args(token_compiler.results);
        !missing_args.empty()) {
        return Error{
            .message = missing_args | std::views::join_with('\n') | std::ranges::to<std::string>()};
    }

    detail::assign_defaults_to_unused(token_compiler.results);

    if (auto posvar_and_rep_val_result = detail::val_var_pos_and_rep(token_compiler.results);
        !posvar_and_rep_val_result.has_value()) {
        return Error{
            .message = std::move(posvar_and_rep_val_result).error()
                       | std::views::join_with(std::string_view{"\n"})
                       | std::ranges::to<std::string>()};
    }
    auto result_args = Args{std::move(token_compiler).results};
    if (auto invalid_mutually_exclusive =
            args::detail::check_mutually_exclusive_group_satisfied(result_args, mutually_exclusive);
        invalid_mutually_exclusive.has_value()) {
        return Error{.message = std::move(invalid_mutually_exclusive).value()};
    }
    return result_args;
}

}  // namespace args::compiler

#endif
