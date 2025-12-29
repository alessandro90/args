#include "include/tokenizer.hpp"
#include <expected>
#include <span>
#include <vector>

namespace args::tokenizer {

auto tokenize(std::span<char const *>) -> std::expected<std::vector<token_t>, std::string> {
    return std::unexpected("not implemented yet");
}
}  // namespace args::tokenizer
