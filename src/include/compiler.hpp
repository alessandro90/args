#ifndef CPP_ARGS_COMPILER_HEADER
#define CPP_ARGS_COMPILER_HEADER

#include <cstddef>
#include <optional>
#include <span>
#include <utility>
#include "tokenizer.hpp"
#include "types.hpp"

namespace args::compiler {
// TODO: how to assert that all required args have been parsed?
// should add 'required' and 'Option default' to the spec?
// may implement wrappers to Specs for Optional/Required. For example
// Optional<ShortFlag>, Required otherwise. Optional has an additional
// field, the default field if not never parsed

template <Spec auto... Specs>
struct TokenVisitor {
    std::tuple<ResultValue<Specs>...> &results;

    [[nodiscard]] auto operator()(tokenizer::ShortFlag short_flag) const -> bool {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (... || [&]() {
                if constexpr (IsFlag_v<std::tuple_element_t<Is, std::tuple<decltype(Specs)...>>>) {
                    auto &item = std::get<Is>(results);
                    if (!item.spec.short_form.has_value) {
                        return false;
                    }
                    if (item.spec.short_form.value != short_flag.flag) {
                        return false;
                    }
                    item.value = true;
                    return true;
                } else {
                    return false;
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

template <Spec auto... Specs>
[[nodiscard]] auto compile(std::span<tokenizer::Token> tokens,
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
