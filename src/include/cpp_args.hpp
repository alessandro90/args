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

template <Str Usage, Str Description, auto... Specs, auto... Gg>
[[nodiscard]] auto try_parse(
    int argc,
    char **argv,
    Rules<Usage, Description, Specs...> rules,
    MutuallyExclusiveGroups<Gg...> mutually_exclusive) -> compile_result_t<Specs...> {
    // TODO: move this inside compile (need to check also subcommands)
    static_assert(
        args::detail::are_valid_mutually_exclusive_groups(rules, mutually_exclusive),
        "Invalid mutually exclusive groups");
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
    return compiler::compile(std::move(tokens), rules, mutually_exclusive);
}

template <Str Usage, Str Description, auto... Specs>
[[nodiscard]] auto try_parse(int argc, char **argv, Rules<Usage, Description, Specs...> rules)
    -> compile_result_t<Specs...> {
    return try_parse(argc, argv, rules, MutuallyExclusiveGroups<>{});
}

template <Str Usage, Str Description, auto... Specs, auto... Gg>
[[nodiscard]] auto try_parse_or_exit_program(
    int argc,
    char **argv,
    Rules<Usage, Description, Specs...> rules,
    MutuallyExclusiveGroups<Gg...> mutually_exclusive) -> Args<Specs...> {
    auto args = try_parse(argc, argv, rules, mutually_exclusive);
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

template <Str Usage, Str Description, auto... Specs>
[[nodiscard]] auto try_parse_or_exit_program(
    int argc, char **argv, Rules<Usage, Description, Specs...> rules) -> Args<Specs...> {
    return try_parse_or_exit_program(argc, argv, rules, MutuallyExclusiveGroups<>{});
}
}  // namespace args

#endif
