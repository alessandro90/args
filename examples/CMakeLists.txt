add_executable(examples
    example.cpp
)

target_link_libraries(examples PRIVATE cpp_args project_warnings)

add_dependencies(examples examples)
