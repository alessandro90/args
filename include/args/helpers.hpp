#ifndef ARGS_HELPERS
#define ARGS_HELPERS

#include <concepts>
#include <print>
#ifndef NDUBUG
    #include <cstdio>
    #include <cstdlib>
    #include <source_location>
#endif
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace args::detail {
template <typename T>
struct IsVector: std::false_type {};

template <typename T, typename A>
struct IsVector<std::vector<T, A>>: std::true_type {};

template <typename T>
inline constexpr auto is_vector_v = IsVector<T>::value;

template <typename T>
concept StdVector = is_vector_v<T>;

template <typename T>
struct IsStringView: std::false_type {};

template <>
struct IsStringView<std::string_view>: std::true_type {};

template <typename T>
inline constexpr auto is_string_view_v = IsStringView<T>::value;

template <typename T>
concept StdStringView = is_string_view_v<T>;

template <typename T>
struct IsString: std::false_type {};

template <>
struct IsString<std::string>: std::true_type {};

template <typename T>
inline constexpr auto is_string_v = IsString<T>::value;

template <typename T>
concept StdString = is_string_v<T>;

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

#ifndef NDEBUG
[[noreturn]] inline auto args_log_and_abort(
    std::string_view msg, std::source_location loc = std::source_location::current()) -> void {
    std::println(
        stderr,
        "File: {} ({}:{}) `{}`: {}",
        loc.file_name(),
        loc.line(),
        loc.column(),
        loc.function_name(),
        msg);
    std::abort();
}
#else
    #define args_log_and_abort(...)
#endif


#endif
