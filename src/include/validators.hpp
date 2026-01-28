#ifndef CPP_ARGS_VALIDATORS
#define CPP_ARGS_VALIDATORS

#include <algorithm>
#include <array>
#include <concepts>
#include <expected>
#include <format>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

namespace args {

using validator_result_t = std::expected<void, std::string>;

template <typename V, typename ErrFn>
struct [[nodiscard]] Validator {
    V fn;
    ErrFn err_fn;

    template <typename T>
    [[nodiscard]] constexpr auto operator()(T const &value) const -> validator_result_t {
        if (fn(value)) {
            return std::unexpected(err_fn(value));
        }
        return {};
    }
};

template <typename V, typename ErrFn>
constexpr auto make_validator(V v, ErrFn err_fn) -> Validator<V, ErrFn> {
    return Validator{.fn = v, .err_fn = err_fn};
}

template <typename>
struct IsValidator: std::false_type {};

template <typename V, typename ErrFn>
struct IsValidator<Validator<V, ErrFn>>: std::true_type {};

template <typename T>
inline constexpr auto is_validator_v = IsValidator<std::remove_cvref_t<T>>::value;

template <typename T>
concept AValidator = is_validator_v<T>;

template <AValidator auto... Vs>
inline constexpr AValidator auto Or = make_validator(
    [](auto const &value) -> bool {
        return (... || Vs.fn(value));
    },
    [](auto const &value) -> std::string {
        return std::array{std::format("'{}'", Vs.err_fn(value))...} | std::views::join_with(" or ")
               | std::ranges::to<std::string>();
    });

template <AValidator auto... Vs>
inline constexpr AValidator auto And = make_validator(
    [](auto const &value) -> bool {
        return (... && Vs.fn(value));
    },
    [](auto const &value) -> std::string {
        return std::array{std::format("'{}'", Vs.err_fn(value))...} | std::views::join_with(" and ")
               | std::ranges::to<std::string>();
    });

template <AValidator auto... Vs>
inline constexpr AValidator auto Xor = make_validator(
    [](auto const &value) -> bool {
        return (... ^ Vs.fn(value));
    },
    [](auto const &value) -> std::string {
        return std::array{std::format("'{}'", Vs.err_fn(value))...} | std::views::join_with(" xor ")
               | std::ranges::to<std::string>();
    });

template <AValidator auto V>
inline constexpr AValidator auto Not = make_validator(
    [](auto const &value) -> bool {
        return !V.fn(value);
    },
    [](auto const &value) -> std::string {
        return std::format("(not '{}')", V.err_fn(value));
    });

inline constexpr auto always = Validator{
    .fn = [](auto const &) -> bool {
        return {};
    },
    .err_fn = [](auto const &) -> std::string {
        return "";
    }};

using always_t = std::remove_cvref_t<decltype(always)>;

template <std::totally_ordered auto Limit>
inline constexpr AValidator auto less_than = Validator{
    .fn = [](auto const &value) -> bool {
        return value < Limit;
    },
    .err_fn = [](auto const &value) -> std::string {
        return std::format("Value '{}' must be less than '{}'", value, Limit);
    }};

template <std::equality_comparable auto Target>
inline constexpr AValidator auto equal = Validator{
    .fn = [](auto const &value) -> bool {
        return value == Target;
    },
    .err_fn = [](auto const &value) -> std::string {
        return std::unexpected(std::format("Value '{}' must be equal to '{}'", value, Target));
    }};

template <std::totally_ordered auto Limit>
inline constexpr AValidator auto greater_than = Validator{
    .fn = [](auto const &value) -> bool {
        return value > Limit;
    },
    .err_fn = [](auto const &value) -> std::string {
        return std::format("Value '{}' must be greater than '{}'", value, Limit);
    }};

template <AValidator auto V>
inline constexpr AValidator auto for_each = Validator{
    .fn = []<typename T>(std::vector<T> const &value) -> bool {
        return std::ranges::all_of(value, [&](auto const &item) {
            return V.fn(item);
        });
    },
    .err_fn = [](auto const &value) -> std::string {
        return std::format("Vector item: {}", V.err_fn(value));
    }};

template <auto X>
inline constexpr AValidator auto less_or_equal = Or<equal<X>, less_than<X>>;

template <auto Value>
inline constexpr AValidator auto greater_or_equal = Or<equal<Value>, greater_than<Value>>;

template <auto Min, auto Max>
inline constexpr AValidator auto exclusive_range = And<greater_than<Min>, less_than<Max>>;

template <auto Min, auto Max>
inline constexpr AValidator auto inclusive_range = And<greater_or_equal<Min>, less_or_equal<Max>>;

template <auto Min, auto Max>
inline constexpr AValidator auto half_open_range = And<greater_or_equal<Min>, less_than<Max>>;
}  // namespace args

#endif
