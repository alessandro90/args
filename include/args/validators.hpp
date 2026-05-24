#ifndef ARGS_VALIDATORS
#define ARGS_VALIDATORS

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

using validator_error_t = std::string;
using validator_result_t = std::expected<void, validator_error_t>;

/// Used to apply validation logic to parsed arguments
///
/// `fn` is a validator function that takes a reference to a constant parsed value
/// and returns `true` if the checks passes
///
/// `err_fn` is a function used to display an error (if `fn` returns `false`). This function also
/// takes a reference to a constant parsed value
template <typename V, typename ErrFn>
struct [[nodiscard]] Validator {
    V fn;
    ErrFn err_fn;

    template <typename T>
    [[nodiscard]] constexpr auto operator()(T const &value) const -> validator_result_t {
        if (!fn(value)) {
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
concept ValidatorObject = is_validator_v<T>;

namespace detail {
[[nodiscard]] auto make_error_char_range(
    auto const &value, std::string_view joiner, ValidatorObject auto const &...validators)
    -> std::ranges::range auto {
    return std::array{std::format("'{}'", validators.err_fn(value))...}
           | std::views::join_with(joiner);
}
}  // namespace detail

/// Creates a new validator that is a logical 'or' between all the provided validators
///
/// Usage:
///
/// `Or<v0, v1, v2, ...>`
template <Validator... Vs>
inline constexpr auto Or = make_validator(
    [](auto const &value) -> bool {
        return (... || Vs.fn(value));
    },
    [](auto const &value) -> std::string {
        auto s = std::string(1, '(');
        s.append_range(detail::make_error_char_range(value, " or ", Vs...));
        s.push_back(')');
        return s;
    });


/// Creates a new validator that is a logical 'and' between all the provided validators
///
/// Usage:
///
/// `And<v0, v1, v2, ...>`
template <Validator... Vs>
inline constexpr auto And = make_validator(
    [](auto const &value) -> bool {
        return (... && Vs.fn(value));
    },
    [](auto const &value) -> std::string {
        auto s = std::string(1, '(');
        s.append_range(detail::make_error_char_range(value, " and ", Vs...));
        s.push_back(')');
        return s;
    });

/// Creates a new validator that is a logical 'xor' between all the provided validators
///
/// Usage:
///
/// `Xor<v0, v1, v2, ...>`
template <Validator... Vs>
inline constexpr auto Xor = make_validator(
    [](auto const &value) -> bool {
        return (... ^ Vs.fn(value));
    },
    [](auto const &value) -> std::string {
        auto s = std::string(1, '(');
        s.append_range(detail::make_error_char_range(value, " xor ", Vs...));
        s.push_back(')');
        return s;
    });

/// Creates a new validator that is a logical 'not' of the provided validator
///
/// Usage:
///
/// `Not<v>`
template <Validator V>
inline constexpr auto Not = make_validator(
    [](auto const &value) -> bool {
        return !V.fn(value);
    },
    [](auto const &value) -> std::string {
        return std::format("(not '{}')", V.err_fn(value));
    });

/// The identity validator, never fails
inline constexpr auto always = Validator{
    .fn = [](auto const &) -> bool {
        return true;
    },
    .err_fn = [](auto const &) -> std::string {
        return "";
    }};

using always_t = std::remove_cvref_t<decltype(always)>;

namespace detail {
// template <typename T, typename E>
// concept both_formattable = std::formattable<T, char> && std::formattable<E, char>;

template <std::formattable<char> Limit, std::formattable<char> Value>
[[nodiscard]] auto less_than_error_message(Limit const &limit, Value const &value) -> std::string {
    return std::format("value '{}' must be less than '{}'", value, limit);
}

template <typename Limit, typename Value>
[[nodiscard]] auto less_than_error_message(Limit const &, Value const &) -> std::string {
    return "value must be less than target";
}

template <std::formattable<char> Target, std::formattable<char> Value>
[[nodiscard]] auto equal_error_message(Target const &target, Value const &value) -> std::string {
    return std::format("value '{}' must be equal to '{}'", value, target);
}

template <typename Target, typename Value>
[[nodiscard]] auto equal_error_message(Target const &, Value const &) -> std::string {
    return "value must be equal to target";
}

template <std::formattable<char> Limit, std::formattable<char> Value>
[[nodiscard]] auto greater_than_error_message(Limit const &limit, Value const &value)
    -> std::string {
    return std::format("value '{}' must be greater than '{}'", value, limit);
}

template <typename Limit, typename Value>
[[nodiscard]] auto greater_than_error_message(Limit const &, Value const &) -> std::string {
    return "value must be greater than target";
}

}  // namespace detail

/// Checks that the parsed value is less than the provided limit
///
/// Usage:
///
/// `less_than<limit>`
template <std::totally_ordered auto Limit>
inline constexpr auto less_than = Validator{
    .fn = [](auto const &value) -> bool {
        return value < Limit;
    },
    .err_fn = [](auto const &value) -> std::string {
        return detail::less_than_error_message(Limit, value);
        // FIXME: for some reason these lines makes clangd crash. But they are correct
        // if constexpr (requires {
        //                   requires detail::both_formattable<decltype(Limit), decltype(value)>;
        //               }) {
        //     return std::format("value '{}' must be less than '{}'", value, Limit);
        // } else {
        // static_cast<void>(value);
        // return "value must be less than target";
        // }
    }};

/// Checks that the parsed value is equal to the provided target
///
/// Usage:
///
/// `equal_to<target>`
template <std::equality_comparable auto Target>
inline constexpr auto equal_to = Validator{
    .fn = [](auto const &value) -> bool {
        return value == Target;
    },
    .err_fn = [](auto const &value) -> std::string {
        return detail::equal_error_message(Target, value);
        // FIXME: for some reason these lines makes clangd crash. But they are correct
        // if constexpr (requires {
        //                   requires detail::both_formattable<decltype(Target), decltype(value)>;
        //               }) {
        //     return std::format("value '{}' must be equal to '{}'", value, Target);
        // } else {
        // static_cast<void>(value);
        // return "value must be equal to target";
        // }
    }};

/// Checks that the parsed value is greater than the provided limit
///
/// Usage:
///
/// `greater_than<limit>`
template <std::totally_ordered auto Limit>
inline constexpr auto greater_than = Validator{
    .fn = [](auto const &value) -> bool {
        return value > Limit;
    },
    .err_fn = [](auto const &value) -> std::string {
        return detail::greater_than_error_message(Limit, value);
        // FIXME: for some reason these lines makes clangd crash. But they are correct
        // if constexpr (requires {
        //                   requires detail::both_formattable<decltype(Limit), decltype(value)>;
        //               }) {
        //     return std::format("value '{}' must be greater than '{}'", value, Limit);
        // } else {
        // static_cast<void>(value);
        // return "value must be greater than target";
        // }
    }};


/// Checks that the parsed value is equal to one of the choices
///
/// Usage:
///
/// `any_of<c0, c1, c2, ...>`
template <auto... Cc>
inline constexpr auto any_of = Or<equal_to<Cc...>>;


/// Apply the provided validator to each element of the parsed value.
///
/// The parsed value must be iterable
///
/// Usage:
///
/// `ForEach<v1, v2, ...>`
template <Validator V>
inline constexpr auto ForEach = Validator{
    .fn = []<typename T>(std::vector<T> const &value) -> bool {
        return std::ranges::all_of(value, [&](auto const &item) {
            return V.fn(item);
        });
    },
    .err_fn = [](auto const &value) -> std::string {
        return std::format("Vector item: {}", V.err_fn(value));
    }};

/// Checks that the parsed value is less or equal than the provided limit
///
/// Usage:
///
/// `less_or_equal<limit>`
template <auto X>
inline constexpr auto less_or_equal = Or<equal_to<X>, less_than<X>>;

/// Checks that the parsed value is greater or equal than the provided limit
///
/// Usage:
///
/// `greater_or_equal<limit>`
template <auto Value>
inline constexpr auto greater_or_equal = Or<equal_to<Value>, greater_than<Value>>;

/// Checks that the parsed value is in the range (extremes excluded)
///
/// Usage:
///
/// `exclusive_range<min, max>`
template <auto Min, auto Max>
inline constexpr auto exclusive_range = And<greater_than<Min>, less_than<Max>>;

/// Checks that the parsed value is in the range (included excluded)
///
/// Usage:
///
/// `inclusive_range<min, max>`
template <auto Min, auto Max>
inline constexpr auto inclusive_range = And<greater_or_equal<Min>, less_or_equal<Max>>;

/// Checks that the parsed value is in the range (including the lower limit and excluding the higher
/// one)
///
/// Usage:
///
/// `half_open_range<min, max>`
template <auto Min, auto Max>
inline constexpr auto half_open_range = And<greater_or_equal<Min>, less_than<Max>>;

/// Can be used to transform a parsed value before feeding it to a validator
///
/// `fn` the transforming function, takes a reference to a constant parsed value and can return
/// anything
template <typename Fn, typename ErrFn>
struct [[nodiscard]] ValidatorTransformer {
    Fn fn;
    ErrFn err_fn;
};

template <typename>
struct IsValidatorTransformer: std::false_type {};

template <typename Fn, typename ErrFn>
struct IsValidatorTransformer<ValidatorTransformer<Fn, ErrFn>>: std::true_type {};

template <typename T>
inline constexpr auto is_validator_transformer_v =
    IsValidatorTransformer<std::remove_cvref_t<T>>::value;

template <typename T>
concept ValidatorTransformerObject = is_validator_transformer_v<T>;

/// Map the provided value to its len (the value must provide a `size` method)
inline constexpr auto len = ValidatorTransformer{
    .fn = [](auto const &value) -> std::size_t {
        return value.size();
    },
    .err_fn = [](std::string msg) -> std::string {
        return std::format("len of {}", msg);
    }};

namespace detail {
template <ValidatorTransformer T1, ValidatorTransformer T2>
constexpr auto compose_transformers() -> ValidatorTransformerObject auto {
    return ValidatorTransformer{
        .fn =
            [](auto const &value) {
                return T2.fn(T1.fn(value));
            },
        .err_fn = [](std::string msg) -> std::string {
            return T2.err_fn(std::format(" {}", T1.err_fn(msg)));
        }};
}

template <ValidatorTransformer T1, ValidatorTransformer T2, ValidatorTransformer... Ts>
requires(sizeof...(Ts) > 0)
constexpr auto compose_transformers() -> ValidatorTransformerObject auto {
    return compose_transformers<compose_transformers<T1, T2>(), Ts...>();
}
}  // namespace detail

/// Compose different tranformers (application is left to right)
///
/// Usage:
///
/// `Compose<t0, t1, t2, ...>`
template <ValidatorTransformer... Ts>
inline constexpr ValidatorTransformerObject auto Compose = detail::compose_transformers<Ts...>();

/// Creates a transformed validator
///
/// The value is transformed and then passed to the validator
///
/// Usage:
///
/// `Pipe<t, v>`
template <ValidatorTransformer T, Validator V>
inline constexpr auto Pipe = Validator{
    .fn = [](auto const &value) -> bool {
        return V.fn(T.fn(value));
    },
    .err_fn =
        [](auto const &x) {
            return T.err_fn(V.err_fn(x));
        }};

}  // namespace args

#endif
