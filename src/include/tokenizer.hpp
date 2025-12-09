#ifndef CPP_ARGS_TOKENIZER_HEADER
#define CPP_ARGS_TOKENIZER_HEADER

#include <cstddef>
#include <optional>
#include <span>
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

struct [[nodiscard]] Error {
    std::size_t argv_nr;
    std::string_view chars;
};

// [[nodiscard]] auto tokenize(std::span<char *> src) -> std::expected<std::vector<Token>, Error>;
[[nodiscard]] auto tokenize(std::span<char *> src) -> std::optional<std::vector<Token>>;

}  // namespace args::tokenizer

#endif
