add_executable(args_example
    example.cpp
)

target_link_libraries(args_example PRIVATE args project_warnings)

add_dependencies(build_all_examples args_example)
