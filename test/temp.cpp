#include "catch2/catch_test_macros.hpp"

#include "include/cpp_args.hpp"
#include "include/types.hpp"

TEST_CASE("Factorials are computed", "[factorial]") {  // NOLINT
    static constexpr auto option = args::Required<args::Flag{
        .long_form = args::Str{"an_option"},
    }>{};
    static constexpr auto rules = args::Rules<option>{};
    auto f = args::try_parse(0, nullptr, rules);
    (void)f;
}
