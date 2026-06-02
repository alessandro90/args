#ifndef ARGS_TYPES
#define ARGS_TYPES

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include "helpers.hpp"
#include "typetag.hpp"
#include "validators.hpp"

namespace args {

template <typename T, auto... Args>
inline constexpr auto Lazy = [] {
    return T{std::move(Args)...};
};

template <typename T, auto... Args>
using lazy_t = decltype(Lazy<T, Args...>);

template <typename T>
using vec_t = lazy_t<std::vector<T>>;

template <typename T>
struct [[nodiscard]] Opt {
    bool has_value;
    T value;

    static consteval auto empty() -> Opt {
        return Opt{.has_value = false, .value = T{}};
    }

    static consteval auto with(T value) -> Opt {
        return Opt{.has_value = true, .value = value};
    }

    [[nodiscard]] constexpr auto operator==(Opt const &) const -> bool = default;
};

template <typename T>
concept Trivial = std::is_trivial_v<T>;

/// A compile time string_view-like object
///
/// Should be used to define string-like quantities needed at compile time
///
/// Note: it can be compared to std::string_view and std::string, and therefore
/// it can be used in validators that deal with those types
///
/// Usage:
///
/// `"a compile-time string-like object"_str`
template <std::size_t N>
struct [[nodiscard]] Str {
    static constexpr auto s_size = N;

    std::array<char, N + 1> chars{};

    consteval Str() noexcept = default;

    consteval Str(char const (&s)[N + 1]) {  // NOLINT
        std::ranges::copy(s, chars.begin());
    }

    [[nodiscard]] constexpr auto as_string_view() const -> std::string_view {
        return std::string_view{chars.data()};
    }

    [[nodiscard]] static constexpr auto is_empty() noexcept -> bool {
        return N == 0;
    }

    template <std::size_t M>
    [[nodiscard]] constexpr auto operator==(Str<M> const &rhs) -> bool {
        return N == M && chars == rhs.chars;
    }

    [[nodiscard]] constexpr auto operator==(std::string_view rhs) -> bool {
        return as_string_view() == rhs;
    }

    [[nodiscard]] constexpr auto operator==(std::string const &rhs) -> bool {
        return as_string_view() == rhs;
    }
};

template <std::size_t N>
Str(char const (&s)[N]) -> Str<N - 1>;  // NOLINT

/// An empty `Str` object. Useful to avoid empty string creation
inline constexpr auto empty = Str{""};

using str_t = lazy_t<std::string>;

using strv_t = lazy_t<std::string_view>;

namespace detail {
constexpr auto help_str = std::string_view{"help"};

template <auto S>
concept HasValidator = requires { S._validator; };

template <auto S>
concept HasDefault = requires { S._default_value; };

template <auto S>
consteval auto is_required() -> bool {
    if constexpr (requires { S._required; }) {
        return S._required;
    } else {
        return false;
    }
}

template <auto S>
concept IsRequired = is_required<S>();

template <typename T>
consteval auto result_type() -> std::remove_cvref_t<T>;

template <std::invocable T>
consteval auto result_type() -> std::remove_cvref_t<decltype(std::declval<T>()())>;

template <typename T>
using result_type_impl_t = decltype(result_type<T>());

template <auto S>
using result_type_t = result_type_impl_t<typename decltype(S)::value_t>;

template <typename T>
consteval auto tag_to_default_type() {
    if constexpr (Trivial<T>) {
        return T{};
    } else {
        return [] {
            return T{};
        };
    }
}

template <typename T>
using tag_to_default_type_t = decltype(tag_to_default_type<T>());

}  // namespace detail

/// A boolean flag descriptor
template <std::size_t N, std::size_t M = 0>
struct [[nodiscard]] Flag {
    /// The long form of the flag (e.g. `"verbose"_str` will parse `--verbose`)
    Str<N> _long_form;
    /// The short form, a `char` (e.g. `"j"_str` will parse `-j`)
    Opt<char> _short_form{Opt<char>::empty()};
    /// The default value if no flag is parsed (defaults to `false`)
    bool _default_value{};
    /// `true` if the flag is required (defaults to `false`)
    bool _required{};
    /// An optional help message
    Str<M> _help{};

    using value_t = bool;

    template <std::size_t Nx>
    consteval auto Long(char const (&long_form)[Nx]) const -> Flag<Nx - 1, M> {
        return Flag<Nx - 1, M>{
            ._long_form = Str<Nx - 1>{long_form},
            ._short_form = _short_form,
            ._default_value = _default_value,
            ._required = _required,
            ._help = _help,
        };
    }

    template <std::size_t Mx>
    consteval auto Help(char const (&help)[Mx]) const -> Flag<N, Mx - 1> {
        return Flag<N, Mx - 1>{
            ._long_form = _long_form,
            ._short_form = _short_form,
            ._default_value = _default_value,
            ._required = _required,
            ._help = Str<Mx - 1>{help},
        };
    }

    consteval auto Short(char short_form) const -> Flag<N, M> {
        return Flag<N, M>{
            ._long_form = _long_form,
            ._short_form = Opt<char>::with(short_form),
            ._default_value = _default_value,
            ._required = _required,
            ._help = _help,
        };
    }

    consteval auto Default(bool default_value) const -> Flag<N, M> {
        return Flag<N, M>{
            ._long_form = _long_form,
            ._short_form = _short_form,
            ._default_value = default_value,
            ._required = _required,
            ._help = _help,
        };
    }

    consteval auto Required(bool required) const -> Flag<N, M> {
        return Flag<N, M>{
            ._long_form = _long_form,
            ._short_form = _short_form,
            ._default_value = _default_value,
            ._required = required,
            ._help = _help,
        };
    }
};

consteval auto flag() -> Flag<0, 0> {
    return Flag<0, 0>{};
}

/// A flag with value descriptor
template <
    typename Tag,
    std::size_t N,
    std::size_t M = 0,
    ValidatorObject V = always_t,
    typename DefaultType = detail::tag_to_default_type_t<Tag>>
struct [[nodiscard]] FlagWithValue {
    /// The long form of the flag (e.g. `"verbose"_str` will parse `--verbose`)
    Str<N> _long_form;
    /// The short form, a `char` (e.g. `"j"_str` will parse `-j`)
    Opt<char> _short_form{Opt<char>::empty()};
    /// The default value if no flag is parsed (defaults to a default constructed `Value`)
    DefaultType _default_value{};
    /// `true` if the flag is required (defaults to `false`)
    bool _required{};
    /// `true` if the flag can be specified multiple times. Default is true if `Value` is a
    /// std::vector
    bool _repeatable{args::detail::is_vector_v<Tag>};
    /// An optional help message
    Str<M> _help{};
    /// A validator to apply to the parsed result (defaults to an infallible validator)
    V _validator{always};

    using value_t = detail::tag_to_default_type_t<Tag>;

    template <std::size_t Nx>
    consteval auto Long(char const (&long_form)[Nx]) const
        -> FlagWithValue<Tag, Nx - 1, M, V, DefaultType> {
        return FlagWithValue<Tag, Nx - 1, M, V, DefaultType>{
            ._long_form = Str<Nx - 1>{long_form},
            ._short_form = _short_form,
            ._default_value = _default_value,
            ._required = _required,
            ._repeatable = _repeatable,
            ._help = _help,
            ._validator = _validator,
        };
    }

    template <std::size_t Mx>
    consteval auto Help(char const (&help)[Mx]) const
        -> FlagWithValue<Tag, N, Mx - 1, V, DefaultType> {
        return FlagWithValue<Tag, N, Mx - 1, V, DefaultType>{
            ._long_form = _long_form,
            ._short_form = _short_form,
            ._default_value = _default_value,
            ._required = _required,
            ._repeatable = _repeatable,
            ._help = help,
            ._validator = _validator,
        };
    }

    consteval auto Short(char short_form) const -> FlagWithValue<Tag, N, M, V, DefaultType> {
        return FlagWithValue<Tag, N, M, V, DefaultType>{
            ._long_form = _long_form,
            ._short_form = Opt<char>::with(short_form),
            ._default_value = _default_value,
            ._required = _required,
            ._repeatable = _repeatable,
            ._help = _help,
            ._validator = _validator,
        };
    }

    template <Trivial D>
    consteval auto Default(D default_value) const
        -> FlagWithValue<detail::result_type_impl_t<D>, N, M, V, D>
        requires std::same_as<detail::result_type_impl_t<D>, Tag>
    {
        return FlagWithValue<detail::result_type_impl_t<D>, N, M, V, D>{
            ._long_form = _long_form,
            ._short_form = _short_form,
            ._default_value = default_value,
            ._required = _required,
            ._repeatable = _repeatable,
            ._help = _help,
            ._validator = _validator,
        };
    }

    consteval auto Required(bool required) const -> FlagWithValue<Tag, N, M, V, DefaultType> {
        return FlagWithValue<Tag, N, M, V, DefaultType>{
            ._long_form = _long_form,
            ._short_form = _short_form,
            ._default_value = _default_value,
            ._required = required,
            ._repeatable = _repeatable,
            ._help = _help,
            ._validator = _validator,
        };
    }

    consteval auto Repeatable(bool repeatable) const -> FlagWithValue<Tag, N, M, V, DefaultType> {
        return FlagWithValue<Tag, N, M, V, DefaultType>{
            ._long_form = _long_form,
            ._short_form = _short_form,
            ._default_value = _default_value,
            ._required = _required,
            ._repeatable = repeatable,
            ._help = _help,
            ._validator = _validator,
        };
    }

    template <ValidatorObject Vx>
    consteval auto Validator(Vx validator) const -> FlagWithValue<Tag, N, M, Vx, DefaultType> {
        return FlagWithValue<Tag, N, M, Vx, DefaultType>{
            ._long_form = _long_form,
            ._short_form = _short_form,
            ._default_value = _default_value,
            ._required = _required,
            ._repeatable = _repeatable,
            ._help = _help,
            ._validator = validator,
        };
    }
};

template <typename T>
consteval auto flag_with_value() -> FlagWithValue<T, 0, 0, always_t> {
    return FlagWithValue<T, 0, 0, always_t>{};
}

/// A positional value descriptor
template <
    std::default_initializable P,
    std::size_t N = 0,
    std::size_t M = 0,
    ValidatorObject V = always_t>
struct [[nodiscard]] Positional {
    /// A tag to indicate the target type (specify as `tag<target_type>`)
    Typetag<P> _type;
    /// Optional name to be displayed int the help message
    Str<N> _name{};
    /// Optional help message
    Str<M> _help{};
    /// `true` if the flag is required (defaults to `false`)
    bool _required{};
    /// If `true` the parsed value must be a `std::vector`. Successive values will be stored into
    /// the vecotor, e.g. `value_1 value_2 value_3` will be parsed into a unique vector of
    /// appropriately parsed values
    bool _variadic{};
    /// A validator to apply to the parsed result (defaults to an infallible validator)
    V _validator{always};

    using value_t = P;

    template <std::size_t Nx>
    consteval auto Name(char const (&name)[Nx]) const -> Positional<P, Nx - 1, M, V> {
        return Positional<P, Nx - 1, M, V>{
            ._type = _type,
            ._name = Str<Nx - 1>{name},
            ._help = _help,
            ._required = _required,
            ._variadic = _variadic,
            ._validator = _validator,
        };
    }

    template <std::size_t Mx>
    consteval auto Help(char const (&help)[Mx]) const -> Positional<P, N, Mx - 1, V> {
        return Positional<P, N, Mx - 1, V>{
            ._type = _type,
            ._name = _name,
            ._help = Str<Mx - 1>{help},
            ._required = _required,
            ._variadic = _variadic,
            ._validator = _validator,
        };
    }

    consteval auto Required(bool required) const -> Positional<P, N, M, V> {
        return Positional<P, N, M, V>{
            ._type = _type,
            ._name = _name,
            ._help = _help,
            ._required = required,
            ._variadic = _variadic,
            ._validator = _validator,
        };
    }

    consteval auto Variadic(bool variadic) const -> Positional<P, N, M, V> {
        return Positional<P, N, M, V>{
            ._type = _type,
            ._name = _name,
            ._help = _help,
            ._required = _required,
            ._variadic = variadic,
            ._validator = _validator,
        };
    }

    template <ValidatorObject Vx>
    consteval auto Validator(Vx validator) const -> Positional<P, N, M, Vx> {
        return Positional<P, N, M, Vx>{
            ._type = _type,
            ._name = _name,
            ._help = _help,
            ._required = _required,
            ._variadic = _variadic,
            ._validator = validator,
        };
    }
};

template <typename T>
consteval auto positional() -> Positional<T, 0, 0, always_t> {
    return Positional<T, 0, 0, always_t>{};
}

template <Str Usage, Str Description, auto... Ops>
struct [[nodiscard]] Options;

template <auto... Ss>
struct [[nodiscard]] MutuallyExclusive {
    static_assert(sizeof...(Ss) > 1, "A mutually exclusive set must have at least 2 members");
    bool at_least_one{};
};

template <auto... Ss>
struct MutuallyExclusiveGroups {};

namespace detail {
template <typename>
struct IsMutuallyExclusiveGroup: std::false_type {};

template <auto... Gg>
struct IsMutuallyExclusiveGroup<MutuallyExclusiveGroups<Gg...>>: std::true_type {};
}  // namespace detail

/// A subcommand descriptor
///
/// A subcommand can only be the first argument of a set of options
template <
    std::size_t N,
    std::size_t M = 0,
    Str Usage = empty,
    Str Description = empty,
    typename Me = MutuallyExclusiveGroups<>,
    auto... Ops>
struct [[nodiscard]] Subcommand {
    /// The name to parse
    Str<N> _name{};
    /// An optional help message
    Str<M> _help{};
    /// The set of options (arguments) for this subcommand
    Options<Usage, Description, Ops...> _options{};
    /// `true` if this subcommand is invoked as a long flag
    bool _is_flag{};
    /// Arbitrary mutually exclusive groups
    Me _mutually_exclusive{};

    static_assert(
        detail::IsMutuallyExclusiveGroup<Me>::value,
        "This type can only be a MutuallyExclusiveGroups type");

    // using value_t = strv_t;
    using value_t = std::string_view;

    template <std::size_t Nx>
    consteval auto Name(char const (&name)[Nx]) const
        -> Subcommand<Nx - 1, M, Usage, Description, Me, Ops...> {
        return Subcommand<Nx - 1, M, Usage, Description, Me, Ops...>{
            ._name = Str<Nx - 1>{name},
            ._help = _help,
            ._options = _options,
            ._is_flag = _is_flag,
            ._mutually_exclusive = _mutually_exclusive,
        };
    }

    template <std::size_t Mx>
    consteval auto Help(char const (&help)[Mx]) const
        -> Subcommand<N, Mx - 1, Usage, Description, Me, Ops...> {
        return Subcommand<N, Mx - 1, Usage, Description, Me, Ops...>{
            ._name = _name,
            ._help = Str<Mx - 1>{help},
            ._options = _options,
            ._is_flag = _is_flag,
            ._mutually_exclusive = _mutually_exclusive,
        };
    }

    consteval auto Flag(bool is_flag) const -> Subcommand<N, M, Usage, Description, Me, Ops...> {
        return Subcommand<N, M, Usage, Description, Me, Ops...>{
            ._name = _name,
            ._help = _help,
            ._options = _options,
            ._is_flag = is_flag,
            ._mutually_exclusive = _mutually_exclusive,
        };
    }

    template <typename NewMe>
    consteval auto MutuallyExclusive(NewMe groups) const
        -> Subcommand<N, M, Usage, Description, NewMe, Ops...> {
        return Subcommand<N, M, Usage, Description, NewMe, Ops...>{
            ._name = _name,
            ._help = _help,
            ._options = _options,
            ._is_flag = _is_flag,
            ._mutually_exclusive = groups,
        };
    }

    /// Update the options layout, altering the structural type properties of the Subcommand
    template <Str NewUsage, Str NewDescription, auto... NewSpecs>
    consteval auto Opts(Options<NewUsage, NewDescription, NewSpecs...> opts) const
        -> Subcommand<N, M, NewUsage, NewDescription, Me, NewSpecs...> {
        return Subcommand<N, M, NewUsage, NewDescription, Me, NewSpecs...>{
            ._name = _name,
            ._help = _help,
            ._options = opts,
            ._is_flag = _is_flag,
            ._mutually_exclusive = _mutually_exclusive,
        };
    }
};

template <typename>
struct IsFlag: std::false_type {};

template <std::size_t N, std::size_t M>
struct IsFlag<Flag<N, M>>: std::true_type {};

template <typename T>
inline constexpr bool is_flag_v = IsFlag<std::remove_cvref_t<T>>::value;

template <typename>
struct IsFlagWithValue: std::false_type {};

template <typename Tag, std::size_t N, std::size_t M, ValidatorObject V, typename DefaultType>
struct IsFlagWithValue<FlagWithValue<Tag, N, M, V, DefaultType>>: std::true_type {};

template <typename P>
struct IsPositional: std::false_type {};

template <typename P, std::size_t N, std::size_t M, ValidatorObject V>
struct IsPositional<Positional<P, N, M, V>>: std::true_type {};

template <typename P>
inline constexpr bool is_positional_v = IsPositional<std::remove_cvref_t<P>>::value;

template <typename T>
inline constexpr bool is_flag_with_value_v = IsFlagWithValue<std::remove_cvref_t<T>>::value;

template <typename>
struct IsSubcommand: std::false_type {};

template <std::size_t N, std::size_t M, Str Usage, Str Description, typename Me, auto... Ops>
struct IsSubcommand<Subcommand<N, M, Usage, Description, Me, Ops...>>: std::true_type {};

template <typename S>
inline constexpr bool is_subcommand_v = IsSubcommand<std::remove_cvref_t<S>>::value;

namespace detail {


template <typename P>
[[nodiscard]] consteval auto is_positional_variadic(P p) -> bool {
    if constexpr (IsPositional<P>::value) {
        return p._variadic;
    } else {
        return false;
    }
}

template <auto S>
[[nodiscard]] consteval auto is_repeatable() -> bool {
    if constexpr (requires { S._repeatable; }) {
        return S._repeatable;
    } else {
        return false;
    }
}

template <auto S>
inline constexpr auto is_repeatable_v = is_repeatable<S>();

template <auto S>
inline constexpr auto is_positional_variadic_v = is_positional_variadic(S);

template <auto S>
concept PositionalVariadic = is_positional_variadic_v<S>;

template <auto S>
concept FlagObject = is_flag_v<decltype(S)> || is_flag_with_value_v<decltype(S)>;

template <typename T>
using RepeatableParseType = std::variant<T, std::vector<T>>;

template <typename T>
auto get_repeatable_single_type(RepeatableParseType<T> const &) -> T;

template <typename T>
using repeatable_single_type_t = decltype(get_repeatable_single_type(std::declval<T>()));

template <typename T>
struct IsRepeatableParseType: std::false_type {};

template <typename T>
struct IsRepeatableParseType<RepeatableParseType<T>>: std::true_type {};

template <auto S>
consteval auto parse_type() -> result_type_t<S>;

template <auto S>
requires PositionalVariadic<S>
consteval auto parse_type() ->
    typename result_type_t<S>::value_type;  // this is a vector, so we can get its contained type

template <auto S>
requires is_repeatable_v<S>
consteval auto parse_type()
    -> RepeatableParseType<typename result_type_impl_t<typename decltype(S)::value_t>::value_type>;

template <auto S>
using parse_type_t = decltype(parse_type<S>());

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto check_variadics() -> bool {
    if constexpr (is_positional_variadic_v<S1>) {
        return is_vector_v<result_type_t<S1>>;
    }
    if constexpr (sizeof...(Ss) > 0) {
        return check_variadics<Ss...>();
    } else {
        return true;
    }
}

namespace rule_assertions {
template <auto S, auto... Ss>
[[nodiscard]] consteval auto assert_valid_defaults() -> bool {
    if constexpr (HasValidator<S> && HasDefault<S> && Not<IsRequired<S>>) {
        if constexpr (!std::is_invocable_v<decltype(S._default_value)>) {
            if (!S._validator.fn(S._default_value)) {
                return false;
            }
        } else {
            if (!S._validator.fn(S._default_value())) {
                return false;
            }
        }
    }
    if constexpr (sizeof...(Ss) == 0) {
        return true;
    } else {
        return assert_valid_defaults<Ss...>();
    }
}

template <auto S>
[[nodiscard]] consteval auto need_unique_name() -> bool {
    return is_subcommand_v<decltype(S)> || FlagObject<S>;
}

template <auto S>
[[nodiscard]] consteval auto get_unique_name() -> std::string_view {
    if constexpr (is_subcommand_v<decltype(S)>) {
        return S._name.as_string_view();
    } else if constexpr (FlagObject<S>) {
        return S._long_form.as_string_view();
    } else {
        static_assert(false, "Invalid argument");
    }
}

template <auto S1, auto S2>
[[nodiscard]] consteval auto have_different_flag_names() -> bool {
    if constexpr (!need_unique_name<S1>() || !need_unique_name<S2>()) {
        return true;
    } else {
        if (get_unique_name<S1>() == get_unique_name<S2>()) {
            return false;
        }
        if constexpr (FlagObject<S1> && FlagObject<S2>) {
            return !S1._short_form.has_value || !S2._short_form.has_value
                   || S1._short_form != S2._short_form;
        } else {
            return true;
        }
    }
}

template <auto S1, auto S2, auto... Ss>
[[nodiscard]] consteval auto check_all_different_names_impl() -> bool {
    bool const s1_not_s2 = have_different_flag_names<S1, S2>();
    if constexpr (sizeof...(Ss) == 0) {
        return s1_not_s2;
    } else {
        return s1_not_s2 && check_all_different_names_impl<S1, Ss...>();
    }
}

template <auto S, auto... Ss>
[[nodiscard]] consteval auto check_all_different_names() -> bool {
    if constexpr (sizeof...(Ss) == 0) {
        return true;
    } else {
        return check_all_different_names_impl<S, Ss...>() && check_all_different_names<Ss...>();
    }
}

[[nodiscard]] constexpr auto is_valid_first_char(char c) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
};

[[nodiscard]] constexpr auto is_valid_non_first_char(char c) -> bool {
    return (is_valid_first_char(c) || (c >= '0' && c <= '9')) || c == '-' || c == '_';
};

[[nodiscard]] consteval auto is_valid_name(std::string_view name) -> bool {
    if (name.empty()) {
        return false;
    }
    if (!is_valid_first_char(name[0])) {
        return false;
    }
    if (name.size() == 1) {
        return true;
    }
    return std::ranges::all_of(name | std::views::drop(1), is_valid_non_first_char);
}

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto check_valid_names() -> bool {
    if constexpr (is_subcommand_v<decltype(S1)>) {
        if (!is_valid_name(S1._name.as_string_view())) {
            return false;
        }
    }
    if constexpr (FlagObject<S1>) {
        if (!is_valid_name(S1._long_form.as_string_view())) {
            return false;
        }
        if (!S1._short_form.has_value) {
            return true;
        }
        if (!is_valid_first_char(S1._short_form.value)) {
            return false;
        }
    }
    if constexpr (sizeof...(Ss) > 0) {
        return check_valid_names<Ss...>();
    } else {
        return true;
    }
}

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto check_help_reserved() -> bool {
    if constexpr (is_subcommand_v<decltype(S1)>) {
        if (S1._name.as_string_view() == help_str) {
            return false;
        }
    }
    if constexpr (FlagObject<S1>) {
        if constexpr (S1._long_form.as_string_view() == help_str) {
            return false;
        }
    }
    if constexpr (sizeof...(Ss) > 0) {
        return check_help_reserved<Ss...>();
    } else {
        return true;
    }
}

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto count_variadics() -> std::size_t {
    if constexpr (is_positional_v<decltype(S1)>) {
        if constexpr (sizeof...(Ss) > 0) {
            return static_cast<std::size_t>(S1._variadic) + check_variadics<Ss...>();
        } else {
            return 0;
        }
    } else {
        if constexpr (sizeof...(Ss) > 0) {
            return check_variadics<Ss...>();
        } else {
            return 0;
        }
    }
}

template <auto S1, auto... Ss>
[[nodiscard]] consteval auto check_variadic_is_last_positional_rec(bool found_variadic) -> bool {
    if constexpr (is_positional_variadic_v<S1>) {
        if constexpr (sizeof...(Ss) > 0) {
            return !found_variadic || check_variadic_is_last_positional_rec<Ss...>(true);
        } else {
            return !found_variadic;
        }
    }
    if constexpr (is_positional_v<decltype(S1)>) {
        if (found_variadic) {
            return false;
        }
    }
    if constexpr (sizeof...(Ss) > 0) {
        return check_variadic_is_last_positional_rec<Ss...>(found_variadic);
    } else {
        return true;
    }
}

template <auto... Ss>
[[nodiscard]] consteval auto check_variadic_is_last_positional() -> bool {
    return check_variadic_is_last_positional_rec<Ss...>(false);
}

template <auto S>
[[nodiscard]] consteval auto check_repeatable_is_vector_value() -> bool {
    if constexpr (is_flag_with_value_v<decltype(S)>) {
        if constexpr (S._repeatable) {
            return detail::is_vector_v<result_type_impl_t<typename decltype(S)::value_t>>;
        }
    }
    return true;
}

template <auto S, auto... Ss>
[[nodiscard]] consteval auto check_repeatable_is_vector() -> bool {
    if constexpr (sizeof...(Ss) == 0) {
        return check_repeatable_is_vector_value<S>();
    } else {
        return check_repeatable_is_vector_value<S>() && check_repeatable_is_vector<Ss...>();
    }
}

template <auto... Ops>
struct CheckRules {
    static_assert(check_all_different_names<Ops...>(), "All flags must have unique identifiers");
    static_assert(
        check_valid_names<Ops...>(),
        "All flags must begin with a letter, both long and short forms");
    static_assert(check_help_reserved<Ops...>(), "'help' is a reserved flag name");
    static_assert(detail::check_variadics<Ops...>(), "Variadics positionals must be vector<T>");
    static_assert(
        count_variadics<Ops...>() <= 1, "You can set at most 1 variadic positional argument");
    static_assert(
        check_variadic_is_last_positional<Ops...>(),
        "Positional variadic argument must be the last positional argument because it consumes all "
        "positionals");
    static_assert(assert_valid_defaults<Ops...>(), "Invalid default for specification");
    static_assert(check_repeatable_is_vector<Ops...>(), "Only vector flags can be made repeatable");
};

template <>
struct CheckRules<> {};
}  // namespace rule_assertions

template <auto S>
[[nodiscard]] auto default_arg_value() {
    if constexpr (std::is_invocable_v<typename decltype(S)::value_t>) {
        return result_type_t<S>{};
    } else if constexpr (requires { S._default_value; }) {
        return S._default_value;
    } else {
        // this is the case for positional arguments.
        return result_type_t<S>{};
    }
}

struct [[nodiscard]] PositionalHelp {
    std::string_view name{};
    std::string_view description{};
    bool is_required{};
};

struct [[nodiscard]] FlagHelp {
    std::optional<char> short_name{};
    std::string_view long_name{};
    std::string_view description{};
    bool is_required{};
};

template <auto S>
auto build_help_data(
    std::vector<PositionalHelp> &positional,
    std::vector<FlagHelp> &pure_flags,  // NOLINT(bugprone-easily-swappable-parameters)
    std::vector<FlagHelp> &flags_with_value) -> void {
    using s_t = decltype(S);
    if constexpr (is_positional_v<s_t>) {
        positional.push_back(
            PositionalHelp{
                .name = S._name.as_string_view(),
                .description = S._help.as_string_view(),
                .is_required = S._required});
    } else if constexpr (is_flag_v<s_t>) {
        auto const short_name =
            S._short_form.has_value ? std::optional{S._short_form.value} : std::optional<char>{};
        pure_flags.push_back(
            FlagHelp{
                .short_name = short_name,
                .long_name = S._long_form.as_string_view(),
                .description = S._help.as_string_view(),
                .is_required = S._required});
    } else if constexpr (is_flag_with_value_v<s_t>) {
        auto const short_name =
            S._short_form.has_value ? std::optional{S._short_form.value} : std::optional<char>{};
        flags_with_value.push_back(
            FlagHelp{
                .short_name = short_name,
                .long_name = S._long_form.as_string_view(),
                .description = S._help.as_string_view(),
                .is_required = S._required});
    } else if constexpr (is_subcommand_v<s_t>) {
        if (S._is_flag) {
            pure_flags.push_back(
                FlagHelp{
                    .long_name = S._name.as_string_view(),
                    .description = S._help.as_string_view()});

        } else {
            positional.push_back(
                PositionalHelp{
                    .name = S._name.as_string_view(), .description = S._help.as_string_view()});
        }
    } else {
        static_assert(false, "Invalid option");
    }
}

template <auto S1, auto S2, auto... Ops>
auto build_help_data(
    std::vector<PositionalHelp> &positional,
    std::vector<FlagHelp> &pure_flags,
    std::vector<FlagHelp> &flags_with_value) -> void {
    build_help_data<S1>(positional, pure_flags, flags_with_value);
    build_help_data<S2, Ops...>(positional, pure_flags, flags_with_value);
}

auto build_positionals_help(std::string &help, std::vector<PositionalHelp> &positional) -> void;
auto build_flags_help(std::string &help, std::vector<FlagHelp> &flags) -> void;

template <Str Usage, Str Description, auto... Ops>
[[nodiscard]] static auto make_help() -> std::string {
    std::vector<PositionalHelp> positional{};
    std::vector<FlagHelp> pure_flags{};
    std::vector<FlagHelp> flags_with_value{};

    build_help_data<Ops...>(positional, pure_flags, flags_with_value);

    auto help = std::string{};

    if constexpr (!Usage.is_empty()) {
        help += Usage.as_string_view();
    }
    if constexpr (!Description.is_empty()) {
        help += "\n\n";
        help += Description.as_string_view();
    }

    if (!positional.empty()) {
        help += "\n\n";
        help += "Arguments:\n\n";
        build_positionals_help(help, positional);
    }

    if (!pure_flags.empty()) {
        help += "\n\n";
        help += "Flags:\n\n";
        build_flags_help(help, pure_flags);
    }

    if (!flags_with_value.empty()) {
        help += "\n\n";
        build_flags_help(help, flags_with_value);
    }

    help.push_back('\n');
    return help;
}

struct WithCount {
    std::size_t count{};
};

struct DummyBase {};


}  // namespace detail

/// Collects all the descriptor to parse the arguments
///
/// A 'Usage' `Str` and a 'Description' `Str` must be provided (use `empty` to avoid writing them)
///
/// Provides a static member function `help` with the automatically generated help message
template <Str Usage, Str Description, auto... Ops>
struct [[nodiscard]] Options {
    [[nodiscard]] static auto help() -> std::string_view {
        static auto help_msg = detail::make_help<Usage, Description, Ops...>();
        return std::string_view{help_msg};
    }

private:
    static constexpr detail::rule_assertions::CheckRules<Ops...> rule_checker{};
};

consteval auto subcommand() -> Subcommand<0, 0, empty, empty, MutuallyExclusiveGroups<>> {
    return Subcommand<0, 0, empty, empty, MutuallyExclusiveGroups<>>{};
}

template <Str Usage, Str Description, auto... Ops>
constexpr auto options = Options<Usage, Description, Ops...>{};

template <auto S>
struct [[nodiscard]] CommandArgValue
    : std::conditional_t<is_flag_v<decltype(S)>, detail::WithCount, detail::DummyBase> {
    detail::result_type_t<S> value{detail::default_arg_value<S>()};
    bool is_used{false};
    static constexpr auto option = S;
};

template <auto... Ops>
class [[nodiscard]] Args;

template <typename>
struct ArgsFromSubCommand {
    static_assert(false, "Not a subcommand");
};

template <std::size_t N, std::size_t M, Str Usage, Str Description, typename Me, auto... Ops>
struct ArgsFromSubCommand<Subcommand<N, M, Usage, Description, Me, Ops...>> {
    using args_t = Args<Ops...>;
};

template <auto S>
struct [[nodiscard]] SubcommandArgValue {
    detail::result_type_t<S> value{detail::default_arg_value<S>()};
    bool is_used{false};
    ArgsFromSubCommand<decltype(S)>::args_t subcommands{};
    static constexpr auto option = S;
};

/// The data associated with each parsed command
///
/// Provides:
///
/// - the actual parsed value (`value`)
/// - if the value has been provided or a default has been used (`is_used`)
/// - the parsed subcommands (`subcommands`, if this is a subcommand directive)
template <auto S>
struct ArgValue
    : std::conditional_t<is_subcommand_v<decltype(S)>, SubcommandArgValue<S>, CommandArgValue<S>> {
};

namespace detail {
template <auto S, auto... Ss>
struct GetRet {
    using type = std::conditional_t<
        is_subcommand_v<decltype(S)>,
        typename GetRet<Ss...>::type,
        result_type_t<S>>;
};

template <auto S>
struct GetRet<S> {
    using type = result_type_t<S>;
};

template <auto S, auto... Ss>
struct GetWithInfoRet {
    using type = std::conditional_t<
        is_subcommand_v<decltype(S)>,
        typename GetWithInfoRet<Ss...>::type,
        ArgValue<S>>;
};

template <auto S>
struct GetWithInfoRet<S> {
    using type = ArgValue<S>;
};
}  // namespace detail

namespace detail {
template <auto S, auto R>
struct IsSameSpec: std::false_type {};

template <auto S>
struct IsSameSpec<S, S>: std::true_type {};

template <auto S, auto... Ss>
[[nodiscard]] consteval auto has_duplicate_mutually_exclusive_flags() -> bool {
    return std::disjunction_v<IsSameSpec<S, Ss>...>;
}

template <auto... Rs, auto... Ss>
[[nodiscard]] consteval auto are_valid_mutually_exclusive_flags(
    Options<Rs...>, MutuallyExclusive<Ss...>) -> bool {
    if constexpr (sizeof...(Ss) > sizeof...(Rs)) {
        return false;
    }
    return has_duplicate_mutually_exclusive_flags<Ss...>()
           && std::conjunction_v<std::disjunction<IsSameSpec<Ss, Rs>...>>;
}

template <auto... Rs, auto... Ss>
[[nodiscard]] consteval auto are_valid_mutually_exclusive_groups(
    Options<Rs...> opts, MutuallyExclusiveGroups<Ss...>) -> bool {
    // This 'if' is not needed, but on gcc 16.1.1 it removes a warning
    // for 'options' being unused
    if constexpr (sizeof...(Ss) == 0) {
        static_cast<void>(opts);
        return true;
    } else {
        return (... && are_valid_mutually_exclusive_flags(opts, Ss));
    }
}
}  // namespace detail

/// A container class for all parsed commands
///
/// Provides getter methods for directly accessing the value and accessing the value with more
/// contextual info (e.g. if the value has actually been provided or a default has been used)
///
/// The getters a generics over the desciptions, so for accessing the value of a description
/// `d` of type `Flag` use `args.get<d>()`. To access the same value with extra information use
/// `args.get_with_info<d>()`.
///
/// For accessing a value `d` of a subcommand `sb` use `args.get<sb, d>()` or
/// `args.get_with_info<sb, d>()`. Both functions are variadic in the sense that they can take an
/// arbitrary number of subcommands and a final descriptor, e.g. `args.get<sb_0, sb_1, sb_2, d>()`
template <auto... Ops>
class [[nodiscard]] Args {
public:
    explicit Args(std::tuple<ArgValue<Ops>...> results)
        : m_results{std::move(results)} {}

    explicit Args() = default;

    template <auto S>
    [[nodiscard]] constexpr auto get_with_info() const noexcept -> ArgValue<S> const & {
        return std::get<ArgValue<S>>(m_results);
    }

    template <auto S>
    [[nodiscard]] constexpr auto get() const noexcept -> detail::result_type_t<S> const & {
        return get_with_info<S>().value;
    }

    template <auto Sb, auto S>
    requires(is_subcommand_v<decltype(Sb)> && !is_subcommand_v<decltype(S)>)
    [[nodiscard]] constexpr auto get_with_info() const noexcept -> ArgValue<S> const & {
        return get_with_info<Sb>().subcommands.template get_with_info<S>();
    }

    template <auto Sb, auto S>
    requires(is_subcommand_v<decltype(Sb)> && !is_subcommand_v<decltype(S)>)
    [[nodiscard]] constexpr auto get() const noexcept -> detail::result_type_t<S> const & {
        return get_with_info<Sb>().subcommands.template get<S>();
    }

    template <auto Sb, auto S, auto... Ss>
    requires(is_subcommand_v<decltype(Sb)> && is_subcommand_v<decltype(S)> && sizeof...(Ss) > 0)
    [[nodiscard]] constexpr auto get() const noexcept -> detail::GetRet<Ss...>::type const & {
        return get_with_info<Sb>().subcommands.template get<S, Ss...>();
    }

    template <auto Sb, auto S, auto... Ss>
    requires(is_subcommand_v<decltype(Sb)> && is_subcommand_v<decltype(S)> && sizeof...(Ss) > 0)
    [[nodiscard]] constexpr auto get_with_info() const noexcept
        -> detail::GetWithInfoRet<Ss...>::type const & {
        return get_with_info<Sb>().subcommands.template get_with_info<S, Ss...>();
    }

private:
    std::tuple<ArgValue<Ops>...> m_results{};
};

namespace detail {
template <auto S>
concept ShortFlagObject =
    S._short_form.has_value && is_flag_v<decltype(S)> && !is_flag_with_value_v<decltype(S)>;

template <auto S>
concept ShortFlagWithValueObject = S._short_form.has_value && is_flag_with_value_v<decltype(S)>;

template <auto S>
concept LongFlagObject = is_flag_v<decltype(S)> && !is_flag_with_value_v<decltype(S)>;

template <auto S>
concept LongFlagWithValueObject = is_flag_with_value_v<decltype(S)>;

template <auto S>
concept PositionalObject = is_positional_v<decltype(S)>;

template <auto S>
concept SubcommandObject = is_subcommand_v<decltype(S)>;

template <auto S>
[[nodiscard]] constexpr auto arg_value_parameter(Typetag<ArgValue<S>>) -> decltype(S) const & {
    return S;
}

template <auto... Ops>
[[nodiscard]] auto nth_positional_argument_name(std::size_t nth) -> std::string_view {
    auto position_count = 0uz;
    template for (auto const &S : std::forward_as_tuple(Ops...)) {
        if constexpr (is_positional_v<decltype(S)>) {
            ++position_count;
            if (position_count == nth) {
                return S._name.as_string_view();
            }
        }
    }
    args_log_and_abort(std::format("cannot find name of positional argument number {}", nth));
    return "";
}

template <auto... Ss, auto... Gg>
[[nodiscard]] auto check_mutually_exclusive_set_satisfied(
    Args<Ss...> const &args, MutuallyExclusive<Gg...> mutually_exclusive, std::size_t index)
    -> std::optional<std::string> {
    auto const used_args = (0 + ... + args.template get_with_info<Gg>().is_used);
    if (mutually_exclusive.at_least_one && used_args == 0) {
        return std::format(
            "Mutually exclusive set number {} not satisfied. At least one argument is required.",
            index);
    }
    if (used_args > 1) {
        return std::format(
                "Mutually exclusive set number {} not satisfied. Too many arguments provided. "
                "Expected 1, provided: {}.",
                index,
                used_args);
    }
    return {};
}

template <auto... Ss, auto... Gg>
[[nodiscard]] auto check_mutually_exclusive_group_satisfied(
    Args<Ss...> const &args, MutuallyExclusiveGroups<Gg...>) -> std::optional<std::string> {
    if constexpr (sizeof...(Gg) == 0) {
        return {};
    } else {
        auto errors = std::vector<std::string>{};
        auto index = 0uz;
        template for (auto gg : {Gg...}) {
            if (auto err = check_mutually_exclusive_set_satisfied(args, gg, index);
                err.has_value()) {
                errors.push_back(std::move(err).value());
            }
            ++index;
        }
        if (!errors.empty()) {
            return std::move(errors) | std::views::join_with('\n') | std::ranges::to<std::string>();
        }
        return {};
    }
}
}  // namespace detail

struct [[nodiscard]] Help {
    std::string_view message;
};

struct [[nodiscard]] Error {
    std::string message;
};

struct NoArguments {};

template <auto... Ss>
using compile_result_t = std::variant<Args<Ss...>, Help, Error, NoArguments>;

template <auto... Ss>
[[nodiscard]] constexpr auto has_args(compile_result_t<Ss...> const &res) noexcept -> bool {
    return std::holds_alternative<Args<Ss...>>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto has_help(compile_result_t<Ss...> const &res) noexcept -> bool {
    return std::holds_alternative<Help>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto has_error(compile_result_t<Ss...> const &res) noexcept -> bool {
    return std::holds_alternative<Error>(res);
}

template <auto... Ss>
[[nodiscard]] constexpr auto is_empty(compile_result_t<Ss...> const &res) noexcept -> bool {
    return std::holds_alternative<NoArguments>(res);
}

/// Returns a reference to a constant `args::Args` object
///
/// Throws if there is no such object. This function should be used after checking with `has_args`
template <auto... Ss>
[[nodiscard]] constexpr auto get_args(compile_result_t<Ss...> const &res) -> Args<Ss...> const & {
    return std::get<Args<Ss...>>(res);
}

/// Returns a reference to a `args::Args` object
///
/// Throws if there is no such object. This function should be used after checking with `has_args`
template <auto... Ss>
[[nodiscard]] constexpr auto get_args(compile_result_t<Ss...> &res) -> Args<Ss...> & {
    return std::get<Args<Ss...>>(res);
}

/// Returns a `args::Args` object
///
/// Throws if there is no such object. This function should be used after checking with `has_args`
template <auto... Ss>
[[nodiscard]] constexpr auto get_args(compile_result_t<Ss...> &&res) -> Args<Ss...> {
    return std::get<Args<Ss...>>(std::move(res));
}

/// Returns a `args::Help` object
///
/// Throws if there is no such object. This function should be used after checking with `has_help`
template <auto... Ss>
[[nodiscard]] constexpr auto get_help(compile_result_t<Ss...> const &res) -> Help {
    return std::get<Help>(res);
}

/// Returns a reference to a constant `args::Error` object
///
/// Throws if there is no such object. This function should be used after checking with `has_error`
template <auto... Ss>
[[nodiscard]] constexpr auto get_error(compile_result_t<Ss...> const &res) -> Error const & {
    return std::get<Error>(res);
}

/// Returns a reference to a `args::Error` object
///
/// Throws if there is no such object. This function should be used after checking with `has_error`
template <auto... Ss>
[[nodiscard]] constexpr auto get_error(compile_result_t<Ss...> &res) -> Error & {
    return std::get<Error>(res);
}

/// Returns a `args::Error` object
///
/// Throws if there is no such object. This function should be used after checking with `has_error`
template <auto... Ss>
[[nodiscard]] constexpr auto get_error(compile_result_t<Ss...> &&res) -> Error {
    return std::get<Error>(std::move(res));
}

namespace literals {

template <Str X>
consteval auto operator""_str() -> decltype(X) {
    return X;
}

template <Str X>
consteval auto operator""_flag() -> decltype(X) {
    return X;
}

template <Str X>
requires(X.chars.size() == 2 && X.chars[1] == '\0')
consteval auto operator""_sflag() -> Opt<char> {
    return Opt<char>::with(X.chars[0]);
}
}  // namespace literals
}  // namespace args

#endif
