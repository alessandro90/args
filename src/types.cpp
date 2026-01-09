#include "include/types.hpp"
#include <algorithm>
#include <cstddef>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
constexpr auto max_descruption_len_bytes = 50uz;

auto apply_description(
    std::string &help,
    std::string_view description,
    std::size_t padding,
    std::size_t offset,
    bool is_first_iteration) -> void {
    if (description.empty()) {
        return;
    }
    if (description.size() <= max_descruption_len_bytes) {
        if (is_first_iteration) {
            help += std::string(padding - offset, padding_sep);
        } else {
            help += std::string(padding, padding_sep);
        }
        help += std::format("{}", description);
        return;
    }
    auto const space_index = description.find(' ', max_descruption_len_bytes);
    if (space_index == std::string_view::npos) {
        return;
    }
    if (is_first_iteration) {
        help += std::string(padding - offset, padding_sep);
    } else {
        help += std::string(padding, padding_sep);
    }
    help.append_range(description.substr(0, space_index));
    if (description.size() > space_index) {
        help += '\n';
        apply_description(help, description.substr(space_index + 1), padding, offset, false);
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
