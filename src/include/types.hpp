#ifndef CPP_ARGS_TYPES
#define CPP_ARGS_TYPES

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace args {
template <typename T>
struct [[nodiscard]] PlainOptional {
    bool has_value;
    T value;

    [[nodiscard]] constexpr auto operator==(PlainOptional const &) const -> bool = default;

    static consteval auto empty() -> PlainOptional {
        return PlainOptional{.has_value = false, .value = T{}};
    }

    static consteval auto with(T value) -> PlainOptional {
        return PlainOptional{.has_value = true, .value = value};
    }
};

template <std::size_t N>
struct [[nodiscard]] Str {
    std::array<char, N + 1> chars{};

    consteval Str(char const (&s)[N + 1]) {  // NOLINT
        std::ranges::copy(s, chars.begin());
    }

    [[nodiscard]] constexpr auto as_string_view() const -> std::string_view {
        return std::string_view{chars.data()};
    }

    [[nodiscard]] constexpr auto operator==(Str const &) const -> bool = default;
};

template <std::size_t N>
Str(char const (&s)[N]) -> Str<N - 1>;  // NOLINT

template <std::size_t N>
struct [[nodiscard]] Flag {
    Str<N> long_form;
    PlainOptional<char> short_form{.has_value = false, .value = {}};
    bool default_value{};
    static constexpr bool is_spec = true;
    using value_t = bool;

    [[nodiscard]] constexpr auto operator==(Flag const &) const -> bool = default;
};

template <typename V>
struct [[nodiscard]] FlagWithValue {
    std::string_view long_form;
    std::optional<char> short_form{.has_value = false, .value = V{}};
    V default_value{};
    static constexpr bool is_spec = true;
    using value_t = V;

    [[nodiscard]] constexpr auto operator==(FlagWithValue const &) const -> bool = default;
};

template <typename P>
struct [[nodiscard]] Positional {
    std::size_t index{};
    static constexpr bool is_spec = true;
    using value_t = P;

    [[nodiscard]] constexpr auto operator==(Positional const &) const -> bool = default;
};

// Cheap way to define a Spec. Not very sound
template <typename S>
concept Spec = S::is_spec;

template <Spec auto S>
struct Required {
    static constexpr bool is_spec = true;
    using value_t = decltype(S)::value_t;

    [[nodiscard]] constexpr auto operator==(Required const &) const -> bool = default;
};

template <typename>
struct IsRequired: std::false_type {};

template <Spec auto S>
struct IsRequired<Required<S>>: std::true_type {};

template <typename T>
inline constexpr bool IsRequired_v = IsRequired<T>::value;

template <typename>
struct IsFlag: std::false_type {};

template <std::size_t N>
struct IsFlag<Flag<N>>: std::true_type {};

template <Spec auto S>
struct IsFlag<Required<S>>
    : std::conditional_t<IsFlag<decltype(S)>::value, std::true_type, std::false_type> {};

template <typename T>
inline constexpr bool IsFlag_v = IsFlag<T>::value;

template <Spec auto... Specs>
struct Rules {};

template <Spec auto S>
struct [[nodiscard]] ResultValue {
    decltype(S)::value_t value;
    bool m_is_used{false};
    static constexpr auto spec = S;  // maybe not needed
};

template <Spec auto... Specs>
struct [[nodiscard]] Result {
    std::tuple<ResultValue<Specs>...> results;

    template <Spec auto S>
    [[nodiscard]] constexpr auto get_with_info() -> ResultValue<S> const & {
        return std::get<ResultValue<S>>(results);
    }

    template <Spec auto S>
    [[nodiscard]] constexpr auto get() -> decltype(S)::value_t const & {
        return get_with_info<S>().value;
    }
};

}  // namespace args

#endif
