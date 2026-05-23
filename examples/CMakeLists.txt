add_executable(cpp_args_example
    example.cpp
)

target_link_libraries(cpp_args_example PRIVATE cpp_args project_warnings)

add_dependencies(build_all_examples cpp_args_example)
