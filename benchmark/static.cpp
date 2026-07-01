#define args taywee
#include <args.hxx>
#undef args
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

void bm_static_taywee_args(benchmark::State &state) {
    for (auto _ : state) {
        // Use taywee:: instead of args::
        taywee::ArgumentParser parser("test_bench");

        taywee::ValueFlag<int> integer(parser, "integer", "The integer", {'i', "integer"});
        taywee::ValueFlag<double> decimal(parser, "float", "The float", {'f', "float"});
        taywee::Flag verbose(parser, "verbose", "The verbose flag", {'v', "verbose"});
        taywee::Flag debug(parser, "debug", "The debug flag", {'d', "debug"});

        try {
            parser.ParseCLI(fake_static_argc, fake_static_argv.data());

            benchmark::DoNotOptimize(parser);
            benchmark::DoNotOptimize(integer);
            benchmark::DoNotOptimize(decimal);
            benchmark::DoNotOptimize(verbose);
            benchmark::DoNotOptimize(debug);
            benchmark::ClobberMemory();
        } catch (taywee::ParseError const &err) {
            state.SkipWithError(err.what());
        } catch (taywee::ValidationError const &err) {
            state.SkipWithError(err.what());
        }
    }
}

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
}  // namespace

BENCHMARK(bm_static_argparse);
BENCHMARK(bm_static_cli11);
BENCHMARK(bm_static_cxxopts);
BENCHMARK(bm_static_args);
BENCHMARK(bm_static_taywee_args);
