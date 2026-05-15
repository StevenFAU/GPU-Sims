# Lattice Boltzmann — second Stack C volumetric-grid sim. Phase 12.
# Consumes gpusims::common_cpp. Builds the binary `lattice_boltzmann`.
#
# Mirrors volumetric-grid/eulerian-smoke/CMakeLists.txt at HEAD c5955d3.
# This sim is the second real consumer of common-cpp's writeVec3Frame
# (writeVec3Grid stub-shipped since Phase 1; first-exercised here for
# optional per-frame velocity-field VDB export). VDB export is gated at
# runtime by gpusims::vdb::isAvailable() and at compile time by
# -DGPU_SIMS_USE_OPENVDB=ON. Stub mode compiles and runs; the VDB toggle
# in the panel becomes a no-op.

add_executable(lattice_boltzmann
    src/main.cpp
)

target_link_libraries(lattice_boltzmann
    PRIVATE
        gpusims::common_cpp
)

target_compile_features(lattice_boltzmann PRIVATE cxx_std_20)

# Pass the shader directory into the binary so it can locate shaders regardless
# of the cwd at launch time. Mirrors common-cpp/examples/hello and eulerian-smoke.
target_compile_definitions(lattice_boltzmann PRIVATE
    GPU_SIMS_LBM_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

# Output binary name keeps snake_case (Phase 1 convention; matches gpu_sims_hello,
# eulerian_smoke, reaction_diffusion_3d, sph_water).
set_target_properties(lattice_boltzmann PROPERTIES
    OUTPUT_NAME       "lattice_boltzmann"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Shaders are compiled at runtime via gpusims::vk::ShaderCompiler.
# Copy them next to the binary so cwd-relative loading works during smoke tests.
# Per-sim shader-copy destination is sim-namespaced (bin/lattice-boltzmann/shaders/)
# to avoid Ninja "multiple rules generate" collisions with other sims that share
# shader filenames (fullscreen.vert.glsl is shared with eulerian-smoke).
# See project-state.md § 7 "Shader-copy destination must namespace by sim".
set(SHADER_SOURCES
    shaders/lattice_constants.glsl
    shaders/fullscreen.vert.glsl
    shaders/init_equilibrium.comp.glsl
    shaders/compute_moments.comp.glsl
    shaders/collide.comp.glsl
    shaders/stream.comp.glsl
    shaders/apply_boundaries.comp.glsl
    shaders/streamline_advect.comp.glsl
    shaders/velmag.frag.glsl
    shaders/streamline.vert.glsl
    shaders/streamline.frag.glsl
)

foreach(SHADER ${SHADER_SOURCES})
    set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(DST ${CMAKE_BINARY_DIR}/bin/lattice-boltzmann/${SHADER})
    add_custom_command(
        OUTPUT ${DST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DST}
        DEPENDS ${SRC}
        COMMENT "Copying ${SHADER}"
    )
    list(APPEND SHADER_OUTPUTS ${DST})
endforeach()

add_custom_target(lattice_boltzmann_shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(lattice_boltzmann lattice_boltzmann_shaders)
