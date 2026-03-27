#include "include/types.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if !defined _WIN32
    #include <sys/ioctl.h>
    #include <unistd.h>
#endif

namespace {
[[nodiscard]] auto count_digits(std::size_t n) -> std::size_t {
    auto count = 1uz;
    n = n / 10;  // NOLINT
    while (n != 0uz) {
        ++count;
        n = n / 10;  // NOLINT
    }
    return count;
}

constexpr auto padding_sep = ' ';
constexpr auto extra_padding = 8uz;
constexpr auto default_description_len_bytes = 80uz;

[[nodiscard]] auto get_terminal_columns() -> std::size_t {
#if !defined _WIN32
    winsize ws{};
    if (auto const success = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws); success == 0) {  // NOLINT
        return static_cast<std::size_t>(ws.ws_col);
    }
#endif

    if (auto const *ev = std::getenv("COLUMNS"); ev != nullptr) {  // NOLINT
        return std::stoul(std::string(ev));
    }
    return default_description_len_bytes;
}

struct [[nodiscard]] NewLineOpts {
    std::string_view s;
    std::size_t max_cols;
    std::size_t starting_index;
};

[[nodiscard]] auto find_newline_space(NewLineOpts opts) -> std::size_t {
    auto const [s, max_cols, starting_index] = opts;
    auto const space_index = s.rfind(' ', starting_index);
    if (space_index == std::string_view::npos) {
        return s.size();
    }
    if (space_index >= max_cols) {
        return starting_index;
    }
    return find_newline_space(
        NewLineOpts{.s = s, .max_cols = max_cols, .starting_index = space_index + 1uz});
}

auto apply_description(
    std::string &help,
    std::string_view description,
    std::size_t padding,
    std::size_t offset,
    bool is_first_iteration) -> void {
    static auto const terminal_cols = get_terminal_columns();
    if (terminal_cols <= padding || description.empty()) {
        return;
    }
    auto const max_description_cols = terminal_cols - padding;
    auto const add_left_padding = [&] {
        if (is_first_iteration) {
            help += std::string(padding - offset, padding_sep);
        } else {
            help += std::string(padding, padding_sep);
        }
    };
    if (description.size() <= max_description_cols) {
        add_left_padding();
        help += std::format("{}", description);
        return;
    }
    auto const space_index = find_newline_space(
        NewLineOpts{
            .s = description.substr(0, max_description_cols),
            .max_cols = max_description_cols,
            .starting_index = 0});
    if (space_index == std::string_view::npos) {
        return;
    }
    add_left_padding();
    help.append_range(description.substr(0, space_index));
    if (description.size() > space_index) {
        help += '\n';
        apply_description(help, description.substr(space_index), padding, offset, false);
    }
}
}  // namespace

namespace args::detail {
auto build_positionals_help(std::string &help, std::vector<PositionalHelp> &positional) -> void {
    auto const longest = std::ranges::max(
        positional | std::views::enumerate | std::views::transform([](auto const &item) {
            auto const &[index, help_data] = item;
            auto const name = help_data.name;
            if (name.empty()) {
                return count_digits(static_cast<std::size_t>(index + 2));  // 0 + 1 + '#'
            }
            return name.size();
        }));
    auto const padding = extra_padding + longest;

    for (auto const &[index, help_data] : std::views::enumerate(positional)) {
        auto const name = help_data.name;
        auto offset = 1uz;
        help += ' ';
        if (name.empty()) {
            auto arg_num = help_data.is_required ? std::format("#{}", index + 1)
                                                 : std::format("[#{}]", index + 1);
            offset = arg_num.size();
            help.append_range(std::move(arg_num));
        } else {
            if (help_data.is_required) {
                offset = name.size();
                help += name;
            } else {
                offset = name.size() + 2;
                help += std::format("[{}]", name);
            }
        }
        apply_description(help, help_data.description, padding, offset, true);
        help += '\n';
    }
}

auto build_flags_help(std::string &help, std::vector<FlagHelp> &flags) -> void {
    auto const longest =
        std::ranges::max(flags | std::views::transform([](FlagHelp const &help_data) {
                             return help_data.long_name.size();
                         }));
    auto const padding = (extra_padding * 2uz) + longest;

    for (auto const &help_data : flags) {
        auto const name = help_data.long_name;
        auto offset = 0uz;
        if (!help_data.is_required) {
            offset += 1;
            help += '[';
        } else {
            offset += 1;
            help += ' ';
        }
        if (help_data.short_name.has_value()) {
            help += std::format("-{}, ", help_data.short_name.value());
        } else {
            help += "  , ";
        }
        offset += 4;
        offset += name.size();
        help += "--";
        offset += 2;
        help += name;
        if (!help_data.is_required) {
            offset += 1;
            help += ']';
        }
        apply_description(help, help_data.description, padding, offset, true);
        help += '\n';
    }
}
}  // namespace args::detail
