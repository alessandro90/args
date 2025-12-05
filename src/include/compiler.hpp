#ifndef CPP_ARGS_COMPILER_HEADER
#define CPP_ARGS_COMPILER_HEADER

#include <cstddef>
#include <optional>
#include <span>
#include <type_traits>
#include "tokenizer.hpp"
#include "types.hpp"

namespace args::compiler {
// TODO: how to assert that all required args have been parsed?
// should add 'required' and 'Option default' to the spec?
// may implement wrappers to Specs for Optional/Required. For example
// Optional<ShortFlag>, Required otherwise. Optional has an additional
// field, the default field if not never parsed

namespace detail {

template <std::size_t N, typename F, typename Arg, typename... Args>
[[nodiscard]] constexpr auto tuple_find_impl(std::tuple<Arg, Args...> &t, F selector) {
    static constexpr bool is_last = N == sizeof...(Args) - 1;
    if constexpr (std::is_invocable_v<F, Arg const &>) {
        auto const &item = std::get<N>(t);
        if (selector(item)) {
            return &item;
        }
        if constexpr (is_last) {
            return nullptr;
        } else {
            return tuple_find_impl<N + 1>(t, selector);
        }
    } else {
        if constexpr (is_last) {
            static_assert(false, "invalid selector match");
        } else {
            return tuple_find_impl<N + 1>(t, selector);
        }
    }
}

template <typename F, typename... Args>
[[nodiscard]] constexpr auto tuple_find(std::tuple<Args...> &t, F selector) {
    return tuple_find_impl<0>(t, selector);
}
}  // namespace detail

template <Spec auto... Specs>
struct TokenVisitor {
    std::tuple<ResultValue<Specs>...> &results;

    [[nodiscard]] auto operator()(tokenizer::ShortFlag short_flag) const -> bool {
        auto const selector = [&]<Spec auto S>(ResultValue<S> const &x)
                                  requires IsFlag_v<decltype(S)>
        {
            if (!x.spec.short_form.has_value) {
                return false;
            }
            if (x.spec.short_form.value != short_flag.flag) {
                return false;
            }
            return false;
        };
        auto *item = detail::tuple_find(results, selector);
        if (item == nullptr) {
            return false;
        }
        item->value = true;
        return true;
        // return [&]<std::size_t... Is>(std::index_sequence<Is...>) {  // TODO: use
        // use tuple_find
        //     return (... || [&]() {
        //         if constexpr (IsFlag_v<std::tuple_element_t<Is,
        //         std::tuple<decltype(Specs)...>>>)
        //         {
        //             auto &item = std::get<Is>(results);
        //             if (!item.spec.short_form.has_value) {
        //                 return false;
        //             }
        //             if (item.spec.short_form.value != short_flag.flag) {
        //                 return false;
        //             }
        //             item.value = true;
        //             return true;
        //         } else {
        //             return false;
        //         }
        //     }());
        // }(std::make_index_sequence<sizeof...(Specs)>());
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

template <Spec auto... Specs>
[[nodiscard]] auto compile(
    std::span<tokenizer::Token> tokens,
    Rules<Specs...>)  // note span may be just a range
    -> std::optional<Result<Specs...>> {
    auto results = std::tuple<ResultValue<Specs>...>{};
    for (auto const token : tokens) {
        bool const ok = std::visit(TokenVisitor{results}, token);
        if (!ok) {
            return std::nullopt;
        }
    }
    return Result{results};
}

}  // namespace args::compiler

#endif
