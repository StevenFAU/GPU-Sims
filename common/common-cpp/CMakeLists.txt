# ============================================================================
# common-cpp — shared infrastructure for native (Vulkan) sims
# ============================================================================
#
# Provides reusable building blocks consumed by every Stack C simulation:
# - Vulkan context, window, swapchain, command-buffer management
# - Compute and graphics pipeline wrappers
# - Hot-reload (with include-graph tracking and graceful pipeline recreation)
# - GPU profiling (RAII scopes, ring-buffered timestamp queries)
# - State capture (JSON metadata + binary blobs)
# - Camera (free-fly, arcball, orbit)
# - ImGui setup helpers
# - VDB / Alembic export (stubs in Phase 1; real impls when first sim needs)
#
# Per-sim CMakeLists consume this as:
#     target_link_libraries(my_sim PRIVATE gpu_sims_common_cpp)
# Includes are written as:
#     #include <gpusims/camera.hpp>
#     #include <gpusims/vk/context.hpp>
# ============================================================================

# ----------------------------------------------------------------------------
# Required dependencies (Vulkan SDK is mandatory)
# ----------------------------------------------------------------------------
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

# ----------------------------------------------------------------------------
# Source-buildable dependencies (FetchContent)
# ----------------------------------------------------------------------------
include(cmake/deps.cmake)

# ----------------------------------------------------------------------------
# Optional dependencies (find_package; gated behind options)
# ----------------------------------------------------------------------------
include(cmake/optional_deps.cmake)

# ----------------------------------------------------------------------------
# Library target
# ----------------------------------------------------------------------------
add_library(gpu_sims_common_cpp STATIC
    # Generic
    src/log.cpp
    src/camera.cpp
    src/hot_reload.cpp
    src/gpu_profiler.cpp
    src/state_writer.cpp
    src/state_reader.cpp
    src/imgui_setup.cpp
    src/vdb_writer.cpp
    src/alembic_writer.cpp

    # Vulkan
    src/vk/debug.cpp
    src/vk/context.cpp
    src/vk/window.cpp
    src/vk/frame.cpp
    src/vk/buffer.cpp
    src/vk/image.cpp
    src/vk/shader_compiler.cpp
    src/vk/compute_pipeline.cpp
    src/vk/graphics_pipeline.cpp
    src/vk/renderer.cpp
)

# Public headers exposed to consumers.
target_include_directories(gpu_sims_common_cpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Internal include path for src/ to find each other.
target_include_directories(gpu_sims_common_cpp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ----------------------------------------------------------------------------
# Public link dependencies — sims using common-cpp inherit these.
# ----------------------------------------------------------------------------
target_link_libraries(gpu_sims_common_cpp PUBLIC
    Vulkan::Vulkan
    glfw
    glm::glm
    imgui
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    GPUOpen::VulkanMemoryAllocator
    Threads::Threads
)

# Private deps (used by .cpp files but not exposed in headers).
target_link_libraries(gpu_sims_common_cpp PRIVATE
    shaderc::shaderc
    vma_impl
)

# ----------------------------------------------------------------------------
# Optional features as compile-time defines.
# ----------------------------------------------------------------------------
if(GPU_SIMS_USE_OPENVDB AND TARGET OpenVDB::openvdb)
    target_link_libraries(gpu_sims_common_cpp PUBLIC OpenVDB::openvdb)
    target_compile_definitions(gpu_sims_common_cpp PUBLIC GPU_SIMS_HAVE_OPENVDB=1)
endif()

if(GPU_SIMS_USE_ALEMBIC AND TARGET Alembic::Alembic)
    target_link_libraries(gpu_sims_common_cpp PUBLIC Alembic::Alembic)
    target_compile_definitions(gpu_sims_common_cpp PUBLIC GPU_SIMS_HAVE_ALEMBIC=1)
endif()

# Validation layers in Debug, off in Release.
target_compile_definitions(gpu_sims_common_cpp PUBLIC
    $<$<CONFIG:Debug>:GPU_SIMS_VALIDATION_LAYERS=1>
    $<$<NOT:$<CONFIG:Debug>>:GPU_SIMS_VALIDATION_LAYERS=0>
)

# Vulkan version target. We commit to 1.3.
target_compile_definitions(gpu_sims_common_cpp PUBLIC
    GPU_SIMS_VULKAN_API_VERSION=VK_API_VERSION_1_3
)

# Make the namespaced alias available so consumers can write
#     target_link_libraries(my_sim PRIVATE gpusims::common_cpp)
add_library(gpusims::common_cpp ALIAS gpu_sims_common_cpp)
