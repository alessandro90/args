#include "catch2/catch_test_macros.hpp"

#include "include/cpp_args.hpp"
#include "include/types.hpp"

TEST_CASE("Factorials are computed", "[factorial]") {  // NOLINT
    static constexpr auto option = args::Flag{.long_form = args::Str{"option_1"}, .required = true};
    static constexpr auto option_x = args::default_flag_with_value<int>(
        args::FlagWithValueArgs{.long_form = args::Str{"option_2"}});
    static constexpr auto option_y = args::FlagWithValue{
        .long_form = args::Str{"option_3"},
        .allow_missing_value = true,
        .value_if_not_specified = 10};
    static constexpr auto option_z = args::Positional<int>{};

    static constexpr auto rules = args::Rules<option, option_x, option_y, option_z>{};

    auto f = args::try_parse(0, nullptr, rules);
    auto const r = f.value().get<option_y>();
    (void)r;
}
