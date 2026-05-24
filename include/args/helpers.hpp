#ifndef ARGS_HELPERS
#define ARGS_HELPERS

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace args::detail {
template <typename T>
struct IsVector: std::false_type {};

template <typename T>
struct IsVector<std::vector<T>>: std::true_type {};

template <typename T>
inline constexpr auto is_vector_v = IsVector<T>::value;

template <typename T>
struct IsStringView: std::false_type {};

template <>
struct IsStringView<std::string_view>: std::true_type {};

template <typename T>
inline constexpr auto is_string_view_v = IsStringView<T>::value;

template <typename T>
struct IsString: std::false_type {};

template <>
struct IsString<std::string>: std::true_type {};

template <typename T>
inline constexpr auto is_string_v = IsString<T>::value;

template <std::invocable F>
class [[nodiscard]] Defer {
public:
    constexpr explicit Defer(F f)
        : m_f{std::move(f)} {}

    Defer(Defer const &) = delete;
    auto operator=(Defer const &) -> Defer & = delete;
    Defer(Defer &&) = delete;
    auto operator=(Defer &&) -> Defer & = delete;

    constexpr ~Defer() {
        m_f();
    }

private:
    F m_f;
};

template <auto C>
concept Not = !C;

template <typename... F>
struct [[nodiscard]] Overload: F... {
    using F::operator()...;
};

}  // namespace args::detail

#endif
