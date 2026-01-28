#ifndef CPP_ARGS_TYPES
#define CPP_ARGS_TYPES

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include "type_helpers.hpp"
#include "typetag.hpp"
#include "validators.hpp"

namespace args {

template <typename T, auto... Args>
inline constexpr auto Lazy = [] {
    return T{std::move(Args)...};
};

template <typename T, auto... Args>
using lazy_t = decltype(Lazy<T, Args...>);

template <typename T>
using vec_t = lazy_t<std::vector<T>>;

template <typename T>
struct [[nodiscard]] Opt {
    bool has_value;
    T value;

    static consteval auto empty() -> Opt {
        return Opt{.has_value = false, .value = T{}};
    }

    static consteval auto with(T value) -> Opt {
        return Opt{.has_value = true, .value = value};
    }

    [[nodiscard]] constexpr auto operator==(Opt const &) const -> bool = default;
};

template <typename T>
concept Trivial = std::is_trivial_v<T>;

template <std::size_t N>
struct [[nodiscard]] Str {
    std::array<char, N + 1> chars{};

    consteval Str() noexcept = default;

    consteval Str(char const (&s)[N + 1]) {  // NOLINT
        std::ranges::copy(s, chars.begin());
    }

    [[nodiscard]] constexpr auto as_string_view() const -> std::string_view {
        return std::string_view{chars.data()};
    }

    [[nodiscard]] static constexpr auto is_empty() noexcept -> bool {
        return N == 0;
    }

    template <std::size_t M>
    [[nodiscard]] constexpr auto operator==(Str<M> const &rhs) -> bool {
        return N == M && chars == rhs.chars;
    }

    [[nodiscard]] constexpr auto operator==(std::string_view rhs) -> bool {
        return as_string_view() == rhs;
    }

    [[nodiscard]] constexpr auto operator==(std::string rhs) -> bool {
        return as_string_view() == rhs;
    }
};

template <std::size_t N>
Str(char const (&s)[N]) -> Str<N - 1>;  // NOLINT

template <Str X>
consteval auto operator""_str() -> decltype(X) {
    return X;
}

inline constexpr auto empty = ""_str;

using str_t = lazy_t<std::string>;

using strv_t = lazy_t<std::string_view>;

template <std::size_t N, std::size_t M = 0>
struct [[nodiscard]] Flag {
    Str<N> long_form;
    Opt<char> short_form{Opt<char>::empty()};
    bool default_value{};
    bool required{};
    Str<M> help{};

    using value_t = bool;
};

template <Trivial Value, std::size_t N, std::size_t M = 0, AValidator V = always_t>
struct [[nodiscard]] FlagWithValue {
    Str<N> long_form;
    Opt<char> short_form{Opt<char>::empty()};
    /// Used if the flag is missing
    Value default_value{};
    bool required{};
    Str<M> help{};
    V validator{always};
    using value_t = Value;
};

template <Str X>
consteval auto operator""_flag() -> decltype(X) {
    return X;
}

template <Str X>
requires(X.chars.size() == 2 && X.chars[1] == '\0')
consteval auto operator""_short_flag() -> Opt<char> {
    return Opt<char>::with(X.chars[0]);
}

template <
    std::default_initializable P,
    std::size_t N = 0,
    std::size_t M = 0,
    AValidator V = always_t>
struct [[nodiscard]] Positional {
    Typetag<P> type;
    Str<N> name{};
    Str<M> help{};
    bool required{};
    bool variadic{};
    V validator{always};

    using value_t = P;
};

template <Str Usage, Str Description, auto... Specs>
requires(sizeof...(Specs) > 0)
struct [[nodiscard]] Rules;

template <
    std::size_t N,
    std::size_t M = 0,
    Str Usage = empty,
    Str Description = empty,
    auto... Specs>
struct [[nodiscard]] Subcommand {
    Str<N> name{};
    Str<M> help{};
    Rules<Usage, Description, Specs...> rules{};

    // using value_t = strv_t;
    using value_t = std::string_view;
};

template <typename>
struct IsFlag: std::false_type {};

template <std::size_t N, std::size_t M>
struct IsFlag<Flag<N, M>>: std::true_type {};

template <typename T>
inline constexpr bool is_flag_v = IsFlag<std::remove_cvref_t<T>>::value;

template <typename>
struct IsFlagWithValue: std::false_type {};

template <Trivial Value, std::size_t N, std::size_t M, AValidator V>
struct IsFlagWithValue<FlagWithValue<Value, N, M, V>>: std::true_type {};

template <typename P>
struct IsPositional: std::false_type {};

template <typename P, std::size_t N, std::size_t M, AValidator V>
struct IsPositional<Positional<P, N, M, V>>: std::true_type {};

template <typename P>
inline constexpr bool is_positional_v = IsPositional<std::remove_cvref_t<P>>::value;

template <typename T>
inline constexpr bool is_flag_with_value_v = IsFlagWithValue<std::remove_cvref_t<T>>::value;

template <typename>
struct IsSubcommand: std::false_type {};

template <std::size_t N, std::size_t M, Str Usage, Str Description, auto... Specs>
struct IsSubcommand<Subcommand<N, M, Usage, Description, Specs...>>: std::true_type {};

template <typename S>
inline constexpr bool is_subcommand_v = IsSubcommand<std::remove_cvref_t<S>>::value;

namespace detail {

template <auto S>
struct IsPositionalVariadic: std::false_type {};

template <Positional P>
struct IsPositionalVariadic<P>: std::conditional_t<P.variadic, std::true_type, std::false_type> {};

template <auto S>
inline constexpr auto is_positional_variadic_v = IsPositionalVariadic<S>::value;

template <auto S>
concept PositionalVariadic = is_positional_variadic_v<S>;

template <auto S>
concept IsAFlag = is_flag_v<decltype(S)> || is_flag_with_value_v<decltype(S)>;

template <auto S1, auto S2, auto...>
[[nodiscard]] consteval auto have_different_flag_names() -> bool {
    if constexpr (!IsAFlag<S1> || !IsAFlag<S2>) {
        return true;
    } else {
        if (S1.long_form.as_string_view() == S2.long_form.as_string_view()) {
            return false;
        }
        return !S1.short_form.has_value || !S2.short_form.has_value
               || S1.short_form != S2.short_form;
    }
}

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto check_all_different_names() -> bool {
    if constexpr (sizeof...(Ss) == 0) {
        return true;
    } else {
        if (!have_different_flag_names<S1, Ss...>()) {
            return false;
        }
        return check_all_different_names<Ss...>();
    }
}

[[nodiscard]] constexpr auto is_valid_first_char(char c) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
};

[[nodiscard]] constexpr auto is_valid_non_first_char(char c) -> bool {
    return (is_valid_first_char(c) || (c >= '0' && c <= '9')) || c == '-' || c == '_';
};

[[nodiscard]] constexpr auto is_valid_name(std::string_view name) -> bool {
    if (name.empty()) {
        return false;
    }
    if (!is_valid_first_char(name[0])) {
        return false;
    }
    if (name.size() == 1) {
        return true;
    }
    for (auto i = 1uz; i < name.size(); ++i) {
        if (!is_valid_non_first_char(name[i])) {
            return false;
        }
    }
    return true;
}

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto check_valid_names() -> bool {
    if constexpr (is_subcommand_v<decltype(S1)>) {
        if (!is_valid_name(S1.name.as_string_view())) {
            return false;
        }
    }
    if constexpr (IsAFlag<S1>) {
        if (!is_valid_name(S1.long_form.as_string_view())) {
            return false;
        }
        if (!S1.short_form.has_value) {
            return true;
        }
        if (!is_valid_first_char(S1.short_form.value)) {
            return false;
        }
    }
    if constexpr (sizeof...(Ss) > 0) {
        return check_valid_names<Ss...>();
    } else {
        return true;
    }
}

template <typename T>
consteval auto result_type() -> std::remove_cvref_t<T>;

template <std::invocable T>
consteval auto result_type() -> std::remove_cvref_t<decltype(std::declval<T>()())>;

template <auto S>
using result_type_t = decltype(result_type<typename decltype(S)::value_t>());

template <auto S>
requires(!is_positional_v<decltype(S)> || (is_positional_v<decltype(S)> && !S.variadic))
consteval auto parse_type() -> result_type_t<S>;

template <auto S>
requires PositionalVariadic<S>
consteval auto parse_type() ->
    typename result_type_t<S>::value_type;  // this is a vector, so we can get its contained type

template <auto S>
using parse_type_t = decltype(parse_type<S>());

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto check_variadics() -> bool {
    if constexpr (is_positional_variadic_v<S1>) {
        return IsVector<result_type_t<S1>>::value;
    }
    if constexpr (sizeof...(Ss) > 0) {
        return check_variadics<Ss...>();
    } else {
        return true;
    }
}

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto count_variadics() -> std::size_t {
    if constexpr (is_positional_v<decltype(S1)>) {
        if constexpr (sizeof...(Ss) > 0) {
            return static_cast<std::size_t>(S1.variadic) + check_variadics<Ss...>();
        } else {
            return 0;
        }
    } else {
        if constexpr (sizeof...(Ss) > 0) {
            return check_variadics<Ss...>();
        } else {
            return 0;
        }
    }
}

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto check_variadic_is_last_positional_rec(bool found_variadic) -> bool {
    if constexpr (is_positional_variadic_v<S1>) {
        if constexpr (sizeof...(Ss) > 0) {
            return !found_variadic || check_variadic_is_last_positional_rec<Ss...>(true);
        } else {
            return !found_variadic;
        }
    }
    if constexpr (is_positional_v<decltype(S1)>) {
        if (found_variadic) {
            return false;
        }
    }
    if constexpr (sizeof...(Ss) > 0) {
        return check_variadic_is_last_positional_rec<Ss...>(found_variadic);
    } else {
        return true;
    }
}

template <auto... Ss>
[[nodiscard]] consteval auto check_variadic_is_last_positional() -> bool {
    return check_variadic_is_last_positional_rec<Ss...>(false);
}

template <auto S>
[[nodiscard]] auto default_arg_value() {
    if constexpr (std::is_invocable_v<typename decltype(S)::value_t>) {
        return result_type_t<S>{};
    } else if constexpr (requires { S.default_value; }) {
        return S.default_value;
    } else {
        // this is the case for positional arguments.
        return result_type_t<S>{};
    }
}

struct [[nodiscard]] PositionalHelp {
    std::string_view name{};
    std::string_view description{};
    bool is_required{};
};

struct [[nodiscard]] FlagHelp {
    std::optional<char> short_name{};
    std::string_view long_name{};
    std::string_view description{};
    bool is_required{};
};

template <auto S>
auto build_help_data(
    std::vector<PositionalHelp> &positional,
    std::vector<FlagHelp> &pure_flags,  // NOLINT(bugprone-easily-swappable-parameters)
    std::vector<FlagHelp> &flags_with_value) -> void {
    using s_t = decltype(S);
    if constexpr (is_positional_v<s_t>) {
        positional.push_back(
            PositionalHelp{
                .name = S.name.as_string_view(),
                .description = S.help.as_string_view(),
                .is_required = S.required});
    } else if constexpr (is_flag_v<s_t>) {
        auto const short_name =
            S.short_form.has_value ? std::optional{S.short_form.value} : std::optional<char>{};
        pure_flags.push_back(
            FlagHelp{
                .short_name = short_name,
                .long_name = S.long_form.as_string_view(),
                .description = S.help.as_string_view(),
                .is_required = S.required});
    } else if constexpr (is_flag_with_value_v<s_t>) {
        auto const short_name =
            S.short_form.has_value ? std::optional{S.short_form.value} : std::optional<char>{};
        flags_with_value.push_back(
            FlagHelp{
                .short_name = short_name,
                .long_name = S.long_form.as_string_view(),
                .description = S.help.as_string_view(),
                .is_required = S.required});
    } else if constexpr (is_subcommand_v<decltype(S)>) {
        positional.push_back(
            PositionalHelp{
                .name = S.name.as_string_view(), .description = S.help.as_string_view()});
    } else {
        static_assert(false, "Invalid spec");
    }
}

template <auto S1, auto S2, auto... Specs>
auto build_help_data(
    std::vector<PositionalHelp> &positional,
    std::vector<FlagHelp> &pure_flags,
    std::vector<FlagHelp> &flags_with_value) -> void {
    build_help_data<S1>(positional, pure_flags, flags_with_value);
    build_help_data<S2, Specs...>(positional, pure_flags, flags_with_value);
}

auto build_positionals_help(std::string &help, std::vector<PositionalHelp> &positional) -> void;
auto build_flags_help(std::string &help, std::vector<FlagHelp> &flags) -> void;

template <Str Usage, Str Description, auto... Specs>
[[nodiscard]] static auto make_help() -> std::string {
    std::vector<PositionalHelp> positional{};
    std::vector<FlagHelp> pure_flags{};
    std::vector<FlagHelp> flags_with_value{};

    build_help_data<Specs...>(positional, pure_flags, flags_with_value);

    auto help = std::string{};

    if constexpr (!Usage.is_empty()) {
        help += Usage.as_string_view();
    }
    if constexpr (!Description.is_empty()) {
        help += "\n\n";
        help += Description.as_string_view();
    }

    if (!positional.empty()) {
        help += "\n\n";
        help += "Arguments:\n\n";
        build_positionals_help(help, positional);
    }

    if (!pure_flags.empty()) {
        help += "\n\n";
        help += "Flags:\n\n";
        build_flags_help(help, pure_flags);
    }

    if (!flags_with_value.empty()) {
        help += "\n\n";
        build_flags_help(help, flags_with_value);
    }

    help.push_back('\n');
    return help;
}

}  // namespace detail

template <Str Usage, Str Description, auto... Specs>
requires(sizeof...(Specs) > 0)
struct [[nodiscard]] Rules {
    static_assert(
        detail::check_all_different_names<Specs...>(), "All flags must have unique identifiers");
    static_assert(
        detail::check_valid_names<Specs...>(),
        "All flags must begin with a letter, both long and short forms");
    static_assert(detail::check_variadics<Specs...>(), "Variadics positionals must be vector<T>");
    static_assert(
        detail::count_variadics<Specs...>() <= 1,
        "You can set at most 1 variadic positional argument");
    static_assert(
        detail::check_variadic_is_last_positional<Specs...>(),
        "Positional variadic argument must be the last positional argument because it consumes all "
        "positionals");

    [[nodiscard]] static auto help() -> std::string_view {
        static auto help_msg = detail::make_help<Usage, Description, Specs...>();
        return std::string_view{help_msg};
    }
};

template <auto S>
struct [[nodiscard]] CommandArgValue {
    detail::result_type_t<S> value{detail::default_arg_value<S>()};
    bool is_used{false};
    static constexpr auto spec = S;
};

template <auto... Specs>
class [[nodiscard]] Args;

template <typename>
struct ArgsFromSubCommand {
    static_assert(false, "Not a subcommand");
};

template <std::size_t N, std::size_t M, Str Usage, Str Description, auto... Specs>
struct ArgsFromSubCommand<Subcommand<N, M, Usage, Description, Specs...>> {
    using args_t = Args<Specs...>;
};

template <auto S>
struct [[nodiscard]] SubcommandArgValue {
    detail::result_type_t<S> value{detail::default_arg_value<S>()};
    bool is_used{false};
    ArgsFromSubCommand<decltype(S)>::args_t subcommands{};
    static constexpr auto spec = S;
};

template <auto S>
struct ArgValue
    : std::conditional_t<is_subcommand_v<decltype(S)>, SubcommandArgValue<S>, CommandArgValue<S>> {
};

namespace detail {
template <auto S, auto... Ss>
struct GetRet {
    using type = std::conditional_t<
        is_subcommand_v<decltype(S)>,
        typename GetRet<Ss...>::type,
        result_type_t<S>>;
};

template <auto S>
struct GetRet<S> {
    using type = result_type_t<S>;
};

template <auto S, auto... Ss>
struct GetWithInfoRet {
    using type = std::conditional_t<
        is_subcommand_v<decltype(S)>,
        typename GetWithInfoRet<Ss...>::type,
        ArgValue<S>>;
};

template <auto S>
struct GetWithInfoRet<S> {
    using type = ArgValue<S>;
};
}  // namespace detail

template <auto... Specs>
class [[nodiscard]] Args {
public:
    explicit Args(std::tuple<ArgValue<Specs>...> results)
        : m_results{std::move(results)} {}

    explicit Args() = default;

    template <auto S>
    [[nodiscard]] constexpr auto get_with_info() const noexcept -> ArgValue<S> const & {
        return std::get<ArgValue<S>>(m_results);
    }

    template <auto S>
    [[nodiscard]] constexpr auto get() const noexcept -> detail::result_type_t<S> const & {
        return get_with_info<S>().value;
    }

    template <auto Sb, auto S>
    requires(is_subcommand_v<decltype(Sb)> && !is_subcommand_v<decltype(S)>)
    [[nodiscard]] constexpr auto get_with_info() const noexcept -> ArgValue<S> const & {
        return get_with_info<Sb>().subcommands.template get_with_info<S>();
    }

    template <auto Sb, auto S>
    requires(is_subcommand_v<decltype(Sb)> && !is_subcommand_v<decltype(S)>)
    [[nodiscard]] constexpr auto get() const noexcept -> detail::result_type_t<S> const & {
        return get_with_info<Sb>().subcommands.template get<S>();
    }

    template <auto Sb, auto S, auto... Ss>
    requires(is_subcommand_v<decltype(Sb)> && is_subcommand_v<decltype(S)> && sizeof...(Ss) > 0)
    [[nodiscard]] constexpr auto get() const noexcept -> detail::GetRet<Ss...>::type const & {
        return get_with_info<Sb>().subcommands.template get<S, Ss...>();
    }

    template <auto Sb, auto S, auto... Ss>
    requires(is_subcommand_v<decltype(Sb)> && is_subcommand_v<decltype(S)> && sizeof...(Ss) > 0)
    [[nodiscard]] constexpr auto get_with_info() const noexcept
        -> detail::GetWithInfoRet<Ss...>::type const & {
        return get_with_info<Sb>().subcommands.template get_with_info<S, Ss...>();
    }

private:
    std::tuple<ArgValue<Specs>...> m_results{};
};

struct [[nodiscard]] Help {
    std::string_view message;
};

struct [[nodiscard]] Error {
    std::string message;
};

template <auto... Ss>
using compile_result_t = std::variant<Args<Ss...>, Help, Error>;

template <auto... Ss>
[[nodiscard]] constexpr auto has_args(compile_result_t<Ss...> const &res) -> bool {
    return std::holds_alternative<Args<Ss...>>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto has_help(compile_result_t<Ss...> const &res) -> bool {
    return std::holds_alternative<Help>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto has_error(compile_result_t<Ss...> const &res) -> bool {
    return std::holds_alternative<Error>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto get_args(compile_result_t<Ss...> const &res) -> Args<Ss...> const & {
    return std::get<Args<Ss...>>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto get_args(compile_result_t<Ss...> &res) -> Args<Ss...> & {
    return std::get<Args<Ss...>>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto get_args(compile_result_t<Ss...> &&res) -> Args<Ss...> {
    return std::get<Args<Ss...>>(std::move(res));
}

template <auto... Ss>
[[nodiscard]] constexpr auto get_help(compile_result_t<Ss...> const &res) -> Help {
    return std::get<Help>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto get_error(compile_result_t<Ss...> const &res) -> Error const & {
    return std::get<Error>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto get_error(compile_result_t<Ss...> &res) -> Error & {
    return std::get<Error>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto get_error(compile_result_t<Ss...> &&res) -> Error {
    return std::get<Error>(std::move(res));
}
}  // namespace args

#endif
