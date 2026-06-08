set(
    examples
    00_basic
    01_subcommand
    02_validation
    #03_custom_parser
    04_mutually_exclusive
)
foreach(example ${examples})
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} PRIVATE args project_warnings)
    add_dependencies(build_all_examples ${example})
endforeach()
