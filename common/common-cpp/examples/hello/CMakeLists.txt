# ============================================================================
# common-cpp hello-world example
# ============================================================================
add_executable(gpu_sims_hello
    main.cpp
)

target_link_libraries(gpu_sims_hello PRIVATE
    gpu_sims_common_cpp
)

# Pass the shader directory path into the binary so it can locate shaders
# regardless of the cwd at launch time.
target_compile_definitions(gpu_sims_hello PRIVATE
    GPU_SIMS_HELLO_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

set_target_properties(gpu_sims_hello PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
