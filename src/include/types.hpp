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

    [[nodiscard]] constexpr auto operator==(Opt const &) const -> bool = default;

    static consteval auto empty() -> Opt {
        return Opt{.has_value = false, .value = T{}};
    }

    static consteval auto with(T value) -> Opt {
        return Opt{.has_value = true, .value = value};
    }
};

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

    [[nodiscard]] constexpr auto operator==(Str const &) const -> bool = default;
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

    [[nodiscard]] constexpr auto operator==(Flag const &) const -> bool = default;
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

    [[nodiscard]] constexpr auto operator==(FlagWithValue const &) const -> bool = default;
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
template <Trivial V1, std::size_t N>
constexpr auto default_flag_with_value(FlagWithValueArgs<N> flag_args) -> FlagWithValue<V1, N> {
    return FlagWithValue{
        .long_form = flag_args.long_form,
        .short_form = flag_args.short_form,
        .default_value = V1{},
        .required = flag_args.required};
}

// TODO: add a size_t index to any positional in order to track the order?
template <std::default_initializable P>
struct [[nodiscard]] Positional {
    static constexpr bool is_spec = true;
    using value_t = P;

    [[nodiscard]] constexpr auto operator==(Positional const &) const -> bool = default;
};

// Cheap way to define a Spec. Not very sound
template <typename S>
concept Spec = S::is_spec;

template <Spec>
struct IsFlag: std::false_type {};

template <std::size_t N>
struct IsFlag<Flag<N>>: std::true_type {};

template <Spec T>
inline constexpr bool is_flag_v = IsFlag<T>::value;

template <Spec>
struct IsFlagWithValue: std::false_type {};

template <Trivial V, std::size_t N>
struct IsFlagWithValue<FlagWithValue<V, N>>: std::true_type {};

template <Spec T>
inline constexpr bool is_flag_with_value_v = IsFlagWithValue<T>::value;

template <Spec auto... Specs>
requires(sizeof...(Specs) > 0)
struct Rules {
};  // TODO: add static validation (e.g. check duplicate flags, check duplicate positional indexes)

template <Spec auto S>
struct [[nodiscard]] ArgValue {
    decltype(S)::value_t value;
    bool is_used{false};
    static constexpr auto spec = S;
};

template <Spec auto... Specs>
class [[nodiscard]] Args {
public:
    explicit Args(std::tuple<ArgValue<Specs>...> results)
        : m_results{std::move(results)} {}

    template <Spec auto S>
    [[nodiscard]] constexpr auto get_with_info() -> ArgValue<S> const & {
        return std::get<ArgValue<S>>(m_results);
    }

    template <Spec auto S>
    [[nodiscard]] constexpr auto get() -> decltype(S)::value_t const & {
        return get_with_info<S>().value;
    }

private:
    std::tuple<ArgValue<Specs>...> m_results;
};

}  // namespace args

#endif
