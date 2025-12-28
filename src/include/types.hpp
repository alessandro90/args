#ifndef CPP_ARGS_TYPES
#define CPP_ARGS_TYPES

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

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
};

consteval auto short_form(char c) -> Opt<char> {
    return Opt<char>::with(c);
}

template <typename T>
concept Trivial = std::is_trivial_v<T>;

template <std::size_t N>
struct [[nodiscard]] Str {
    std::array<char, N + 1> chars{};

    consteval Str(char const (&s)[N + 1]) {  // NOLINT
        std::ranges::copy(s, chars.begin());
    }

    [[nodiscard]] constexpr auto as_string_view() const -> std::string_view {
        return std::string_view{chars.data()};
    }
};

template <std::size_t N>
Str(char const (&s)[N]) -> Str<N - 1>;  // NOLINT

template <std::size_t N>
struct [[nodiscard]] Flag {
    Str<N> long_form;
    Opt<char> short_form{Opt<char>::empty()};
    bool default_value{};
    bool required{};
    static constexpr bool is_spec = true;
    using value_t = bool;
};

template <Trivial V, std::size_t N>
struct [[nodiscard]] FlagWithValue {
    Str<N> long_form;
    Opt<char> short_form{Opt<char>::empty()};
    /// Used if the flag is missing
    V default_value{};
    bool required{};
    static constexpr bool is_spec = true;
    using value_t = V;
};

template <std::size_t N>
struct [[nodiscard]] FlagWithValueArgs {
    Str<N> long_form;
    Opt<char> short_form{Opt<char>::empty()};
    bool required{};
};

/// If you want a specific default value, use FlagWithValue directly. This function is meant
/// to be used to specify the type of the attached value in case you do not want to manually set
/// a default. For example if the flag is requried
///
/// This is a workaround because all the specs must be literal types
///
/// FIXME: this function is not ideal after the support of complex types such as vector and string
template <Trivial V1, std::size_t N>
constexpr auto default_flag_with_value(FlagWithValueArgs<N> flag_args) -> FlagWithValue<V1, N> {
    return FlagWithValue{
        .long_form = flag_args.long_form,
        .short_form = flag_args.short_form,
        .default_value = V1{},
        .required = flag_args.required};
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

template <typename P>
struct [[nodiscard]] Positional {
    static constexpr bool is_spec = true;
    using value_t = P;
};

// Cheap way to define a Spec. Not very sound
template <typename S>
concept Spec = S::is_spec;

template <Spec>
struct IsFlag: std::false_type {};

template <std::size_t N>
struct IsFlag<Flag<N>>: std::true_type {};

template <Spec T>
inline constexpr bool is_flag_v = IsFlag<std::remove_cvref_t<T>>::value;

template <Spec>
struct IsFlagWithValue: std::false_type {};

template <Trivial V, std::size_t N>
struct IsFlagWithValue<FlagWithValue<V, N>>: std::true_type {};

template <typename P>
struct IsPositional: std::false_type {};

template <typename P>
struct IsPositional<Positional<P>>: std::true_type {};

template <typename P>
inline constexpr bool is_positional_v = IsPositional<std::remove_cvref_t<P>>::value;

template <Spec T>
inline constexpr bool is_flag_with_value_v = IsFlagWithValue<std::remove_cvref_t<T>>::value;

namespace detail {

template <typename T>
struct Typetag {
    using type = T;
};

template <auto S>
concept IsAnyFlag = is_flag_v<decltype(S)> || is_flag_with_value_v<decltype(S)>;

template <Spec auto S1, Spec auto S2, Spec auto...>
[[nodiscard]] consteval auto have_different_flag_names() -> bool {
    if constexpr (!IsAnyFlag<S1> || !IsAnyFlag<S2>) {
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

template <Spec auto S1, Spec auto... Ss>
[[nodiscard]] consteval auto check_valid_names() -> bool {
    auto const is_valid_char = [](char c) -> bool {
        return c >= 'a' && c <= 'z';
    };
    if constexpr (!IsAnyFlag<S1>) {
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
}  // namespace detail

template <Spec auto... Specs>
requires(sizeof...(Specs) > 0)
struct Rules {
    static_assert(
        detail::check_all_different_names<Specs...>(), "All flags must have unique identifiers");
    static_assert(
        detail::check_valid_names<Specs...>(),
        "All flags must begin with a letter, both long and short forms");
};

template <Spec auto S>
using GetterReturnValue = decltype(S.default_value()) const &;

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
inline constexpr auto Lazy = []() {
    return T{std::move(Args)...};
};

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
