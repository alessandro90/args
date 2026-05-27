#add_executable(args_example
    #00_basic.cpp
    #01_subcommand.cpp
    #02_validation.cpp
    #03_custom_parser.cpp
    #04_mutually_exclusive.cpp
#)

add_executable(args_example
    example.cpp
)

target_link_libraries(args_example PRIVATE args project_warnings)

add_dependencies(build_all_examples args_example)
