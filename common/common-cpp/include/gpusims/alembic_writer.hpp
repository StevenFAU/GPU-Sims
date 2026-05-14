#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include <glm/glm.hpp>

namespace gpusims {

// Alembic writer for particle-fluid sims and meshes.
//
// In Phase 1, this is a stub: if GPU_SIMS_HAVE_ALEMBIC is not defined at
// compile time, all functions log a warning on first call and return false.
// Real implementations land when the first Alembic-consuming sim (likely
// SPH water) is built.

namespace abc {

// Particle frame for streaming particle exports.
struct ParticleFrame {
    const float*    positions  = nullptr;  // 3 floats per particle (x, y, z)
    const float*    velocities = nullptr;  // 3 floats per particle (optional)
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    const float*    radii      = nullptr;  // 1 float per particle (optional)
    const std::uint64_t* ids   = nullptr;  // optional
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    std::size_t     count      = 0;
};

// Streaming particle writer. One instance per .abc file; writeFrame is called
// once per simulation frame to be exported.
class ParticleWriter {
public:
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    static std::unique_ptr<ParticleWriter> create(const std::filesystem::path& path,
                                                  double fps = 24.0);
    virtual ~ParticleWriter();

// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    virtual bool writeFrame(const ParticleFrame& frame) = 0;

protected:
    ParticleWriter() = default;
};

// True if this build was compiled with Alembic support.
bool isAvailable();

}  // namespace abc
}  // namespace gpusims
