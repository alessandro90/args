#ifndef CPP_ARGS_TOKENIZER_HEADER
#define CPP_ARGS_TOKENIZER_HEADER

#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// TODO: add 'has_equal' to long flag, short flag and flag group.
// is_equal implies for the compiler to assume the flag is a flag with value
// otherwise it tries to match the arguments based on the rules

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

using token_t = std::variant<ShortFlag, LongFlag, FlagGroup, Argument>;

[[nodiscard]] auto tokenize(std::span<char const *> src)
    -> std::expected<std::vector<token_t>, std::string>;

}  // namespace args::tokenizer

#endif
