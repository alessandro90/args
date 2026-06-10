#ifndef ARGS_HELPERS
#define ARGS_HELPERS

#include <algorithm>
#include <array>
#include <concepts>
#include <meta>
#include <print>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#ifndef NDUBUG
    #include <cstdio>
    #include <cstdlib>
    #include <source_location>
#endif

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

template <typename T>
consteval auto name_of() -> std::string_view {
    return std::meta::display_string_of(^^T);
}

template <typename C>
concept ValuedContainer = requires { typename C::value_type; };

template <typename C>
concept VecLikeContainer =
    ValuedContainer<C> && requires(C &c, typename C::value_type v) { c.push_back(v); };

template <typename C>
concept SetLikeContainer =
    ValuedContainer<C> && requires(C &c, typename C::value_type v) { c.insert(v); };

template <typename C>
concept InplaceContainer = VecLikeContainer<C> || SetLikeContainer<C>;

}  // namespace args::detail

namespace args {

/// A compile time string_view-like object
///
/// Should be used to define string-like quantities needed at compile time
///
/// Note: it can be compared to std::string_view and std::string, and therefore
/// it can be used in validators that deal with those types
///
/// Usage:
///
/// `"a compile-time string-like object"_str`
template <std::size_t N>
struct [[nodiscard]] Str {
    static constexpr auto s_size = N;

    std::array<char, N + 1> chars{};

    consteval Str() noexcept = default;

    consteval Str(char const (&s)[N + 1]) {  // NOLINT
        std::ranges::copy(s, chars.begin());
    }

    [[nodiscard]] constexpr auto as_string_view() const -> std::string_view {
        return std::string_view{chars.data()};
    }

    [[nodiscard]] static constexpr auto is_empty() noexcept -> bool {
        return N == 0;
    }

    template <std::size_t M>
    [[nodiscard]] constexpr auto operator==(Str<M> const &rhs) -> bool {
        return N == M && chars == rhs.chars;
    }

    [[nodiscard]] constexpr auto operator==(std::string_view rhs) -> bool {
        return as_string_view() == rhs;
    }

    [[nodiscard]] constexpr auto operator==(std::string const &rhs) -> bool {
        return as_string_view() == rhs;
    }
};

template <std::size_t N>
Str(char const (&s)[N]) -> Str<N - 1>;  // NOLINT
}  // namespace args

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
