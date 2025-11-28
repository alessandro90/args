#ifndef CPP_ARGS_TOKENIZER_HEADER
#define CPP_ARGS_TOKENIZER_HEADER

#include <cstddef>
#include <expected>
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

struct [[nodiscard]] ShortFlagWithValue {
    char flag;
    std::string_view value;
};

struct [[nodiscard]] LongFlagWithValue {
    std::string_view flag;
    std::string_view value;
};

struct [[nodiscard]] LongFlag {
    std::string_view flag;
};

struct [[nodiscard]] FlagGroup {
    std::string_view group;
};

using Token =
    std::variant<ShortFlag, ShortFlagWithValue, LongFlag, LongFlagWithValue, FlagGroup, Argument>;

struct [[nodiscard]] Error {
    constexpr Error(std::size_t nr, std::string_view s)
        : argv_nr{nr}
        , chars{s} {}

    std::size_t argv_nr;
    std::string_view chars;
};

[[nodiscard]] auto tokenize(std::span<char *> src) -> std::expected<std::vector<Token>, Error>;

}  // namespace args::tokenizer

#endif
