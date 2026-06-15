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
- Supports for non default intializable types
- Parsing of common containers: `std::vector`, `std::set`, etc.. Provided their contained types are supported
  - Concepts are used to parse the container, therefore even custom container could be automatically be supported provided they satisfy the necessary concepts (TODO: explain concepts).
- the output of the parsing is compatible with `std::println`. It a parsed type is not printable its name will be printed instead.

To use the library define a set of constexpr objects for the expected command arguments. These objects are validated at compile-time and they define the structure of the parsed result. Meaning the result is a struct correctly typed based on the provided commands. See the [examples](./examples/) for more information.

The result object `args::Args` does not allocate anything on the heap. Of course parsed types that do allocate (e.g. `std::vector`) will still allocate.

### Quick example

For example for a simple flag one gets:

```cpp
static constexpr auto verbose = args::flag().Long("verbose"); // No short version provided. Default is `false`.

auto const options = args::options<verbose>;

// Attempts to parse the commands from input. If 'help' is detected it prints help on stdout and exits.
// If an error is found, it prints the error on stderr and the help message and exits.
auto const commands = args::parse_or_exit(argc, argv, options);

// decltype(v) is `bool const &`. No casts are performed, commands contains the correct types
auto const &v = commands.get<verbose>(); // for trivial types you can of course just copy the value, but `get` always returns a reference
```

## Types of commands

All commands live inside the `args` namespace. They are just plain structs with all fields public (and documented).

- `Flag`: a simple boolean flag.
- `FlagWithValue`: a flag with an associated value, like `--cout 3` or `-c=8`.
- `Positional`: nameless position argument.
- `Subcommand`: defines a nested set of commands. It can be a flag (`--version`) or just a positional (`version`). It supports its own internal set of options and mutually exclusive groups. Subcommands can also be arbitrarily nested.

All the commands have a `function constructor + fluent interface` to build them. See the examples for the complete API.

Once the set of possible commands is defined they need to be gathered into a `options` object: `args::options<cmd1, cmd2, ...>`. `options` performs several checks at compile-time, some of them are:

- duplicate names
- invalid names/flags
- invalid mutually exclusive groups
- invalid default values

### Default validation

Defaults values are checked at compile time only if the container can be _default_ constructed in a constexpr context. `std::vector` can do that. As of C++26, `std::unordered_set` cannot for example. For such types the default validation is skipped and the library will blindly use the default you provide (or not provide) without any check. The library will also print a warning indicating the type that cannot be checked.

### Default arguments

Most commands (even the non-optional ones) must have a default value. Because everything is computed at compile-time, all defaults need to be trivial types. For non-trivial types (strings, vectors and others), a lambda may be used. For example for a flag parsing a vector of integers one can write:

```cpp
static constexpr auto vec = args::flag_with_value<std::vector<int>>()
                            .Long("a-vector")
                            .Default([] { return std::vector<int>{1, 2}; }); // You need a default only if it is different from the default provided by the type itself.
```

The library understands that the lambda is there for the sole purpose of allowing a non trivial type as default.

### Non default-initializable types

The library can handle non default-initializable types. Such types must be either have a default value set via `Default` or be explicitly _required_ with `Required(true)`. The result returned by the library is actally a static storage in which the value lives (see [`args::LazyStorage<T>`](./include/args/lazy_storage.hpp)). The inner type can be retrieved with `.as_ref()`. See [`05_non_default_intializable_arguments.cpp`](./examples/05_non_default_intializable_arguments.cpp).

### `std::optional` arguments

Arguments of type `std::optional` cannot be set as _required_ and cannot have a default value. Their default value is just an empty optional.

## Parsing

Provided the options are defined, the result can be obtained with either:

- `try_parse`: returns a variant containing either the parsed commands, the requested 'help' message or an error.
- `parse_or_exit`: returns the parsed commands, otherwise log what did not work and close the application.

Helpers are defined to inspect and read the result if `parse_or_exit` is not used.

- `is_empty`. `true` if no arguments where provided.
- `has_error`, `has_args`, `has_help`.
- `get_error`, `get_args`, `get_help`. Calling a getter without checking first if the result actually contains that value will raise an exception.

## Parse result

The parse result is `args::Args`, the structure of which depends on the template arguments of the options provided. It exposes the following methods:

- `get<cmd>()`: returns the parsed value associated with `cmd`. The default is provided if the command has not been set. Note that if `cmd` is requried but not provided, the parse will fail.
- `get_with_info<cmd>()`: Same as `get` but the value is wrapped inside a struct with additional data. The extra data depends on the type of `cmd`. For example for a repeatable flag it has `count` indicating the number of times the flag has been provided. `is_used` is also always provided indicating if `cmd` was provided as command line argument.
- both `get` and `get_with_info` supports subcommands like this: `get<sub_cmd_1, sub_cmd_2, cmd>()`. That means we are retrieving the `cmd` from a subcommand nested inside another subcommand.
- To get a subcommand itself, only `get_with_info<subcommand>()` can be used. A subcommand `get` would just return its name, which is not useful.

Strings can be parsed as `std::string_view` that points directly to the `argv`, therefore no memory is allocated. `std::string` can also be used.

## Validation

All flags and positional arguments supports validation. A validator is defined as:

```cpp
template <typename V, typename ErrFn>
struct Validator {
    V fn;          // Validation logic: bool(TargetType const&)
    ErrFn err_fn;  // Error generator: std::string(TargetType const&)
};
```

Both fields are functions. `V` is the validation function. It should accept the expected parsed type and return `true` is the validation succeeded, `false` otherwise. `ErrFn` is the function used to display the error; it should take the parsed value as input (the expected type of argument) and return a `std::string`.

Validators can be composed in several way. Custom validators are supported.

### Predefined validators:

- `always`: Unconditional pass.
- `less_than`, `less_or_equal`, `greater_than`, `greater_or_equal`, `equal_to`.
- `choices`: Short-circuiting logical disjunction over multiple validators.
- `inclusive_range`, `exclusive_range`, `half_open_range`.

### Functional Combinators

- `All`: Maps a scalar validator over every element within an iterable container.
- `Compose`: Chains sequential validators together sequentially (left-to-right evaluation).
- `len`: Extracts value dimensions (`.size()`) before forwarding execution.
- `Pipe`: Combines a property extraction transformer (like `len`) with a distinct evaluator.
- `And`, `Or`, `Not`, `Xor`: Standard logical operators evaluating boolean conditions across child validators.

### Validation example

Restricting a incoming `std::string_view` argument to a length bounds constraints between 1 and 50 characters:

```cpp
static constexpr auto name = args::flag_with_value<std::string_view>()
                            .Long("name")
                            .Validator(args::Pipe<args::len, args::inclusive_range<1, 50>>);
```

See [02_validation.cpp](./examples/02_validation.cpp) for an example.

## Custom parsers

Custom objects can be parsed. To write a custom parser write a specialization of `args::parsers::Parser<T>`. See [03_custom_parser.cpp](./examples/03_custom_parser.cpp) for an example.

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

The project builds with Cmake. Tested only using gcc. _C++26 and reflection_ (via `-freflection`) support is required. At the moment there is really no support for other compilers. Many several warnings exists only for C++. However the code is fully portable and the CMakeLists could be adjusted to support multiple compilers.

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

The `make` command without any specific target builds tests and examples.

### Specific targets

- _args_tests_ Build just the tests.
- _args_examples_ Build just the examples. add `-DBUILD_CUSTOM_PARSER_EXAMPLE` to build `03_custom_parser`. It downlaods the `nlohman` json library. So disable the flag if do not want to download it.
