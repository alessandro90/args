#ifndef ARGS_COLORS
#define ARGS_COLORS

#include <format>
#include <string>
#include <utility>
#include "helpers.hpp"
#if !defined _WIN32
    #include <cstdlib>
    #include <unistd.h>
#endif

namespace args::color {
template <std::size_t N>
struct Color {
    args::Str<N> value;
};

constexpr auto reset = Str{"\033[0m"};
constexpr auto cred = Color{Str{"\033[31m"}};
constexpr auto cgreen = Color{Str{"\033[32m"}};
constexpr auto cyellow = Color{Str{"\033[33m"}};
constexpr auto ccyan = Color{Str{"\033[36m"}};
constexpr auto bold = Color{Str{"\033[1m"}};

namespace detail {
#if !defined _WIN32
[[nodiscard]] inline auto terminal_has_colors() -> bool {
    if (std::getenv("NO_COLOR") != nullptr) {  // NOLINT
        return false;
    }

    auto const term = std::string_view{std::getenv("TERM")};  // NOLINT
    if (term.empty()) {
        return false;
    }

    if (term == "dumb") {
        return false;
    }

    // Common terminal types that natively handle ANSI codes
    return term.contains("color") || term.contains("xterm") || term.contains("linux")
           || term.contains("screen");
}

[[nodiscard]] inline auto should_use_color() -> bool {
    return isatty(STDOUT_FILENO) == 1 && terminal_has_colors();
}
#else
[[nodiscard]] inline auto should_use_color() -> bool {
    return false;
}
#endif
}  // namespace detail

template <Color C, typename... Args>
[[nodiscard]] auto color_format(std::format_string<Args...> fmt, Args &&...args) -> std::string {
    if (detail::should_use_color()) {
        return C.value.as_string_view() + std::format(fmt, std::forward<Args>(args)...)
               + reset.as_string_view();
    }
    return std::format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
[[nodiscard]] auto yellow(std::format_string<Args...> fmt, Args &&...args) -> std::string {
    return color_format<cyellow>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
[[nodiscard]] auto cyan(std::format_string<Args...> fmt, Args &&...args) -> std::string {
    return color_format<ccyan>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
[[nodiscard]] auto green(std::format_string<Args...> fmt, Args &&...args) -> std::string {
    return color_format<cgreen>(fmt, std::forward<Args>(args)...);
}
}  // namespace args::color

#endif
