include(FetchContent)

set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER) # (Optional safety guard)
set(CMAKE_FETCHCONTENT_OVERHUNG_SYSTEM TRUE) # Treats Fetched content headers as SYSTEM

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(catch2)

# Suppress warnigns coming from Catch2 header files
# note: target the real concrete targets, not the aliases
if(TARGET Catch2)
    get_target_property(CATCH2_INCLUDES Catch2 INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(Catch2 PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${CATCH2_INCLUDES}")
endif()

if(TARGET Catch2WithMain)
    get_target_property(CATCH2_MAIN_INCLUDES Catch2WithMain INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(Catch2WithMain PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${CATCH2_MAIN_INCLUDES}")
endif()

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
