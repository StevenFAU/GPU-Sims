#include <gpusims/vdb_writer.hpp>

#include <gpusims/log.hpp>

#if GPU_SIMS_HAVE_OPENVDB
#include <openvdb/openvdb.h>
#include <openvdb/io/File.h>
#include <openvdb/tools/Dense.h>
#endif

#include <atomic>
#include <cstdio>
#include <mutex>

namespace gpusims::vdb {

namespace {

void logUnavailableOnce() {
    static std::atomic<bool> warned{false};
    bool expected = false;
    if (warned.compare_exchange_strong(expected, true)) {
        logWarn("vdb-writer: GPU_SIMS_HAVE_OPENVDB=0; VDB exports skipped. "
                "Rebuild with -DGPU_SIMS_USE_OPENVDB=ON and libopenvdb-dev installed.");
    }
}

#if GPU_SIMS_HAVE_OPENVDB
void initOpenVdbOnce() {
    static std::once_flag flag;
    std::call_once(flag, []() { openvdb::initialize(); });
}
#endif

std::filesystem::path frameSequencePath(const std::filesystem::path& base, std::uint32_t frame) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "_%04u.vdb", frame);
    auto p = base;
    p += buf;
    return p;
}

}  // namespace

bool isAvailable() {
#if GPU_SIMS_HAVE_OPENVDB
    return true;
#else
    return false;
#endif
}

bool writeFloatGrid(const std::filesystem::path& path,
                    const float*                 data,
                    glm::ivec3                   dims,
                    float                        voxel_size,
                    glm::vec3                    origin,
                    const char*                  grid_name) {
#if GPU_SIMS_HAVE_OPENVDB
    if (!data || dims.x <= 0 || dims.y <= 0 || dims.z <= 0) return false;
    initOpenVdbOnce();
    try {
        openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(0.0f);
        grid->setName(grid_name ? grid_name : "density");
        grid->setTransform(openvdb::math::Transform::createLinearTransform(voxel_size));
        grid->setGridClass(openvdb::GRID_FOG_VOLUME);

        const openvdb::CoordBBox bbox(
            openvdb::Coord(0, 0, 0),
            openvdb::Coord(dims.x - 1, dims.y - 1, dims.z - 1));
        openvdb::tools::Dense<const float> dense(bbox, data);
        openvdb::tools::copyFromDense(dense, grid->tree(), 0.0f);

        // Apply origin offset by post-translate.
        if (origin != glm::vec3(0.0f)) {
            grid->setTransform(openvdb::math::Transform::createLinearTransform(voxel_size));
            grid->transform().postTranslate(openvdb::Vec3d(origin.x, origin.y, origin.z));
        }

        openvdb::GridPtrVec grids;
        grids.push_back(grid);
        openvdb::io::File file(path.string());
        file.write(grids);
        file.close();
        return true;
    } catch (const std::exception& e) {
        logError("vdb-writer: failed writing {}: {}", path.string(), e.what());
        return false;
    }
#else
    (void)data; (void)dims; (void)voxel_size; (void)origin; (void)grid_name; (void)path;
    logUnavailableOnce();
    return false;
#endif
}

bool writeVec3Grid(const std::filesystem::path& path,
                   const float*                 data,
                   glm::ivec3                   dims,
                   float                        voxel_size,
                   glm::vec3                    origin,
                   const char*                  grid_name) {
#if GPU_SIMS_HAVE_OPENVDB
    if (!data || dims.x <= 0 || dims.y <= 0 || dims.z <= 0) return false;
    initOpenVdbOnce();
    try {
        openvdb::Vec3SGrid::Ptr grid = openvdb::Vec3SGrid::create(openvdb::Vec3s(0.0f));
        grid->setName(grid_name ? grid_name : "velocity");
        grid->setTransform(openvdb::math::Transform::createLinearTransform(voxel_size));
        grid->setGridClass(openvdb::GRID_STAGGERED);

        // Manual fill: openvdb::tools::copyFromDense doesn't have a Vec3 specialization
        // we can rely on across versions, so we set values voxel-by-voxel.
        auto accessor = grid->getAccessor();
        for (int z = 0; z < dims.z; ++z) {
            for (int y = 0; y < dims.y; ++y) {
                for (int x = 0; x < dims.x; ++x) {
                    const std::size_t i = static_cast<std::size_t>(
                        x + dims.x * (y + dims.y * z)) * 3;
                    accessor.setValue(openvdb::Coord(x, y, z),
                                      openvdb::Vec3s(data[i + 0], data[i + 1], data[i + 2]));
                }
            }
        }

        if (origin != glm::vec3(0.0f)) {
            grid->transform().postTranslate(openvdb::Vec3d(origin.x, origin.y, origin.z));
        }

        openvdb::GridPtrVec grids;
        grids.push_back(grid);
        openvdb::io::File file(path.string());
        file.write(grids);
        file.close();
        return true;
    } catch (const std::exception& e) {
        logError("vdb-writer: failed writing {}: {}", path.string(), e.what());
        return false;
    }
#else
    (void)data; (void)dims; (void)voxel_size; (void)origin; (void)grid_name; (void)path;
    logUnavailableOnce();
    return false;
#endif
}

bool writeFloatFrame(const std::filesystem::path& base,
                     std::uint32_t                frame_idx,
                     const float*                 data,
                     glm::ivec3                   dims,
                     float                        voxel_size,
                     glm::vec3                    origin,
                     const char*                  grid_name) {
    return writeFloatGrid(frameSequencePath(base, frame_idx), data, dims,
                          voxel_size, origin, grid_name);
}

}  // namespace gpusims::vdb
