#ifndef CPP_ARGS_VALIDATORS
#define CPP_ARGS_VALIDATORS

#include <algorithm>
#include <expected>
#include <format>
#include <string>
#include <type_traits>
#include <vector>

namespace args {

using validator_result_t = std::expected<void, std::string>;

template <typename V>
struct [[nodiscard]] Validator {
    V fn;

    template <typename T>
    [[nodiscard]] constexpr auto operator()(T const &value) const -> validator_result_t {
        return fn(value);
    }

    template <typename W>
    constexpr auto operator|(Validator<W> v) const {
        return make_validator([vv = fn, v]<typename T>(T const &value) -> validator_result_t {
            return vv(value).or_else([&] {
                return v(value);
            });
        });
    }

    template <typename W>
    constexpr auto operator&(Validator<W> v) const {
        return make_validator([vv = fn, v]<typename T>(T const &value) -> validator_result_t {
            return vv(value).and_then([&] {
                return v(value);
            });
        });
    }

    template <typename W>
    constexpr auto operator^(Validator<W> v) const {
        return make_validator([vv = fn, v]<typename T>(T const &value) -> validator_result_t {
            auto const res_vv = vv(value);
            auto const res_v = v(value);
            if ((res_vv && res_v) || (!res_vv && !res_v)) {
                return {};
            }
            return std::unexpected("'xor' validation not satisfied");
        });
    }

    template <typename W>
    constexpr auto operator!() const {
        return make_validator([vv = fn]<typename T>(T const &value) -> validator_result_t {
            if (!vv(value)) {
                return {};
            }
            return std::unexpected("'not' validation not satisfied");
        });
    }
};

template <typename V>
constexpr auto make_validator(V v) -> Validator<V> {
    return Validator{.fn = v};
}

template <typename>
struct IsValidator: std::false_type {};

template <typename V>
struct IsValidator<Validator<V>>: std::true_type {};

template <typename T>
inline constexpr auto is_validator_v = IsValidator<T>::value;

template <typename T>
concept AValidator = is_validator_v<T>;

inline constexpr auto always = Validator{.fn = [](auto const &) -> validator_result_t {
    return {};
}};

using always_t = std::remove_cvref_t<decltype(always)>;

template <typename T>
constexpr auto less_than(T x) -> AValidator auto {
    return Validator{.fn = [value = std::move(x)](T const &other) -> validator_result_t {
        if (other >= value) {
            return std::unexpected(std::format("Value must be less than {}, got {}", value, other));
        }
        return {};
    }};
}

template <typename T>
constexpr auto equal(T x) -> AValidator auto {
    return Validator{.fn = [value = std::move(x)](T const &other) -> validator_result_t {
        if (other != value) {
            return std::unexpected(std::format("Value must be equal to {}, got {}", value, other));
        }
        return {};
    }};
}

template <typename T>
constexpr auto greater_than(T x) -> AValidator auto {
    return Validator{.fn = [value = std::move(x)](T const &other) -> validator_result_t {
        if (other <= value) {
            return std::unexpected(
                std::format("Value must be greater than {}, got {}", value, other));
        }
        return {};
    }};
}

template <typename V>
constexpr auto for_each(Validator<V> v) -> AValidator auto {
    return Validator{
        .fn = [validator =
                   std::move(v)]<typename T>(std::vector<T> const &other) -> validator_result_t {
            for (auto const &value : other) {
                auto result = validator(value);
                if (result.has_error()) {
                    return result;
                }
            }
            return {};
        }};
}

constexpr auto less_or_equal(auto x) -> AValidator auto {
    return equal(x) | less_than(x);
};

constexpr auto greater_or_equal(auto value) -> AValidator auto {
    return equal(value) | greater_than(value);
};

constexpr auto exclusive_range(auto min, auto max) -> AValidator auto {
    return greater_than(min) & less_than(max);
};

constexpr auto inclusive_range(auto min, auto max) -> AValidator auto {
    return greater_or_equal(min) & less_or_equal(max);
};

constexpr auto half_open_range(auto min, auto max) -> AValidator auto {
    return greater_or_equal(min) & less_than(max);
};

}  // namespace args

#endif
