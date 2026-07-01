cmake_minimum_required(VERSION 4.0.0)
project(args_benchmarks LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.5
    SYSTEM
)
FetchContent_MakeAvailable(google_benchmark)

FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.6.2
    SYSTEM
)
FetchContent_MakeAvailable(cli11)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG        v3.2
    SYSTEM
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        v3.3.1
    SYSTEM
)
FetchContent_MakeAvailable(cxxopts)

# 1. Declare the dependency but skip processing its CMakeLists.txt
FetchContent_Declare(
    taywee_args
    GIT_REPOSITORY https://github.com/Taywee/args.git
    GIT_TAG        6.4.16
    SYSTEM
    SOURCE_SUBDIR  "bypassed" # Prevents CMake from running Taywee's CMakeLists.txt
)

FetchContent_MakeAvailable(taywee_args)

# Create your custom interface target
add_library(taywee_args_lib INTERFACE)
target_include_directories(taywee_args_lib SYSTEM INTERFACE ${taywee_args_SOURCE_DIR})

# 4. Create the namespaced alias for your target link block
add_library(taywee::args ALIAS taywee_args_lib)

add_executable(
    bench_exe
    bench.cpp
    static.cpp
    dynamic.cpp
)

target_link_libraries(bench_exe
    PRIVATE
        args::args
        taywee::args
        project_warnings
        benchmark::benchmark
        CLI11::CLI11
        argparse
        cxxopts::cxxopts
)

target_compile_options(bench_exe PRIVATE -O3 -freflection)

if(supported)
    set_property(TARGET bench_exe PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_dependencies(args_bench bench_exe)
