
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
constexpr auto fake_dynamic_argv =
    std::array{"./program", "-c", "10,20,30,40,50", "-m", "100", "-m", "200", "-m", "300"};
constexpr int fake_dynamic_argc = static_cast<int>(fake_dynamic_argv.size());

void bm_dynamic_argparse(benchmark::State &state) {
    for (auto _ : state) {
        argparse::ArgumentParser program("test_bench");

        program.add_argument("-c", "--csv")
            .help("argparse requires manual split for CSV strings, or sequential args");

        program.add_argument("-m", "--multi").append().scan<'i', int>();

        try {
            program.parse_args(fake_dynamic_argc, fake_dynamic_argv.data());
            benchmark::DoNotOptimize(program);
            benchmark::ClobberMemory();
        } catch (std::runtime_error const &err) {
            state.SkipWithError(err.what());
        }
    }
}

void bm_dynamic_cli11(benchmark::State &state) {
    for (auto _ : state) {
        CLI::App app{"CLI11 Benchmark"};

        std::vector<int> csv_vals;
        std::vector<int> multi_vals;

        app.add_option("-c,--csv", csv_vals)->delimiter(',');
        app.add_option("-m,--multi", multi_vals);

        try {
            app.parse(fake_dynamic_argc, fake_dynamic_argv.data());

            benchmark::DoNotOptimize(csv_vals);
            benchmark::DoNotOptimize(multi_vals);
            benchmark::ClobberMemory();
        } catch (CLI::ParseError const &e) {
            state.SkipWithError(e.what());
        }
    }
}

void bm_dynamic_cxxopts(benchmark::State &state) {
    for (auto _ : state) {
        cxxopts::Options options("cxxopts_bench", "Benchmark for cxxopts");

        options.add_options()("c,csv", "CSV Vector", cxxopts::value<std::vector<int>>())(
            "m,multi", "Multi-flag Vector", cxxopts::value<std::vector<int>>());
        try {
            auto result = options.parse(fake_dynamic_argc, fake_dynamic_argv.data());

            benchmark::DoNotOptimize(result);
            benchmark::ClobberMemory();
        } catch (cxxopts::exceptions::exception const &e) {
            state.SkipWithError(e.what());
        }
    }
}

constexpr auto flag_c = args::flag_with_value<std::vector<int>>().Long("csv").Short('c');
constexpr auto flag_m =
    args::flag_with_value<std::vector<int>>().Long("multi").Short('m').Repeatable();

constexpr auto dynamic_options = args::options<args::empty, args::empty, flag_c, flag_m>;

void bm_dynamic_args(benchmark::State &state) {
    for (auto _ : state) {
        auto commands =
            args::try_parse(fake_dynamic_argc, fake_dynamic_argv.data(), dynamic_options);
        benchmark::DoNotOptimize(commands);
        benchmark::ClobberMemory();
    }
}
}  // namespace

BENCHMARK(bm_dynamic_argparse);
BENCHMARK(bm_dynamic_cli11);
BENCHMARK(bm_dynamic_cxxopts);
BENCHMARK(bm_dynamic_args);
