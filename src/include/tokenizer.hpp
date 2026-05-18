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

    [[nodiscard]] constexpr auto operator==(Argument const &) const -> bool = default;
};

struct [[nodiscard]] ShortFlag {
    std::string_view raw;
    char flag{};
    bool has_equal{};

    [[nodiscard]] constexpr auto operator==(ShortFlag const &) const -> bool = default;
};

struct [[nodiscard]] LongFlag {
    std::string_view raw;
    std::string_view flag;
    bool has_equal{};

    [[nodiscard]] constexpr auto operator==(LongFlag const &) const -> bool = default;
};

struct [[nodiscard]] FlagGroup {
    std::string_view raw;
    std::string_view group;
    bool has_equal{};

    [[nodiscard]] constexpr auto operator==(FlagGroup const &) const -> bool = default;
};

struct [[nodiscard]] DoubleDash {
    [[nodiscard]] constexpr auto operator==(DoubleDash const &) const -> bool {
        return true;
    }
};

using token_t = std::variant<ShortFlag, LongFlag, FlagGroup, Argument, DoubleDash>;

[[nodiscard]] auto tokenize(std::span<char const *> src)
    -> std::expected<std::vector<token_t>, std::string>;

}  // namespace args::tokenizer

#endif
