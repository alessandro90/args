#ifndef CPP_ARGS_COMPILER_HEADER
#define CPP_ARGS_COMPILER_HEADER

#include <cstddef>
#include <optional>
#include <string_view>
#include <tuple>

namespace args::compiler {

// TODO: a compiler could take a the tokens and a set of user defined rules and produce a parsed
// struct containing all the provided arguments, correctly typed
// compile could take a struct builder as input. The struct could expose methods to handle the
// tokens?
// check at compile time (concepts) if the builder can handle the arguments, otherwise fail
// handle_flag(Flag)
// handle_positional(pos, arg)
// handle_named(name, value)
//
// Another idea is to build a set of rules in some way and having the compiler assert the stream
// of tokens abide by such rules

struct FlagSpec {
    std::string_view long_form;
    std::optional<char> short_form;
    static constexpr bool is_spec = true;
};

template <typename V>
struct FlagWithValue {
    std::string_view long_form;
    std::optional<char> short_form;
    V value;
    static constexpr bool is_spec = true;
};

template <typename P>
struct Positional {
    P value;
    std::optional<P> default_value;
    std::size_t index;
    static constexpr bool is_spec = true;
};

template <typename S>
concept Spec = S::is_spec;

template <Spec... Specs>
struct Rules {
    std::tuple<Specs...> specs;
    // todo: find_by_long_form, find_by_short_form etc...
};

// to be implemented by the user
// must be def. constructible
// define a trait for better user experience
struct Result {
    // template not mandatory, just define enough overloads to work
    template <typename P>
    void set_positional(std::size_t index, P value);

    void set_short_flag(char);
    void set_long_flag(std::string_view);

    // template not mandatory, just define enough overloads to work
    template <typename V>
    void set_short_flag_with_value(char, V);

    // template not mandatory, just define enough overloads to work
    template <typename V>
    void set_long_flag_with_value(std::string_view);
};

}  // namespace args::compiler

#endif
