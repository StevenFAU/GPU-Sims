cmake_minimum_required(VERSION 3.25)

project(gpu_sims
    VERSION     0.1.0
    DESCRIPTION "GPU-accelerated physics and emergence simulations"
    LANGUAGES   C CXX
)

# ----------------------------------------------------------------------------
# Project-wide configuration
# ----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_C_STANDARD            11)
set(CMAKE_C_STANDARD_REQUIRED   ON)

# Generate compile_commands.json for clangd / IDE indexers.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if not specified. Debug builds enable validation layers.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# Warnings: error-as-warning on the warnings we care about, opt-in to extras.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /permissive-)
endif()

# ----------------------------------------------------------------------------
# Build options (top-level toggles for opt-in subsystems)
# ----------------------------------------------------------------------------
option(GPU_SIMS_BUILD_EXAMPLES "Build the common-cpp hello-world example" ON)
option(GPU_SIMS_USE_OPENVDB    "Enable OpenVDB export (requires libopenvdb-dev)" OFF)
option(GPU_SIMS_USE_ALEMBIC    "Enable Alembic export (requires libalembic-dev)" OFF)

# Per-sim opt-ins. All OFF in Phase 1; flipped ON as sims are built.
option(GPU_SIMS_BUILD_CLOSED_FORM         "Build closed-form sims"          OFF)
option(GPU_SIMS_BUILD_AGENT_BASED         "Build agent-based sims"          OFF)
option(GPU_SIMS_BUILD_CONTINUOUS_CA       "Build continuous-CA sims"        OFF)
option(GPU_SIMS_BUILD_VOLUMETRIC_GRID     "Build volumetric-grid sims"      OFF)
option(GPU_SIMS_BUILD_PARTICLE_FLUIDS     "Build particle-fluid sims"       OFF)
option(GPU_SIMS_BUILD_HYBRID_PARTICLE_GRID "Build hybrid particle-grid sims" OFF)
option(GPU_SIMS_BUILD_QUANTUM             "Build quantum visualization sims" OFF)

# ----------------------------------------------------------------------------
# Subdirectories
# ----------------------------------------------------------------------------
add_subdirectory(common/common-cpp)

if(GPU_SIMS_BUILD_EXAMPLES)
    add_subdirectory(common/common-cpp/examples/hello)
endif()

# Per-sim subdirectories are added when sims are implemented; commented stubs
# are left here as the agreed insertion point.

# Continuous CA sims (Phase 3+).
add_subdirectory(continuous-ca/reaction-diffusion-3d)

add_subdirectory(volumetric-grid/eulerian-smoke)
add_subdirectory(particle-fluids/sph-water)

# if(GPU_SIMS_BUILD_VOLUMETRIC_GRID)
#     add_subdirectory(volumetric-grid/lattice-boltzmann)
# endif()

# ----------------------------------------------------------------------------
# Status banner
# ----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "GPU-Sims ${PROJECT_VERSION}")
message(STATUS "  CMake:        ${CMAKE_VERSION}")
message(STATUS "  Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:     ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  OpenVDB:      ${GPU_SIMS_USE_OPENVDB}")
message(STATUS "  Alembic:      ${GPU_SIMS_USE_ALEMBIC}")
message(STATUS "  Examples:     ${GPU_SIMS_BUILD_EXAMPLES}")
message(STATUS "")
