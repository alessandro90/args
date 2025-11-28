#ifndef CPP_ARGS_HEADER
#define CPP_ARGS_HEADER

#include "compiler.hpp"
#include "tokenizer.hpp"
#include <cassert>
#include <cstddef>
#include <expected>
#include <optional>
#include <variant>

namespace args {

using Error = std::variant<tokenizer::Error>;

template <compiler::Spec auto... Specs>
[[nodiscard]] auto try_parse(int argc, char **argv, compiler::Rules<Specs...> rules)
    -> std::optional<compiler::Result<Specs...>> {
    auto const s = std::span{argv, static_cast<std::size_t>(argc)};
    // FIXME: this should not compile, not a monadic operation
    return tokenizer::tokenize(s).and_then(
        [rules](std::vector<tokenizer::Token> tokens) { return compiler::compile(tokens, rules); });
}
}  // namespace args

#endif
