#ifndef CPP_ARGS_HEADER
#define CPP_ARGS_HEADER

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <print>
#include "compiler.hpp"
#include "tokenizer.hpp"
#include "types.hpp"

namespace args {

template <Str Usage, Str Description, auto... Specs>
[[nodiscard]] auto try_parse(int argc, char **argv, Rules<Usage, Description, Specs...> rules)
    -> compile_result_t<Specs...> {
    if (argc <= 1) {
        return Error{.message = "No command line arguments provided"};
    }
    auto const constify = [](char **x) -> char const *const * {
        return x;
    };
    // Skip the program name
    auto const s = std::span{std::next(constify(argv)), static_cast<std::size_t>(argc - 1)};
    auto tokens = tokenizer::tokenize(s);
    if (!tokens.has_value()) {
        return compile_result_t<Specs...>{Error{.message = std::move(tokens).error()}};
    }
    return compiler::compile(std::move(tokens), rules);
}

template <Str Usage, Str Description, auto... Specs>
[[nodiscard]] auto try_parse_or_exit_program(
    int argc, char **argv, Rules<Usage, Description, Specs...> rules) -> Args<Specs...> {
    auto args = try_parse(argc, argv, rules);
    if (has_error(args)) {
        auto const error = get_error(args);
        std::println(stderr, "ERROR: {}", error.message);
        std::println(stderr, "{}", rules.help());
        std::exit(EXIT_FAILURE);  // NOLINT(concurrency-mt-unsafe)
    }
    if (has_help(args)) {
        auto const help = get_help(args);
        std::println("{}", help.message);
        std::exit(EXIT_SUCCESS);  // NOLINT(concurrency-mt-unsafe)
    }
    return get_args(std::move(args));
}
}  // namespace args

#endif
