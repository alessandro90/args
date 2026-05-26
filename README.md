# args

## Introduction

`args` is a command-line argument parser. It features:

- Variadic positional arguments
- Flags specified multiple times
- Mutually exclusive flags and groups of mutually exclusive flags
- Subcommands
- Automatic help generation
- Arbitrary argument validation
- Optional and non-optional arguments
- Groups of short flags like `-abc` and `-abc=3`/`-abc 3`
- Parsing custom types directly into the final structure

To use the library define a set of constexpr objects for the expected command arguments. These objects are validated at compile-time and they define the structure of the parsed result. Meaning the result is a struct correctly typed based on the provided commands. See the [examples](./examples/) for more information.

### Quick example

For example for a simple flag one gets:

```cpp
static constexpr auto verbose = args::Flag{
    .long_form = "verbose"_flag
};

auto const rules = args::rules<verbose>;

// Attempts to parse the commands from input. If 'help' is detected it prints help on stdout and exits.
// If an error is found, it prints the error on stderr and the help message and exits.
auto const commands = args::parse_or_exit(argc, argv, rules);

// decltype(v) is `bool const &`. No casts are performed, commands contains the correct types
auto const &v = commands.get<verbose>(); // for trivial types you can of course just copy the value, but `get` always returns a reference
```

## Types of commands

All commands live inside the `args` namespace. They are just plain structs with all fields public (and documented).

- `Flag`: a simple boolean flag.
- `FlagWithValue`: a flag with an associated value, like `--cout 3` or `-c=8`.
- `Positional`: nameless position argument.
- `Subcommand`: defines a nested set of commands. It can be a flag (`--version`) or just a positional (`version`). It supports its own internal set of rules and mutually exclusive groups. Subcommands can also be arbitrarily nested.

Once the set of possible commands is defined they need to be gathered into a `Rules` object: `args::rules<cmd1, cmd2, ...>`. `Rules` performs several checks at compile-time, some of them are:

- duplicate names
- invalid names/flags
- invalid mutually exclusive groups
- invalid default values

### Default arguments

Most commands (even the non-optional ones) must have a default value. Because everything is computed at compile-time, all defaults need to be trivial types. For non-trivial types (strings, vectors and others), a lambda may be used. For example for a flag parsing a vector of integers one can write:

```cpp
static constexpr auto vec = args::FlagWithValue{
    .long_form = "a-vector"_flag,
    .default_value = [] { return std::vector<int>{}; }};
```

Exceptions are `Positional` that just needs a `.type = args::tag<the_type>` for deducing the type and `Subcommand` that actually does not really have a value on its own.

The library understands that the lambda is there for the sole purpose of allowing a non trivial type as default.

## Parsing

Provided the rules are defined, the result can be obtained with either:

- `try_parse`: returns a variant containing either the parsed commands, the requested 'help' message or an error.
- `parse_or_exit`: returns the parsed commands, otherwise log what did not work and close the application.

Helpers are defined to inspect and read the result:

- `has_error`, `has_args`, `has_help`.
- `get_error`, `get_args`, `get_help`. Calling a getter without checking first if the result actually contains that value will raise an exception.

## Parse result

The parse result is `args::Args`, the structure of which depends on the template arguments of the rules provided. It exposes the following methods:

- `get<cmd>()`: returns the parsed value associated with `cmd`. The default is provided if the command has not been set. Note that if `cmd` is requried but not provided, the parse will fail.
- `get_with_info<cmd>()`: Same as `get` but the value is wrapped inside a struct with additional data. The extra data depends on the type of `cmd`. For example for a repeatable flag it has `count` indicating the number of times the flag has been provided. `is_used` is also always provided indicating if `cmd` was provided as command line argument.
- both `get` and `get_with_info` supports subcommands like this: `get<sub_cmd_1, sub_cmd_2, cmd>()`. That means we are retrieving the `cmd` from a subcommand nested inside another subcommand.

Strings can be parsed as `std::string_view` that points directly to the `argv`, therefore no memory is allocated.

## Validation

All flags and positional arguments supports validation. A validator is is a struct with two fields:

```cpp
template <typename V, typename ErrFn>
struct Validator {
    V fn;          // Validation logic: bool(TargetType const&)
    ErrFn err_fn;  // Error generator: std::string(TargetType const&)
};
```

Both fields are functions. `V` is the validation function. It should accept the expected parsed type and return `true` is the validation succeeded. `ErrFn` is the function used to display the error; it should take the parsed value as input and return a `std::string`.

Validators can be composed in several way. Custom validators are supported.

### Predefined validators:

- `always`: Unconditional pass.
- `less_than`, `less_or_equal`, `greater_than`, `greater_or_equal`, `equal_to`.
- `any_of`: Short-circuiting logical disjunction over multiple validators.
- `inclusive_range`, `exclusive_range`, `half_open_range`.

### Functional Combinators

- `ForEach`: Maps a scalar validator over every element within an iterable container.
- `Compose`: Chains sequential validators together sequentially (left-to-right evaluation).
- `len`: Extracts value dimensions (`.size()`) before forwarding execution.
- `Pipe`: Combines a property extraction transformer (like `len`) with a distinct evaluator.
- `And`, `Or`, `Not`, `Xor`: Standard logical operators evaluating boolean conditions across child validators.

### Validation example

Restricting a incoming `std::string_view` argument to a length bounds constraints between 1 and 50 characters:

```cpp
static constexpr auto name = args::FlagWithValue{
    .long_form = "name"_flag,
    .default_value = [] { return std::string_view{""}; },
    .validator = args::Pipe<args::len, args::inclusive_range<1, 50>>};
```

## Custom parsers

Custom objects can be parsed. To write a custom parsed write a specialization of `args::parsers::Parser<T>`. See the examples for more details.

## Integration

The library can be included in a project via Cmake's FetchContent

```
FetchContent_Declare(
    args
    GIT_REPOSITORY https://github.com/alessandro90/args.git
    GIT_TAG        trunk
)

FetchContent_MakeAvailable(args)

target_link_libraries(my_app PRIVATE args::args)
```

Headers are then imported as `args/args.hpp`, etc..

## Building from source

The project builds with Cmake. Tested only using gcc. C++26 support is required. At the moment there really no support for other compilers. Many several warnings exists only for C++. However the code is fully portable and the CMakeLists could be adjust to support multiple compilrs.

```bash
mkdir build && cd build
cmake ..
make
```

Or

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

The main command builds tests and examples.

### Specific targets

- _build_all_tests_ Build just the tests.
- _build_all_examples_ Build just the examples.
