include(FetchContent)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(catch2)


add_executable(args_test
    compiler.cpp
    parsers.cpp
    validators.cpp
    tokenizer.cpp
)

target_link_libraries(args_test PRIVATE project_warnings args Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
# include(CTest)
include(Catch)
catch_discover_tests(args_test)

add_dependencies(build_all_tests args_test)
# add_test(NAME args_test COMMAND args_test)
