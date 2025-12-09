#ifndef CPP_ARGS_TOKENIZER_HEADER
#define CPP_ARGS_TOKENIZER_HEADER

#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace args::tokenizer {
struct [[nodiscard]] Argument {
    std::string_view value;
};

struct [[nodiscard]] ShortFlag {
    char flag;
};

struct [[nodiscard]] LongFlag {
    std::string_view flag;
};

struct [[nodiscard]] FlagGroup {
    std::string_view group;
};

using Token = std::variant<ShortFlag, LongFlag, FlagGroup, Argument>;

[[nodiscard]] auto tokenize(std::span<char *> src)
    -> std::expected<std::vector<Token>, std::string>;

}  // namespace args::tokenizer

#endif
