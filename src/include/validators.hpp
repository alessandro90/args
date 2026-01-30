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

template <Validator... Vs>
inline constexpr auto Or = make_validator(
    [](auto const &value) -> bool {
        return (... || Vs.fn(value));
    },
    [](auto const &value) -> std::string {
        auto s = std::string(1, '(');
        s.append_range(
            std::array{std::format("'{}'", Vs.err_fn(value))...} | std::views::join_with(" or ")
            | std::ranges::to<std::string>());
        s.push_back(')');
        return s;
    });

template <Validator... Vs>
inline constexpr auto And = make_validator(
    [](auto const &value) -> bool {
        return (... && Vs.fn(value));
    },
    [](auto const &value) -> std::string {
        auto s = std::string(1, '(');
        s.append_range(
            std::array{std::format("'{}'", Vs.err_fn(value))...} | std::views::join_with(" and ")
            | std::ranges::to<std::string>());
        s.push_back(')');
        return s;
    });

template <Validator... Vs>
inline constexpr auto Xor = make_validator(
    [](auto const &value) -> bool {
        return (... ^ Vs.fn(value));
    },
    [](auto const &value) -> std::string {
        auto s = std::string(1, '(');
        s.append_range(
            std::array{std::format("'{}'", Vs.err_fn(value))...} | std::views::join_with(" xor ")
            | std::ranges::to<std::string>());
        s.push_back(')');
        return s;
    });

template <Validator V>
inline constexpr auto Not = make_validator(
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

namespace detail {
template <typename T, typename E>
concept both_formattable = std::formattable<T, char> && std::formattable<E, char>;
}  // namespace detail

template <std::totally_ordered auto Limit>
inline constexpr auto less_than = Validator{
    .fn = [](auto const &value) -> bool {
        return value < Limit;
    },
    .err_fn = [](auto const &value) -> std::string {
        if constexpr (requires {
                          requires detail::both_formattable<decltype(Limit), decltype(value)>;
                      }) {
            return std::format("Value '{}' must be less than '{}'", value, Limit);
        } else {
            static_cast<void>(value);
            return "Value must be less than target";
        }
    }};

template <std::equality_comparable auto Target>
inline constexpr auto equal = Validator{
    .fn = [](auto const &value) -> bool {
        return value == Target;
    },
    .err_fn = [](auto const &value) -> std::string {
        if constexpr (requires {
                          requires detail::both_formattable<decltype(Target), decltype(value)>;
                      }) {
            return std::format("Value '{}' must be equal to '{}'", value, Target);
        } else {
            static_cast<void>(value);
            return "Value must be equal to target";
        }
    }};

template <std::totally_ordered auto Limit>
inline constexpr auto greater_than = Validator{
    .fn = [](auto const &value) -> bool {
        return value > Limit;
    },
    .err_fn = [](auto const &value) -> std::string {
        if constexpr (requires {
                          requires detail::both_formattable<decltype(Limit), decltype(value)>;
                      }) {
            return std::format("Value '{}' must be greater than '{}'", value, Limit);
        } else {
            static_cast<void>(value);
            return "Value must be greater than target";
        }
    }};

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

template <auto X>
inline constexpr auto less_or_equal = Or<equal<X>, less_than<X>>;

template <auto Value>
inline constexpr auto greater_or_equal = Or<equal<Value>, greater_than<Value>>;

template <auto Min, auto Max>
inline constexpr auto exclusive_range = And<greater_than<Min>, less_than<Max>>;

template <auto Min, auto Max>
inline constexpr auto inclusive_range = And<greater_or_equal<Min>, less_or_equal<Max>>;

template <auto Min, auto Max>
inline constexpr auto half_open_range = And<greater_or_equal<Min>, less_than<Max>>;

template <typename Fn>
struct [[nodiscard]] ValidatorTransformer {
    Fn fn;
};

template <typename>
struct IsValidatorTransformer: std::false_type {};

template <typename Fn>
struct IsValidatorTransformer<ValidatorTransformer<Fn>>: std::true_type {};

template <typename T>
inline constexpr auto is_validator_transformer_v =
    IsValidatorTransformer<std::remove_cvref_t<T>>::value;

template <typename T>
concept AValidatorTransformer = is_validator_transformer_v<T>;

inline constexpr auto len = ValidatorTransformer{.fn = [](auto const &value) -> std::size_t {
    return value.size();
}};

namespace detail {
template <ValidatorTransformer T1, ValidatorTransformer T2>
constexpr auto compose_transformers() -> AValidatorTransformer auto {
    return ValidatorTransformer{.fn = [](auto const &value) {
        return T2.fn(T1.fn(value));
    }};
}

template <ValidatorTransformer T1, ValidatorTransformer T2, ValidatorTransformer... Ts>
requires(sizeof...(Ts) > 0)
constexpr auto compose_transformers() -> AValidatorTransformer auto {
    return compose_transformers<compose_transformers<T1, T2>(), Ts...>();
}
}  // namespace detail

template <ValidatorTransformer... Ts>
inline constexpr AValidatorTransformer auto Compose = detail::compose_transformers<Ts...>();

template <ValidatorTransformer T, Validator V>
inline constexpr auto Pipe = Validator{
    .fn = [](auto const &value) -> bool {
        return V.fn(T.fn(value));
    },
    .err_fn = V.err_fn};

}  // namespace args

#endif
