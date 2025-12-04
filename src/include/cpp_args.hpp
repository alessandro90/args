#ifndef CPP_ARGS_HEADER
#define CPP_ARGS_HEADER

#include <cstddef>
#include <optional>
#include <variant>
#include "compiler.hpp"
#include "tokenizer.hpp"
#include "types.hpp"

namespace args {

using Error = std::variant<tokenizer::Error>;

template <Spec auto... Specs>
[[nodiscard]] auto try_parse(int argc, char **argv, Rules<Specs...> rules)
    -> std::optional<Result<Specs...>> {
    auto const s = std::span{argv, static_cast<std::size_t>(argc)};
    return tokenizer::tokenize(s).and_then(
        [rules](std::vector<tokenizer::Token> tokens) { return compiler::compile(tokens, rules); });
}
}  // namespace args

#endif
