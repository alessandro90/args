cmake_minimum_required(VERSION 4.0.0)
project(
    args
    VERSION 0.1.0
    LANGUAGES
    CXX
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/args/version.hpp"
)


set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(project_warnings INTERFACE)

target_compile_options(project_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wdouble-promotion
    -Wformat=2
    -Wformat-security
    -Wformat-signedness
    -Wnull-dereference
    -Wtrivial-auto-var-init
    -Wunused-parameter
    -Wunused-const-variable=2
    -Wuninitialized
    -Wmaybe-uninitialized
    -Wstringop-overflow=4
    -Wconversion
    -Wfloat-conversion
    -Wsign-conversion
    -Warith-conversion
    -Wbool-compare
    -Wduplicated-branches
    -Wduplicated-cond
    -Wfloat-equal
    -Wshadow
    -Wundef
    -Wunused-macros
    -Wcast-qual
    -Wcast-align=strict
    -Wlogical-op
    -Wmissing-declarations
    -Wredundant-decls
    # -Winline
    -Wlong-long
    -Wimplicit-fallthrough=5
    -Wmissing-include-dirs
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -g")

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

add_library(args STATIC
	src/tokenizer.cpp
	src/types.cpp
)

target_compile_options(args PUBLIC -freflection)

add_library(args::args ALIAS args)

target_link_libraries(args PRIVATE project_warnings)

target_include_directories(args PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(args PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/generated")

if( supported )
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET args PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()

add_custom_target(args_tests)
add_custom_target(args_examples)
add_custom_target(args_bench)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTING "Build tests" ON)
    if(BUILD_TESTING)
        include(CTest)
        add_subdirectory(test)
    endif()

    option(BUILD_EXAMPLES "Build examples" ON)
    if(BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()

    option(BUILD_BENCHMARKS "Build the benchmark suite" OFF)
    if(BUILD_BENCHMARKS)
        # Prevents Google Benchmark from building its own internal tests/benchmarks
        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        
        add_subdirectory(benchmark)
    endif()
endif()

