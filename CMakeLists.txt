cmake_minimum_required(VERSION 4.0.0)
project(CPP_ARGS LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
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

add_library(cpp_args STATIC
	src/cpp_args.cpp
)

if( supported )
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET cpp_args PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTS)
	include(CTest)
	add_subdirectory(test)
endif()
