include(FetchContent)

option(BUILD_CUSTOM_PARSER_EXAMPLE "Download nlohmann_json for example source files" ON)

set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER) # (Optional safety guard)
set(CMAKE_FETCHCONTENT_OVERHUNG_SYSTEM TRUE) # Treats Fetched content headers as SYSTEM

if (BUILD_CUSTOM_PARSER_EXAMPLE)
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    )
    FetchContent_MakeAvailable(json)

    # suppress warnings from nlohman header files
    if(TARGET nlohmann_json)
        get_target_property(JSON_INCLUDES nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
        set_target_properties(nlohmann_json PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${JSON_INCLUDES}")
    endif()
endif()

set(
    examples
    00_basic
    01_subcommand
    02_validation
    03_custom_parser
    04_mutually_exclusive
    05_non_default_initializable_arguments
    06_nargs
)

foreach(example ${examples})
    if (NOT "${example}" STREQUAL "03_custom_parser")
        add_executable(${example} ${example}.cpp)
        target_link_libraries(${example} PRIVATE args project_warnings)
        add_dependencies(args_examples ${example})
    else()
        if (BUILD_CUSTOM_PARSER_EXAMPLE AND "${example}" STREQUAL "03_custom_parser")
            add_executable(${example} ${example}.cpp)
            target_link_libraries(${example} PRIVATE args project_warnings nlohmann_json::nlohmann_json)
            add_dependencies(args_examples ${example})
        endif()
    endif()

endforeach()
