#ifndef CPP_ARGS_TYPE_HELPERS
#define CPP_ARGS_TYPE_HELPERS

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
struct IsStringView: std::false_type {};

template <>
struct IsStringView<std::string_view>: std::true_type {};

template <typename T>
struct IsString: std::false_type {};

template <>
struct IsString<std::string>: std::true_type {};

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
}  // namespace args::detail

#endif
