# Reaction-Diffusion 3D — first Stack C sim.
# Consumes gpusims::common_cpp. Builds the binary `reaction_diffusion_3d`.

add_executable(reaction_diffusion_3d
    src/main.cpp
)

target_link_libraries(reaction_diffusion_3d
    PRIVATE
        gpusims::common_cpp
)

target_compile_features(reaction_diffusion_3d PRIVATE cxx_std_20)

# Pass the shader directory into the binary so it can locate shaders regardless
# of the cwd at launch time. Mirrors common-cpp/examples/hello.
target_compile_definitions(reaction_diffusion_3d PRIVATE
    GPU_SIMS_RD3D_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

# Output binary name keeps snake_case (Phase 1 convention; matches gpu_sims_hello).
set_target_properties(reaction_diffusion_3d PROPERTIES
    OUTPUT_NAME       "reaction_diffusion_3d"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Shaders are compiled at runtime via gpusims::vk::ShaderCompiler.
# Copy them next to the binary so cwd-relative loading works during smoke tests.
set(SHADER_SOURCES
    shaders/rd_update.comp.glsl
    shaders/raymarch.frag.glsl
    shaders/fullscreen.vert.glsl
)

foreach(SHADER ${SHADER_SOURCES})
    set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(DST ${CMAKE_BINARY_DIR}/bin/${SHADER})
    add_custom_command(
        OUTPUT ${DST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DST}
        DEPENDS ${SRC}
        COMMENT "Copying ${SHADER}"
    )
    list(APPEND SHADER_OUTPUTS ${DST})
endforeach()

add_custom_target(reaction_diffusion_3d_shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(reaction_diffusion_3d reaction_diffusion_3d_shaders)
