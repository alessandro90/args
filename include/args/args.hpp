#ifndef ARGS_HEADER
#define ARGS_HEADER

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <print>
#include "colors.hpp"
#include "compiler.hpp"
#include "helpers.hpp"  // for Str
#include "tokenizer.hpp"
#include "types.hpp"

namespace args {
template <Str Usage, Str Description, auto... Ops, auto... Gg>
[[nodiscard]] auto try_parse(
    int argc,
    char **argv,
    Options<Usage, Description, Ops...> opts,
    MutuallyExclusiveGroups<Gg...> mutually_exclusive) -> compile_result_t<Ops...> {
    // TODO: move this inside compile (need to check also subcommands)
    static_assert(
        args::detail::are_valid_mutually_exclusive_groups(opts, mutually_exclusive),
        "Invalid mutually exclusive groups");
    if (argc <= 1) {
        return NoArguments{};
    }
    auto const constify = [](char **x) -> char const *const * {
        return x;
    };
    // Skip the program name
    auto s = std::span{std::next(constify(argv)), static_cast<std::size_t>(argc - 1)};
    auto tokens = tokenizer::tokenize(s);
    if (!tokens.has_value()) {
        return compile_result_t<Ops...>{Error{.message = std::move(tokens).error()}};
    }
    std::span<args::tokenizer::token_t const> tks = tokens.value();
    return compiler::compile(tks, opts, mutually_exclusive);
}

template <Str Usage, Str Description, auto... Ops>
[[nodiscard]] auto try_parse(int argc, char **argv, Options<Usage, Description, Ops...> opts)
    -> compile_result_t<Ops...> {
    return try_parse(argc, argv, opts, MutuallyExclusiveGroups<>{});
}

template <Str Usage, Str Description, auto... Ops, auto... Gg>
[[nodiscard]] auto parse_or_exit(
    int argc,
    char **argv,
    Options<Usage, Description, Ops...> opts,
    MutuallyExclusiveGroups<Gg...> mutually_exclusive) -> Args<Ops...> {
    auto args = try_parse(argc, argv, opts, mutually_exclusive);
    if (has_error(args)) {
        auto const error = get_error(args);
        // std::println(stderr, "{} {}", color::bold("{}", color::red("ERROR:")), error.message);
        std::println(
            stderr,
            "{} {}",
            color::color_format<color::cbold + color::cred>("{}", "ERROR:"),
            error.message);
        std::println(stderr, "{}", opts.help());
        std::exit(EXIT_FAILURE);  // NOLINT(concurrency-mt-unsafe)
    }
    if (has_help(args)) {
        auto const help = get_help(args);
        std::println("{}", help.message);
        std::exit(EXIT_SUCCESS);  // NOLINT(concurrency-mt-unsafe)
    }
    if (is_empty(args)) {
        std::println(stderr, "No arguments provided");
        std::println(stderr, "{}", opts.help());
        std::exit(EXIT_FAILURE);  // NOLINT(concurrency-mt-unsafe)
    }
    return get_args(std::move(args));
}

template <Str Usage, Str Description, auto... Ops>
[[nodiscard]] auto parse_or_exit(int argc, char **argv, Options<Usage, Description, Ops...> opts)
    -> Args<Ops...> {
    return parse_or_exit(argc, argv, opts, MutuallyExclusiveGroups<>{});
}
}  // namespace args

#endif
