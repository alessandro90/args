#ifndef CPP_ARGS_TYPES
#define CPP_ARGS_TYPES

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace args {

template <typename T>
struct Typetag {
    using type_t = T;
};

template <typename T>
inline constexpr auto tag = Typetag<T>{};

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
};

template <std::size_t N>
Str(char const (&s)[N]) -> Str<N - 1>;  // NOLINT

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
consteval auto operator""_str() -> decltype(X) {
    return X;
}

template <Str X>
consteval auto operator""_flag() -> decltype(X) {
    return X;
}

template <Str X>
requires(X.chars.size() == 2 && X.chars[1] == '\0')
consteval auto operator""_short_flag() -> Opt<char> {
    return Opt<char>::with(X.chars[0]);
}

template <typename P, std::size_t N = 0, std::size_t M = 0>
struct [[nodiscard]] Positional {
    Typetag<P> type;
    Str<N> name{};
    Str<M> help{};

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

[[nodiscard]] constexpr auto is_valid_char(char c) -> bool {
    return c >= 'a' && c <= 'z';
};

template <Spec auto S1, Spec auto... Ss>
[[nodiscard]] consteval auto check_valid_names() -> bool {
    if constexpr (!IsAFlag<S1>) {
        return true;
    } else {
        if (!is_valid_char(S1.long_form.chars[0])) {
            return false;
        }
        if (!S1.short_form.has_value) {
            return true;
        }
        if (!is_valid_char(S1.short_form.value)) {
            return false;
        }
        if constexpr (sizeof...(Ss) > 0) {
            return check_valid_names<Ss...>();
        } else {
            return true;
        }
    }
}

struct [[nodiscard]] PositionalHelp {
    std::string name{};
    std::string description{};
};

struct [[nodiscard]] PureFlagHelp {
    std::string short_name{};
    std::string long_name{};
    std::string description{};
};

struct [[nodiscard]] FlagWithValueHelp {
    std::string short_name{};
    std::string long_name{};
    std::string type{};
    std::string description{};
};

struct HelpMaker {
    // wrap into struct so we do not need 'inline' or a .cpp file
    // just for this single function
    [[nodiscard]] static auto make_help() -> std::string {
        // NOTE: will need padding to align all descriptions. Usage and description string
        // must be provided to rules upon construction
        //
        // Something like this
        //
        // Usage: usage string (provided as extra arg to rules?)
        //
        // Description string (optional, may not be provided)
        //
        // Arguments:
        //
        // arg_name (or index like #1 if not provided)  description (if provided)
        //
        // Flags:
        //
        // short_name (if provided), long_name description
        //
        // Flags with values:
        //
        // short_name (if provided), long_name <type> description (may include possible values
        // allowed?)
        // TODO:
        return "";
    }
};

}  // namespace detail

template <Spec auto... Specs>
requires(sizeof...(Specs) > 0)
struct Rules {
    static_assert(
        detail::check_all_different_names<Specs...>(), "All flags must have unique identifiers");
    static_assert(
        detail::check_valid_names<Specs...>(),
        "All flags must begin with a letter, both long and short forms");
    // TODO:
    // [[nodiscard]] static auto help() -> std::string_view;
};

namespace detail {

template <typename T>
consteval auto result_type() -> std::remove_cvref_t<T>;

template <std::invocable T>
consteval auto result_type() -> std::remove_cvref_t<decltype(std::declval<T>()())>;

template <Spec auto S>
using result_type_t = decltype(result_type<typename decltype(S)::value_t>());

template <Spec auto S>
[[nodiscard]] auto default_arg_value() {
    if constexpr (std::is_invocable_v<typename decltype(S)::value_t>) {
        return result_type_t<S>{};
    } else if constexpr (requires { S.default_value; }) {
        return S.default_value;
    } else {
        // this is the case for positional arguments. They have no default,
        // but we need a default anyway, so if the builder is a function returning
        // a vector, we just create an empty vector
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
struct [[nodiscard]] ArgValue {
    detail::result_type_t<S> value{detail::default_arg_value<S>()};
    bool is_used{false};
    static constexpr auto spec = S;
};

template <Spec auto... Specs>
class [[nodiscard]] Args {
public:
    explicit Args(std::tuple<ArgValue<Specs>...> results)
        : m_results{std::move(results)} {}

    template <Spec auto S>
    [[nodiscard]] constexpr auto get_with_info() const -> ArgValue<S> const & {
        return std::get<ArgValue<S>>(m_results);
    }

    template <Spec auto S>
    [[nodiscard]] constexpr auto get() const -> detail::result_type_t<S> const & {
        return get_with_info<S>().value;
    }

private:
    std::tuple<ArgValue<Specs>...> m_results;
};

}  // namespace args

#endif
