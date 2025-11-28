#ifndef CPP_ARGS_COMPILER_HEADER
#define CPP_ARGS_COMPILER_HEADER

#include "tokenizer.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace args::compiler {

template <typename T>
struct PlainOptional {
    bool has_value;
    T value;

    [[nodiscard]] constexpr bool operator==(PlainOptional const &) const = default;

    static consteval auto empty() -> PlainOptional {
        return PlainOptional{.has_value = false, .value = T{}};
    }

    static consteval auto with(T value) -> PlainOptional {
        return PlainOptional{.has_value = true, .value = value};
    }
};

template <std::size_t N>
struct StrView {
    std::array<char, N + 1> view;

    consteval StrView(char const (&s)[N + 1]) {  // NOLINT
        std::ranges::copy(s, view.begin());
    }

    [[nodiscard]] constexpr bool operator==(StrView const &) const = default;
};

template <std::size_t N>
StrView(char const (&s)[N]) -> StrView<N - 1>;  // NOLINT

template <std::size_t N>
struct [[nodiscard]] FlagSpec {
    StrView<N> long_form;
    PlainOptional<char> short_form;
    static constexpr bool is_spec = true;
    using value_t = bool;

    [[nodiscard]] constexpr bool operator==(FlagSpec const &) const = default;
};

template <typename>
struct IsFlagSpec: std::false_type {};

template <std::size_t N>
struct IsFlagSpec<FlagSpec<N>>: std::true_type {};

template <typename T>
inline constexpr bool IsFlagSpec_v = IsFlagSpec<T>::value;

template <typename V>
struct [[nodiscard]] FlagWithValue {
    std::string_view long_form;
    std::optional<char> short_form;
    static constexpr bool is_spec = true;
    using value_t = V;

    [[nodiscard]] constexpr bool operator==(FlagWithValue const &) const = default;
};

template <typename P>
struct [[nodiscard]] Positional {
    std::optional<P> default_value;
    std::size_t index;
    static constexpr bool is_spec = true;
    using value_t = P;

    [[nodiscard]] constexpr bool operator==(Positional const &) const = default;
};

template <typename S>
concept Spec = S::is_spec;

template <Spec auto... Specs>
struct Rules {};

template <Spec auto S>
struct [[nodiscard]] ResultValue {
    decltype(S)::value_t value;
    static constexpr auto spec = S;
};

template <Spec auto... Specs>
struct [[nodiscard]] Result {
    std::tuple<ResultValue<Specs>...> results;

    template <Spec auto S>
    auto get() -> ResultValue<S> const & {
        return std::get<ResultValue<S>>(results);
    }
};

template <Spec auto... Specs>
struct TokenVisitor {
    std::tuple<ResultValue<Specs>...> &results;

    [[nodiscard]] auto operator()(tokenizer::ShortFlag short_flag) const -> bool {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (... || [&]() {
                if constexpr (IsFlagSpec_v<
                                  std::tuple_element_t<Is, std::tuple<decltype(Specs)...>>>) {
                    auto &item = std::get<Is>(results);
                    if (!item.spec.short_form.has_value) { return false; }
                    if (item.spec.short_form.value != short_flag.flag) { return false; }
                    item.value = true;
                    return true;
                } else {
                    return false;
                }
            }());
        }(std::make_index_sequence<sizeof...(Specs)>());
    }

    [[nodiscard]] auto operator()(tokenizer::ShortFlagWithValue) const -> bool { return false; }

    [[nodiscard]] auto operator()(tokenizer::LongFlagWithValue) const -> bool { return false; }

    [[nodiscard]] auto operator()(tokenizer::LongFlag) const -> bool { return false; }

    [[nodiscard]] auto operator()(tokenizer::FlagGroup) const -> bool { return false; }

    [[nodiscard]] auto operator()(tokenizer::Argument) const -> bool { return false; }
};

template <Spec auto... Specs>
[[nodiscard]] auto compile(std::span<tokenizer::Token> tokens,
                           Rules<Specs...>)  // note span may be just a range
    -> std::optional<Result<Specs...>> {
    auto results = std::tuple<ResultValue<Specs>...>{};
    for (auto const token : tokens) {
        bool const ok = std::visit(TokenVisitor{results}, token);
        if (!ok) { return std::nullopt; }
    }
    return Result{results};
}


}  // namespace args::compiler

#endif
