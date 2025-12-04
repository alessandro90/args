#include "catch2/catch_test_macros.hpp"

#include "include/compiler.hpp"
#include "include/cpp_args.hpp"

TEST_CASE("Factorials are computed", "[factorial]") {  // NOLINT
    static constexpr auto option = args::compiler::Required<args::compiler::FlagSpec{
        .long_form = args::compiler::Str{"an_option"},
    }>{};
    static constexpr auto rules = args::compiler::Rules<option>{};
    auto f = args::try_parse(0, nullptr, rules);
    (void)f;
}
