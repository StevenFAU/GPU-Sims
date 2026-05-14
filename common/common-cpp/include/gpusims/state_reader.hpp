#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace gpusims {

// Load a capture written by StateWriter.
//
// Usage:
//     auto cap = StateReader::open("captures/capture_0001");
//     if (cap) {
//         auto cam_meta = cap->meta("camera");        // nlohmann::json
//         auto bytes    = cap->buffer("particles");   // std::vector<uint8_t>
//     }

class StateReader {
public:
    static std::optional<StateReader> open(const std::filesystem::path& capture_dir);

    // Locate the most recent capture_NNNN/ subdirectory under `root`. Returns
    // nullopt if no captures exist or `root` does not exist. Selection is by
    // the largest NNNN suffix (sorted lexicographically on padded names).
    static std::optional<std::filesystem::path>
    findLatest(const std::filesystem::path& root);

    // Top-level metadata. Returns null json if key not present.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    nlohmann::json meta(const std::string& key) const;

    // Buffer descriptor (the per-buffer meta passed to StateWriter::saveBuffer).
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    nlohmann::json bufferMeta(const std::string& name) const;

    // Raw bytes for a saved buffer. Empty if not present.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    std::vector<std::uint8_t> buffer(const std::string& name) const;

// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    std::uint32_t frameIndex() const { return frame_idx_; }

// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    const std::filesystem::path& dir() const { return dir_; }

private:
    StateReader() = default;

    std::filesystem::path dir_;
    nlohmann::json        state_;
    std::uint32_t         frame_idx_ = 0;
};

}  // namespace gpusims
