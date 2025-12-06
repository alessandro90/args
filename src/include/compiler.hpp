#ifndef CPP_ARGS_COMPILER_HEADER
#define CPP_ARGS_COMPILER_HEADER

#include <cstddef>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include "tokenizer.hpp"
#include "types.hpp"

namespace args::compiler {
namespace detail {

template <auto S>
concept ShortFlagCompatible =
    Spec<decltype(S)> && S.short_form.has_value
    && (is_flag_v<decltype(S)> || (is_flag_with_value_v<decltype(S)> && S.allow_missing_value));

template <Spec auto... Specs>
struct [[nodiscard]] TokenVisitor {
    std::tuple<ResultValue<Specs>...> &results;

    [[nodiscard]] auto operator()(tokenizer::ShortFlag short_flag) const -> bool {
        auto const selector = [&]<Spec auto S>(ResultValue<S> const &x)
                                  requires detail::ShortFlagCompatible<S>
        {
            return x.spec.short_form.value == short_flag.flag;
        };
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (... || [&]() {  // 'or' will execute until the first 'true'
                using ArgType = std::tuple_element_t<Is, std::tuple<ResultValue<Specs>...>>;
                if constexpr (std::is_invocable_v<decltype(selector), ArgType const &>) {
                    auto &item = std::get<Is>(results);
                    if (!selector(item)) {
                        return false;  // no match, keep looping
                    }
                    // match found, do side effects and return 'true' to stop iteration
                    item.m_is_used = true;
                    if constexpr (is_flag_with_value_v<decltype(item.spec)>) {
                        item.value = item.spec.value_if_not_specified;
                    } else {
                        item.value = true;
                    }
                    return true;
                } else {
                    return false;  // not callable, keep looping
                }
            }());
        }(std::make_index_sequence<sizeof...(Specs)>());
    }

    [[nodiscard]] auto operator()(tokenizer::ShortFlagWithValue) const -> bool {
        return false;
    }

    [[nodiscard]] auto operator()(tokenizer::LongFlagWithValue) const -> bool {
        return false;
    }

    [[nodiscard]] auto operator()(tokenizer::LongFlag) const -> bool {
        return false;
    }

    [[nodiscard]] auto operator()(tokenizer::FlagGroup) const -> bool {
        return false;
    }

    [[nodiscard]] auto operator()(tokenizer::Argument) const -> bool {
        return false;
    }
};

}  // namespace detail

template <Spec auto... Specs>
[[nodiscard]] auto compile(
    std::span<tokenizer::Token> tokens,
    Rules<Specs...>)  // NOTE: span may be just a range. Would allow to process tokes as a stream
                      // without dynamic allocation
    -> std::optional<Result<Specs...>> {
    auto results = std::tuple<ResultValue<Specs>...>{};
    for (auto const &token : tokens) {
        bool const ok = std::visit(detail::TokenVisitor{results}, token);
        if (!ok) {
            return std::nullopt;
        }
    }
    return Result{results};
}

}  // namespace args::compiler

#endif
