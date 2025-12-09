#include "include/tokenizer.hpp"
#include <expected>
#include <span>
#include <vector>

namespace args::tokenizer {

auto tokenize(std::span<char *>) -> std::expected<std::vector<Token>, std::string> {
    return std::unexpected("not implemented yet");
}
}  // namespace args::tokenizer
