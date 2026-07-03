# args

## Introduction

`args` is a command-line argument parser. It features:

- Variadic positional arguments
- Flags specified multiple times
- Mutually exclusive flags and groups of mutually exclusive flags
- Subcommands
- Automatic help generation. _The quality of the help message is currenlty quite low. Needs to be improved_.
- Arbitrary argument validation
- Optional and non-optional arguments
- Groups of short flags like `-abc` and `-abc=3`/`-abc 3`
- Parsing custom types directly into the final structure
- Supports for non default intializable types
- Parsing of common containers: `std::vector`, `std::set`, etc.. provided their contained types are supported
  - Concepts are used to parse the containers, therefore even custom containers are automatically supported provided they satisfy the necessary concepts defined in [`helpers.hpp`](./include/args/helpers.hpp).
- the output of the parsing is compatible with `std::println`. If a parsed type is not printable its name will be printed instead.
- _nargs_. Stuff like `--name 1 2 3 4` with the resulting parsed value being a vector of `1,2,3,4`.
- flags with optional values, like `-j` and `-j 8`, see #optional-arguments

To use the library define a set of constexpr objects for the expected command arguments. These objects are validated at compile-time and they define the structure of the parsed result. Meaning the result is a struct correctly typed based on the provided commands. See the [examples](./examples/) for more information.

The result object `args::Args` does not allocate anything on the heap. Of course parsed types that do allocate (e.g. `std::vector`) will still allocate. Another vector is allocated is performed for the tokenization of the input arguments, before creating `args::Args`.

My goal was to write a library that does not perform any conversion when reading an already parsed value and also with a lookup system checked a compile-time. Here no conversion is made because the final class contains exactly the expected types. Lookup is checked at compile-time because the 'key' used to access the value is a compile-time constant, so providing something wrong produces a compile-time error.

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

The library can handle non default-initializable types. Such types must be either have a default value set via `Default` or be explicitly _required_ with `Required(true)`. The result returned by the library is actally a static storage in which the value lives (see [`args::LazyStorage<T>`](./include/args/lazy_storage.hpp)). The inner type can be retrieved with `.as_ref()`. See [`05_non_default_intializable_arguments.cpp`](./examples/05_non_default_initializable_arguments.cpp).

### `std::optional` arguments (#optional-arguments)

Arguments of type `std::optional` cannot a default value. Their default value is just an empty optional. If the argument is a `args::Positional`, it cannot have a default value (it is an empty `std::optional` by default). If the argument is a `args::flag_with_value` than it has a special logic such that the flag can be specifies both with and without its corresponding value. Meaning both `-j` and `-j8` are valid. In the first case the flag is used but its value is an empty `std::optional`. In the second case its value is an `std::optional{8}`.

## Parsing

Provided the options are defined, the result can be obtained with either:

- `try_parse`: returns a variant containing either the parsed commands, the requested 'help' message or an error.
- `parse_or_exit`: returns the parsed commands, otherwise log what did not work and close the application.

Helpers are defined to inspect and read the result if `parse_or_exit` is not used.

- `is_empty`. `true` if no arguments where provided.
- `has_error`, `has_args`, `has_help`.
- `get_error`, `get_args`, `get_help`. Calling a getter if the result actually contains that value will raise an exception.

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

## Benchmarks

The `benchmark` directory contains some benchmarks against commonly used similar libraries: [`argparse`](https://github.com/p-ranav/argparse), [`cxxopts`](https://github.com/jarro2783/cxxopts), [`CLI11`](https://github.com/cliutils/cli11), [`args`](https://github.com/Taywee/args) (mine has actually the same name and exported namespace because I found out about his one only much later for some reason). Only two simple tests are performad:

- parsing simple flags/scalar options
- parsing `std::vector<int>`

The results looks good, but surely the compared libraries offer a much greater functionality and I am not sure of how representative the tests are. Anyway on a machine with a _AMD Ryzen 7 7800X3D_ these are the results:

| Benchmark                                                   | Time (ns) | CPU (ns) | Iterations |
| :---------------------------------------------------------- | --------: | -------: | :--------: |
| bm_dynamic_argparse                                         |      1082 |     1075 |   650057   |
| bm_dynamic_cli11                                            |      2818 |     2793 |   248141   |
| bm_dynamic_cxxopts                                          |      5800 |     5762 |   121146   |
| bm_dynamic_taywee_args                                      |       807 |      804 |   871004   |
| :small_red_triangle:**bm_dynamic_args**:small_red_triangle: |       542 |      539 |  1290237   |
| bm_static_argparse                                          |      1361 |     1355 |   516353   |
| bm_static_cli11                                             |      3978 |     3960 |   177150   |
| bm_static_cxxopts                                           |      6913 |     6873 |   101897   |
| bm_static_taywee_args                                       |      1323 |     1305 |   539586   |
| :small_red_triangle:**bm_static_args**:small_red_triangle:  |       302 |      300 |  2344296   |

These tests are no comprehensive in any way. I just wanted to see how the library performs with a couple of basic cases.

## Building from source

The project builds with CMake. Tested only using gcc. _C++26 and reflection_ (via `-freflection`) support is required. At the moment there is really no support for other compilers. Many several warnings exists only for C++. However the code is fully portable and the CMakeLists could be adjusted to support multiple compilers. Default build is debug, use `CMAKE_BUILD_TYPE=Relase` for release version.

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

The `make` command without any specific target builds tests and examples. Benchmarks are not built by default. To build the benchmarks use `-DBUILD_BENCHMARKS=ON`.

### Specific targets

- _args_tests_ Build just the tests.
- _args_examples_ Build just the examples. add `-DBUILD_CUSTOM_PARSER_EXAMPLE` to build `03_custom_parser`. It downlaods the `nlohman` json library. So disable the flag if do not want to download it.
- _args_bench_ Build the benchmarks. You should configure cmake with `-DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF`.
