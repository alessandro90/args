#ifndef CPP_ARGS_HEADER
#define CPP_ARGS_HEADER

#include "tokenizer.hpp"
#include <cassert>
#include <cstddef>
#include <expected>
#include <iterator>
#include <ranges>
#include <variant>

namespace args {

using Error = std::variant<tokenizer::Error>;

template <typename Result>
[[nodiscard]] auto try_parse(int argc, char **argv) -> std::expected<Result, Error> {
    auto const s = std::span{argv, static_cast<std::size_t>(argc)};
    tokenizer::tokenize(s).and_then([](std::vector<tokenizer::Token> tokens) {
        // TODO: compile the tokes here into the target Result
        auto r = std::ranges::subrange(std::make_move_iterator(tokens.begin()),
                                       std::make_move_iterator(tokens.end()));
    });
    assert(false && "Not implemented yet");
}
}  // namespace args

#endif
