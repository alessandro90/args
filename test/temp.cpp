#include <vector>
#include "catch2/catch_test_macros.hpp"

#include "include/cpp_args.hpp"
#include "include/types.hpp"

using args::operator""_flag;

TEST_CASE("Factorials are computed", "[factorial]") {  // NOLINT
    static constexpr auto option = args::Flag{.long_form = "option_1"_flag, .required = true};
    static constexpr auto option_x =
        args::default_flag_with_value<int>(args::FlagWithValueArgs{.long_form = "option_2"_flag});
    static constexpr auto option_z = args::Positional<int>{};
    static constexpr auto option_vec =
        args::FlagWithValue{.long_form = "option_v"_flag, .default_value = []() {
                                return std::vector{1, 2, 3};
                            }};

    static constexpr auto rules = args::Rules<option, option_x, option_z>{};

    auto f = args::try_parse(0, nullptr, rules);
    auto const r = f.value().get<option_z>();
    (void)r;
}
