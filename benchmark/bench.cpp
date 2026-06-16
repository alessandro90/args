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
constexpr auto fake_static_argv =
    std::array{"./program", "-i", "42", "-f", "3.14159", "--verbose", "-d"};
constexpr int fake_static_argc = static_cast<int>(fake_static_argv.size());

void bm_static_argparse(benchmark::State &state) {
    for (auto _ : state) {
        argparse::ArgumentParser program("test_bench");

        program.add_argument("-i", "--integer").scan<'i', int>();
        program.add_argument("-f", "--float").scan<'g', double>();
        program.add_argument("-v", "--verbose").implicit_value(true).default_value(false);
        program.add_argument("-d", "--debug").implicit_value(true).default_value(false);

        try {
            program.parse_args(fake_static_argc, fake_static_argv.data());
            benchmark::DoNotOptimize(program);
            benchmark::ClobberMemory();
        } catch (std::runtime_error const &err) {
            state.SkipWithError(err.what());
        }
    }
}

BENCHMARK(bm_static_argparse);

void bm_static_cli11(benchmark::State &state) {
    for (auto _ : state) {
        CLI::App app{"CLI11 Benchmark"};

        int integer_val = 0;
        double float_val = 0.0;
        bool verbose_flag = false;
        bool debug_flag = false;

        app.add_option("-i,--integer", integer_val);
        app.add_option("-f,--float", float_val);
        app.add_flag("-v,--verbose", verbose_flag);
        app.add_flag("-d,--debug", debug_flag);

        try {
            app.parse(fake_static_argc, fake_static_argv.data());

            benchmark::DoNotOptimize(integer_val);
            benchmark::DoNotOptimize(float_val);
            benchmark::DoNotOptimize(verbose_flag);
            benchmark::DoNotOptimize(debug_flag);
            benchmark::ClobberMemory();
        } catch (CLI::ParseError const &e) {
            state.SkipWithError(e.what());
        }
    }
}

BENCHMARK(bm_static_cli11);

void bm_static_cxxopts(benchmark::State &state) {
    for (auto _ : state) {
        cxxopts::Options options("cxxopts_bench", "Benchmark for cxxopts");
        options.add_options()("i,integer", "Int", cxxopts::value<int>())(
            "f,float",
            "Float",
            cxxopts::value<double>())("v,verbose", "Verbose")("d,debug", "Debug");

        try {
            auto result = options.parse(fake_static_argc, fake_static_argv.data());

            benchmark::DoNotOptimize(result);
            benchmark::ClobberMemory();
        } catch (cxxopts::exceptions::exception const &e) {
            state.SkipWithError(e.what());
        }
    }
}

BENCHMARK(bm_static_cxxopts);

constexpr auto flag_i = args::flag_with_value<int>().Long("integer").Short('i');
constexpr auto flag_f = args::flag_with_value<double>().Long("float").Short('f');
constexpr auto flag_v = args::flag().Long("verbose").Short('v');
constexpr auto flag_d = args::flag().Long("debug").Short('d');
constexpr auto static_options =
    args::options<args::empty, args::empty, flag_i, flag_f, flag_v, flag_d>;

void bm_static_args(benchmark::State &state) {
    for (auto _ : state) {
        auto commands = args::try_parse(fake_static_argc, fake_static_argv.data(), static_options);
        benchmark::DoNotOptimize(commands);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(bm_static_args);

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

BENCHMARK(bm_dynamic_argparse);

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

BENCHMARK(bm_dynamic_cli11);

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

BENCHMARK(bm_dynamic_cxxopts);


constexpr auto flag_c = args::flag_with_value<std::vector<int>>().Long("csv").Short('c');
constexpr auto flag_m = args::flag_with_value<std::vector<int>>().Long("multi").Short('m');

constexpr auto dynamic_options = args::options<args::empty, args::empty, flag_c, flag_m>;

void bm_dynamic_args(benchmark::State &state) {
    for (auto _ : state) {
        auto commands =
            args::try_parse(fake_dynamic_argc, fake_dynamic_argv.data(), dynamic_options);
        benchmark::DoNotOptimize(commands);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(bm_dynamic_args);

}  // namespace

BENCHMARK_MAIN();
