#include "include/tokenizer.hpp"
#include <optional>
#include <span>
#include <vector>

namespace args::tokenizer {

auto tokenize(std::span<char *>) -> std::optional<std::vector<Token>> { return {}; }
}  // namespace args::tokenizer
