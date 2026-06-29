#ifndef ARGS_HELPERS
#define ARGS_HELPERS

#include <algorithm>
#include <array>
#include <concepts>
#include <meta>
#include <string_view>
#include <type_traits>
#include <utility>

namespace args::detail {
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
concept SizedContainer = requires(C c) {
    { c.size() } -> std::convertible_to<std::size_t>;
};

template <typename C>
concept ValuedContainer = requires { typename C::value_type; };

template <typename C>
concept VecLikeContainer =
    SizedContainer<C> && ValuedContainer<C> && requires(C &c, typename C::value_type v) {
        c.push_back(std::move(v));
        c.append_range(std::move(c));
    };

template <typename C>
concept SetLikeContainer =
    SizedContainer<C> && ValuedContainer<C> && requires(C &c, typename C::value_type v) {
        c.insert(std::move(v));
        c.insert_range(std::move(c));
    };

template <typename C>
concept PushContainer =
    std::default_initializable<C> && (SetLikeContainer<C> || VecLikeContainer<C>);

template <typename C>
concept VecLikeContainerExtendableWithRange =
    VecLikeContainer<C> && requires(C &c) { c.append_range(std::move(c)); };

template <typename C>
concept SetLikeContainerExtendableWithRange =
    SetLikeContainer<C> && requires(C &c) { c.insert_range(std::move(c)); };

template <typename C>
concept InplaceContainer =
    std::default_initializable<C>
    && (VecLikeContainerExtendableWithRange<C> || SetLikeContainerExtendableWithRange<C>);

template <typename T>
using default_fn_ptr_t = T (*)();

template <typename>
struct IsDefaultFnPtr: std::false_type {};

template <typename T>
struct IsDefaultFnPtr<default_fn_ptr_t<T>>: std::true_type {};

template <typename T>
inline constexpr auto is_def_fn_ptr_v = IsDefaultFnPtr<T>::value;

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

#endif
