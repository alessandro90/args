// cmake ../.. -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON -DBUILD_EXAMPLES=OFF
// -DBUILD_TESTING=OFF
#include <argparse/argparse.hpp>
#include <array>
#include <benchmark/benchmark.h>
#include <CLI/CLI.hpp>
#include <cxxopts.hpp>
#include "args/args.hpp"
#include "args/types.hpp"

namespace {
constexpr auto fake_argv = std::array{"./program", "-i", "42", "--verbose"};
constexpr auto fake_argc = static_cast<int>(fake_argv.size());

void bm_argparse(benchmark::State &state) {
    for (auto _ : state) {
        argparse::ArgumentParser program("test_bench");

        program.add_argument("-i").scan<'i', int>();  // scan as integer

        program.add_argument("--verbose").implicit_value(true).default_value(false);

        try {
            program.parse_args(fake_argc, fake_argv.data());
            benchmark::DoNotOptimize(program);
        } catch (std::runtime_error const &err) {
            state.SkipWithError(err.what());
        }
    }
}

BENCHMARK(bm_argparse);

void bm_cli11(benchmark::State &state) {
    for (auto _ : state) {
        CLI::App app{"CLI11 Benchmark"};

        int integer_value = 0;
        bool verbose_flag = false;

        app.add_option("-i,--integer", integer_value, "An integer option");
        app.add_flag("--verbose", verbose_flag, "A boolean flag");

        try {
            app.parse(fake_argc, fake_argv.data());

            benchmark::DoNotOptimize(integer_value);
            benchmark::DoNotOptimize(verbose_flag);
            benchmark::ClobberMemory();
        } catch (CLI::ParseError const &e) {
            state.SkipWithError(e.what());
        }
    }
}

BENCHMARK(bm_cli11);

void bm_cxxopts(benchmark::State &state) {
    for (auto _ : state) {
        cxxopts::Options options("cxxopts_bench", "Benchmark for cxxopts");

        options.add_options()("i,integer", "An integer option", cxxopts::value<int>())(
            "verbose", "A boolean flag", cxxopts::value<bool>()->default_value("false"));

        try {
            auto result = options.parse(fake_argc, fake_argv.data());

            benchmark::DoNotOptimize(result);
            benchmark::ClobberMemory();
        } catch (cxxopts::exceptions::exception const &e) {
            state.SkipWithError(e.what());
        }
    }
}

BENCHMARK(bm_cxxopts);

constexpr auto c_i = args::flag_with_value<int>().Long("integer").Short('i');
constexpr auto c_verbose = args::flag().Long("verbose");
constexpr auto options = args::options<args::empty, args::empty, c_i, c_verbose>;

void bm_args(benchmark::State &state) {
    for (auto _ : state) {
        auto commands = args::try_parse(fake_argc, fake_argv.data(), options);
        benchmark::DoNotOptimize(commands);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(bm_args);
}  // namespace

BENCHMARK_MAIN();
