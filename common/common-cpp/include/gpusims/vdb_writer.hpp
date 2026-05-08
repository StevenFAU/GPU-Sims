#pragma once

#include <cstdint>
#include <filesystem>

#include <glm/glm.hpp>

namespace gpusims {

// OpenVDB writer for volumetric grid sims.
//
// In Phase 1, this is a stub: if GPU_SIMS_HAVE_OPENVDB is not defined at
// compile time (i.e., GPU_SIMS_USE_OPENVDB=OFF in CMake), all functions log
// a warning on first call and return false. When OpenVDB is enabled, real
// implementations are provided.
//
// Data convention:
//   - 3D grids are linearized x-fastest, then y, then z.
//   - voxel_size is in world units per cell.
//   - origin is the world-space position of voxel (0,0,0) corner.

namespace vdb {

// Write a single dense float grid to a .vdb file. Returns false on failure.
bool writeFloatGrid(const std::filesystem::path& path,
                    const float*                 data,
                    glm::ivec3                   dims,
                    float                        voxel_size,
                    glm::vec3                    origin   = glm::vec3(0.0f),
                    const char*                  grid_name = "density");

// Write a single dense vec3 grid to a .vdb file (interleaved xyz floats).
bool writeVec3Grid(const std::filesystem::path& path,
                   const float*                 data,
                   glm::ivec3                   dims,
                   float                        voxel_size,
                   glm::vec3                    origin   = glm::vec3(0.0f),
                   const char*                  grid_name = "velocity");

// Convenience for sequence frames. Writes to <base>_<NNNN>.vdb.
bool writeFloatFrame(const std::filesystem::path& base,
                     std::uint32_t                frame_idx,
                     const float*                 data,
                     glm::ivec3                   dims,
                     float                        voxel_size,
                     glm::vec3                    origin   = glm::vec3(0.0f),
                     const char*                  grid_name = "density");

// True if this build was compiled with OpenVDB support.
bool isAvailable();

}  // namespace vdb
}  // namespace gpusims
