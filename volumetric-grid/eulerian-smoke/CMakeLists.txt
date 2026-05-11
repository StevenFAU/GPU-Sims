# Eulerian Smoke — first Tier-2 flagship Stack C sim.
# Consumes gpusims::common_cpp. Builds the binary `eulerian_smoke`.
#
# This sim is the first real consumer of common-cpp's OpenVDB writer
# (gpusims::vdb::writeFloatFrame). VDB export is gated at runtime by
# gpusims::vdb::isAvailable(), which returns true only when common-cpp
# was built with -DGPU_SIMS_USE_OPENVDB=ON. Stub mode compiles and runs
# fine; the VDB toggle in the panel becomes a no-op.

add_executable(eulerian_smoke
    src/main.cpp
)

target_link_libraries(eulerian_smoke
    PRIVATE
        gpusims::common_cpp
)

target_compile_features(eulerian_smoke PRIVATE cxx_std_20)

# Pass the shader directory into the binary so it can locate shaders regardless
# of the cwd at launch time. Mirrors common-cpp/examples/hello and reaction-diffusion-3d.
target_compile_definitions(eulerian_smoke PRIVATE
    GPU_SIMS_ES_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

# Output binary name keeps snake_case (Phase 1 convention; matches gpu_sims_hello,
# reaction_diffusion_3d).
set_target_properties(eulerian_smoke PROPERTIES
    OUTPUT_NAME       "eulerian_smoke"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Shaders are compiled at runtime via gpusims::vk::ShaderCompiler.
# Copy them next to the binary so cwd-relative loading works during smoke tests.
set(SHADER_SOURCES
    shaders/fullscreen.vert.glsl
    shaders/raymarch.frag.glsl
    shaders/advect_velocity.comp.glsl
    shaders/advect_scalar.comp.glsl
    shaders/apply_buoyancy.comp.glsl
    shaders/compute_curl.comp.glsl
    shaders/apply_vorticity_confinement.comp.glsl
    shaders/emit_sources.comp.glsl
    shaders/apply_boundaries.comp.glsl
    shaders/compute_divergence.comp.glsl
    shaders/jacobi_pressure.comp.glsl
    shaders/apply_pressure.comp.glsl
)

foreach(SHADER ${SHADER_SOURCES})
    set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(DST ${CMAKE_BINARY_DIR}/bin/eulerian-smoke/${SHADER})
    add_custom_command(
        OUTPUT ${DST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DST}
        DEPENDS ${SRC}
        COMMENT "Copying ${SHADER}"
    )
    list(APPEND SHADER_OUTPUTS ${DST})
endforeach()

add_custom_target(eulerian_smoke_shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(eulerian_smoke eulerian_smoke_shaders)
