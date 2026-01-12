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
#include <vector>
#include "typetag.hpp"

namespace args {


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

consteval auto short_form(char c) -> Opt<char> {
    return Opt<char>::with(c);
}

template <typename T>
concept Trivial = std::is_trivial_v<T>;

template <std::size_t N>
struct [[nodiscard]] Str {
    std::array<char, N + 1> chars{};

    consteval Str() = default;

    consteval Str(char const (&s)[N + 1]) {  // NOLINT
        std::ranges::copy(s, chars.begin());
    }

    [[nodiscard]] constexpr auto as_string_view() const -> std::string_view {
        return std::string_view{chars.data()};
    }

    [[nodiscard]] static constexpr auto is_empty() -> bool {
        return N == 0;
    }
};

template <std::size_t N>
Str(char const (&s)[N]) -> Str<N - 1>;  // NOLINT

template <Str X>
consteval auto operator""_str() -> decltype(X) {
    return X;
}

inline constexpr auto empty = ""_str;

template <std::size_t N, std::size_t M = 0>
struct [[nodiscard]] Flag {
    Str<N> long_form;
    Opt<char> short_form{Opt<char>::empty()};
    bool default_value{};
    bool required{};
    Str<M> help{};
    static constexpr bool is_spec = true;
    using value_t = bool;
};

template <Trivial V, std::size_t N, std::size_t M = 0>
struct [[nodiscard]] FlagWithValue {
    Str<N> long_form;
    Opt<char> short_form{Opt<char>::empty()};
    /// Used if the flag is missing
    V default_value{};
    bool required{};
    Str<M> help{};
    static constexpr bool is_spec = true;
    using value_t = V;
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

template <std::default_initializable P, std::size_t N = 0, std::size_t M = 0>
struct [[nodiscard]] Positional {
    Typetag<P> type;
    Str<N> name{};
    Str<M> help{};
    bool required{};

    static constexpr bool is_spec = true;
    using value_t = P;
};

// Cheap way to define a Spec. Not very sound
template <typename S>
concept Spec = S::is_spec;

template <Spec>
struct IsFlag: std::false_type {};

template <std::size_t N, std::size_t M>
struct IsFlag<Flag<N, M>>: std::true_type {};

template <Spec T>
inline constexpr bool is_flag_v = IsFlag<std::remove_cvref_t<T>>::value;

template <Spec>
struct IsFlagWithValue: std::false_type {};

template <Trivial V, std::size_t N, std::size_t M>
struct IsFlagWithValue<FlagWithValue<V, N, M>>: std::true_type {};

template <typename P>
struct IsPositional: std::false_type {};

template <typename P, std::size_t N, std::size_t M>
struct IsPositional<Positional<P, N, M>>: std::true_type {};

template <typename P>
inline constexpr bool is_positional_v = IsPositional<std::remove_cvref_t<P>>::value;

template <Spec T>
inline constexpr bool is_flag_with_value_v = IsFlagWithValue<std::remove_cvref_t<T>>::value;

namespace detail {

template <auto S>
concept IsAFlag = is_flag_v<decltype(S)> || is_flag_with_value_v<decltype(S)>;

template <Spec auto S1, Spec auto S2, Spec auto...>
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

template <Spec auto S1, Spec auto... Ss>
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

template <Spec auto S1, Spec auto... Ss>
[[nodiscard]] consteval auto check_valid_names() -> bool;

template <typename T>
consteval auto result_type() -> std::remove_cvref_t<T>;

template <std::invocable T>
consteval auto result_type() -> std::remove_cvref_t<decltype(std::declval<T>()())>;

template <Spec auto S>
using result_type_t = decltype(result_type<typename decltype(S)::value_t>());

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

template <Spec auto S>
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
    } else {
        static_assert(false, "Invalid spec");
    }
}

template <Spec auto S1, Spec auto S2, Spec auto... Specs>
auto build_help_data(
    std::vector<PositionalHelp> &positional,
    std::vector<FlagHelp> &pure_flags,
    std::vector<FlagHelp> &flags_with_value) -> void {
    build_help_data<S1>(positional, pure_flags, flags_with_value);
    build_help_data<S2, Specs...>(positional, pure_flags, flags_with_value);
}

auto build_positionals_help(std::string &help, std::vector<PositionalHelp> &positional) -> void;
auto build_flags_help(std::string &help, std::vector<FlagHelp> &flags) -> void;

template <Str Usage, Str Description, Spec auto... Specs>
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

template <Str Usage, Str Description, Spec auto... Specs>
requires(sizeof...(Specs) > 0)
struct [[nodiscard]] Rules {
    static_assert(
        detail::check_all_different_names<Specs...>(), "All flags must have unique identifiers");
    static_assert(
        detail::check_valid_names<Specs...>(),
        "All flags must begin with a letter, both long and short forms");

    [[nodiscard]] static auto help() -> std::string_view {
        static auto help_msg = detail::make_help<Usage, Description, Specs...>();
        return std::string_view{help_msg};
    }
};

template <std::size_t N, std::size_t M, Str Usage, Str Description, Spec auto... Specs>
struct [[nodiscard]] Subcommand {
    Str<N> name{};
    Str<M> help{};
    Rules<Usage, Description, Specs...> rules{};

    static constexpr bool is_spec = true;
    using value_t = decltype([]() {
        return std::string_view{};
    });
};

template <typename>
struct IsSubcommand: std::false_type {};

template <std::size_t N, std::size_t M, Str Usage, Str Description, Spec auto... Specs>
struct IsSubcommand<Subcommand<N, M, Usage, Description, Specs...>>: std::true_type {};

template <Spec S>
inline constexpr bool is_subcommand_v = IsSubcommand<std::remove_cvref_t<S>>::value;

namespace detail {
template <Spec auto S1, Spec auto... Ss>
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

template <Spec auto S>
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
}  // namespace detail

template <typename T, auto... Args>
inline constexpr auto Lazy = [] {
    return T{std::move(Args)...};
};

template <typename T, auto... Args>
using lazy_t = decltype(Lazy<T, Args...>);

template <Spec auto S>
struct [[nodiscard]] CommandArgValue {
    detail::result_type_t<S> value{detail::default_arg_value<S>()};
    bool is_used{false};
    static constexpr auto spec = S;
};

template <Spec auto... Specs>
class [[nodiscard]] Args;

template <typename>
struct ArgsFromSubCommand {};

template <std::size_t N, std::size_t M, Str Usage, Str Description, Spec auto... Specs>
struct ArgsFromSubCommand<Subcommand<N, M, Usage, Description, Specs...>> {
    using args_t = Args<Specs...>;
};

template <Spec auto S>
struct [[nodiscard]] SubcommandArgValue {
    detail::result_type_t<S> value{detail::default_arg_value<S>()};
    bool is_used{false};
    ArgsFromSubCommand<decltype(S)>::args_t subcommands{};
    static constexpr auto spec = S;
};

template <Spec auto S>
struct [[nodiscard]] ArgValue
    : std::conditional_t<is_subcommand_v<decltype(S)>, SubcommandArgValue<S>, CommandArgValue<S>> {
};

template <Spec auto... Specs>
class [[nodiscard]] Args {
public:
    explicit Args(std::tuple<ArgValue<Specs>...> results)
        : m_results{std::move(results)} {}

    explicit Args() = default;

    template <Spec auto S>
    [[nodiscard]] constexpr auto get_with_info() const -> ArgValue<S> const & {
        return std::get<ArgValue<S>>(m_results);
    }

    template <Spec auto S>
    [[nodiscard]] constexpr auto get() const -> detail::result_type_t<S> const & {
        return get_with_info<S>().value;
    }

    template <Spec auto Sb, Spec auto S>
    requires is_subcommand_v<decltype(Sb)>
    [[nodiscard]] constexpr auto get() const -> detail::result_type_t<S> const & {
        return get_with_info<Sb>().subcommands.template get<S>();
    }

    template <Spec auto Sb, Spec auto S>
    requires is_subcommand_v<decltype(Sb)>
    [[nodiscard]] constexpr auto get_with_info() const -> ArgValue<S> const & {
        return get_with_info<Sb>().subcommands.template get_with_info<S>();
    }

private:
    std::tuple<ArgValue<Specs>...> m_results{};
};

}  // namespace args

#endif
