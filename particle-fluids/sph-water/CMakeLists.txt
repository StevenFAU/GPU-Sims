# SPH Water — first Tier-2 particle-fluids flagship; first Alembic real-impl consumer.
# Consumes gpusims::common_cpp. Builds the binary `sph_water`.
#
# This sim is the first real consumer of common-cpp's Alembic writer
# (gpusims::abc::ParticleWriter). Alembic export is gated at runtime by
# gpusims::abc::isAvailable(), which returns true only when common-cpp was
# built with -DGPU_SIMS_USE_ALEMBIC=ON. Stub mode compiles and runs fine;
# the Alembic toggle in the panel becomes a no-op.

add_executable(sph_water
    src/main.cpp
)

target_link_libraries(sph_water
    PRIVATE
        gpusims::common_cpp
)

target_compile_features(sph_water PRIVATE cxx_std_20)

# Pass the shader directory into the binary so it can locate shaders regardless
# of the cwd at launch time. Mirrors common-cpp/examples/hello, reaction-diffusion-3d,
# and eulerian-smoke.
target_compile_definitions(sph_water PRIVATE
    GPU_SIMS_SPH_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

# Output binary name keeps snake_case (Phase 1 convention; matches gpu_sims_hello,
# reaction_diffusion_3d, eulerian_smoke).
set_target_properties(sph_water PROPERTIES
    OUTPUT_NAME       "sph_water"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Shaders are compiled at runtime via gpusims::vk::ShaderCompiler.
# Copy them next to the binary so cwd-relative loading works during smoke tests.
# Sim-namespaced destination `bin/sph-water/...` per § 7 banked convention
# "Shader-copy destination must namespace by sim" (Phase 8 retro at eulerian-smoke).
set(SHADER_SOURCES
    # Shared fullscreen vertex
    shaders/fullscreen.vert.glsl
    # Spatial-hash kernels (7: 6 base + 1 second-level scan for large cell grids)
    shaders/morton_code.comp.glsl
    shaders/cell_count.comp.glsl
    shaders/prefix_sum_local.comp.glsl
    shaders/prefix_sum_block.comp.glsl
    shaders/prefix_sum_block_l2.comp.glsl
    shaders/prefix_sum_addback.comp.glsl
    shaders/scatter.comp.glsl
    # DFSPH solver kernels (5)
    shaders/density_alpha.comp.glsl
    shaders/divergence_solve.comp.glsl
    shaders/density_solve.comp.glsl
    shaders/pressure_apply.comp.glsl
    shaders/integrate_forces.comp.glsl
    # Emitter + initial fill (2)
    shaders/apply_emitter.comp.glsl
    shaders/initial_fill.comp.glsl
    # Bilateral smoothing for screen-space fluid (1)
    shaders/bilateral_smooth.comp.glsl
    # Particle sprite depth + thickness (4)
    shaders/particle_sprite.vert.glsl
    shaders/particle_sprite.frag.glsl
    shaders/thickness.vert.glsl
    shaders/thickness.frag.glsl
    # Final composite (1; reuses fullscreen.vert.glsl)
    shaders/composite.frag.glsl
)

foreach(SHADER ${SHADER_SOURCES})
    set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(DST ${CMAKE_BINARY_DIR}/bin/sph-water/${SHADER})
    add_custom_command(
        OUTPUT ${DST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DST}
        DEPENDS ${SRC}
        COMMENT "Copying ${SHADER}"
    )
    list(APPEND SHADER_OUTPUTS ${DST})
endforeach()

add_custom_target(sph_water_shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(sph_water sph_water_shaders)
