#ifndef CPP_ARGS_HEADER
#define CPP_ARGS_HEADER

#include <cstddef>
#include <expected>
#include <iterator>
#include <string>
#include "compiler.hpp"
#include "tokenizer.hpp"
#include "types.hpp"

namespace args {

template <Spec auto... Specs>
[[nodiscard]] auto try_parse(int argc, char **argv, Rules<Specs...> rules)
    -> std::expected<Args<Specs...>, std::string> {
    if (argc <= 1) {
        return std::unexpected("No command line arguments provided");
    }
    auto const constify = [](char **x) -> char const *const * {
        return x;
    };
    // Skip the program name
    auto const s = std::span{std::next(constify(argv)), static_cast<std::size_t>(argc - 1)};
    return tokenizer::tokenize(s).and_then([rules](std::vector<tokenizer::Token> tokens) {
        return compiler::compile(tokens, rules);
    });
}
}  // namespace args

#endif
