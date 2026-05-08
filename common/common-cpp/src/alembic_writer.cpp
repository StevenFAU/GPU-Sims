#include <gpusims/alembic_writer.hpp>

#include <gpusims/log.hpp>

#include <atomic>

#if GPU_SIMS_HAVE_ALEMBIC
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/Abc/All.h>
#endif

namespace gpusims::abc {

namespace {

void logUnavailableOnce() {
    static std::atomic<bool> warned{false};
    bool expected = false;
    if (warned.compare_exchange_strong(expected, true)) {
        logWarn("alembic-writer: GPU_SIMS_HAVE_ALEMBIC=0; Alembic exports skipped. "
                "Rebuild with -DGPU_SIMS_USE_ALEMBIC=ON and libalembic-dev installed.");
    }
}

}  // namespace

bool isAvailable() {
#if GPU_SIMS_HAVE_ALEMBIC
    return true;
#else
    return false;
#endif
}

#if GPU_SIMS_HAVE_ALEMBIC

class RealParticleWriter : public ParticleWriter {
public:
    RealParticleWriter(const std::filesystem::path& path, double fps)
        : archive_(Alembic::AbcCoreOgawa::WriteArchive(),
                   path.string(),
                   Alembic::Abc::ErrorHandler::kThrowPolicy),
          time_sampling_(std::make_shared<Alembic::AbcCoreAbstract::TimeSampling>(
              1.0 / fps, 0.0)) {
        Alembic::AbcGeom::OObject top(archive_, Alembic::Abc::kTop);
        Alembic::AbcGeom::OPoints points_obj(top, "particles", time_sampling_);
        points_ = points_obj.getSchema();
    }

    bool writeFrame(const ParticleFrame& frame) override {
        try {
            if (!frame.positions || frame.count == 0) return false;
            std::vector<Alembic::Abc::V3f> positions(frame.count);
            std::vector<Alembic::Abc::uint64_t> ids(frame.count);
            for (std::size_t i = 0; i < frame.count; ++i) {
                positions[i] = Alembic::Abc::V3f(
                    frame.positions[3 * i + 0],
                    frame.positions[3 * i + 1],
                    frame.positions[3 * i + 2]);
                ids[i] = frame.ids ? frame.ids[i] : i;
            }
            Alembic::AbcGeom::OPointsSchema::Sample sample(
                Alembic::Abc::V3fArraySample(positions),
                Alembic::Abc::UInt64ArraySample(ids));
            if (frame.velocities) {
                std::vector<Alembic::Abc::V3f> vels(frame.count);
                for (std::size_t i = 0; i < frame.count; ++i) {
                    vels[i] = Alembic::Abc::V3f(
                        frame.velocities[3 * i + 0],
                        frame.velocities[3 * i + 1],
                        frame.velocities[3 * i + 2]);
                }
                sample.setVelocities(Alembic::Abc::V3fArraySample(vels));
            }
            points_.set(sample);
            return true;
        } catch (const std::exception& e) {
            logError("alembic-writer: writeFrame failed: {}", e.what());
            return false;
        }
    }

private:
    Alembic::Abc::OArchive                                  archive_;
    Alembic::AbcCoreAbstract::TimeSamplingPtr               time_sampling_;
    Alembic::AbcGeom::OPointsSchema                         points_;
};

#else

class StubParticleWriter : public ParticleWriter {
public:
    StubParticleWriter() { logUnavailableOnce(); }
    bool writeFrame(const ParticleFrame&) override { return false; }
};

#endif

std::unique_ptr<ParticleWriter> ParticleWriter::create(const std::filesystem::path& path,
                                                       double fps) {
#if GPU_SIMS_HAVE_ALEMBIC
    try {
        return std::unique_ptr<ParticleWriter>(new RealParticleWriter(path, fps));
    } catch (const std::exception& e) {
        logError("alembic-writer: failed to open {}: {}", path.string(), e.what());
        return nullptr;
    }
#else
    (void)path; (void)fps;
    return std::unique_ptr<ParticleWriter>(new StubParticleWriter());
#endif
}

ParticleWriter::~ParticleWriter() = default;

}  // namespace gpusims::abc
