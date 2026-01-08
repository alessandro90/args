#include "include/types.hpp"
#include <algorithm>
#include <cstddef>
#include <format>
#include <ranges>
#include <string>
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
    auto const padding = 8uz + longest;

    for (auto const &[index, help_data] : std::views::enumerate(positional)) {
        auto const name = help_data.name;
        auto offset = 0uz;
        if (name.empty()) {
            auto arg_num = [&] {
                if (help_data.is_required) {
                    return std::format("#{}", index + 1);
                }
                return std::format("[#{}]", index + 1);
            }();
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
        if (!help_data.description.empty()) {
            help += std::string(padding - offset, padding_sep);
            help += std::format("{}", help_data.description);
        }
    }
}

auto build_flags_help(std::string &help, std::vector<PureFlagHelp> &flags) -> void {}

auto build_flags_with_value_help(std::string &help, std::vector<FlagWithValueHelp> &flags) -> void {
}
}  // namespace args::detail
