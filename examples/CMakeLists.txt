#add_executable(args_example
    #01_basic.cpp
    #02_subcommand.cpp
    #03_validation.cpp
    #04_custom_parser.cpp
#)

add_executable(args_example
    example.cpp
)

target_link_libraries(args_example PRIVATE args project_warnings)

add_dependencies(build_all_examples args_example)
