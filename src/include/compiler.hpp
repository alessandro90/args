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
struct [[nodiscard]] TokenCompiler {
    std::tuple<ArgValue<Specs>...> results{};

    [[nodiscard]] auto operator()(tokenizer::ShortFlag short_flag) -> bool {
        auto const selector = [&]<Spec auto S>(ArgValue<S> const &x)
                                  requires detail::ShortFlagCompatible<S>
        {
            return x.spec.short_form.value == short_flag.flag;
        };
        auto const action = []<Spec auto S>(ArgValue<S> &item)
                                requires detail::ShortFlagCompatible<S>
        {
            item.is_used = true;
            if constexpr (is_flag_with_value_v<decltype(item.spec)>) {
                item.value = item.spec.value_if_not_specified;
            } else {
                item.value = true;
            }
        };
        return handle_token(selector, action);
    }

    [[nodiscard]] auto operator()(tokenizer::ShortFlagWithValue) -> bool {
        return false;
    }

    [[nodiscard]] auto operator()(tokenizer::LongFlagWithValue) -> bool {
        return false;
    }

    [[nodiscard]] auto operator()(tokenizer::LongFlag) -> bool {
        return false;
    }

    [[nodiscard]] auto operator()(tokenizer::FlagGroup) -> bool {
        return false;
    }

    [[nodiscard]] auto operator()(tokenizer::Argument) -> bool {
        return false;
    }

private:
    template <typename Selector, typename Action>
    [[nodiscard]] auto handle_token(Selector selector, Action action) -> bool {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (... || [&]() {  // 'or' will execute until the first 'true'
                using ArgType = std::tuple_element_t<Is, std::tuple<ArgValue<Specs>...>>;
                if constexpr (std::is_invocable_v<Selector, ArgType const &>) {
                    auto &item = std::get<Is>(results);
                    if (!selector(item)) {
                        return false;  // no match, keep looping
                    }
                    action(item);
                    return true;
                } else {
                    return false;  // not callable, keep looping
                }
            }());
        }(std::make_index_sequence<sizeof...(Specs)>());
    }
};

}  // namespace detail

template <Spec auto... Specs>
[[nodiscard]] auto compile(
    std::span<tokenizer::Token> tokens,
    Rules<Specs...>)  // NOTE: span may be just a range. Would allow to process tokes as a stream
                      // without dynamic allocation
    -> std::optional<Args<Specs...>> {
    auto token_compiler = detail::TokenCompiler<Specs...>{};
    for (auto const token : tokens) {
        bool const ok = std::visit(token_compiler, token);
        if (!ok) {
            return std::nullopt;
        }
    }
    return Args{std::move(token_compiler).results};
}

}  // namespace args::compiler

#endif
