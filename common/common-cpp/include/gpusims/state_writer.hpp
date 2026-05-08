#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

namespace gpusims {

// Save full simulation state to disk in an inspectable format.
//
// Layout:
//     <root>/capture_<NNNN>/
//         state.json     -- metadata; describes each blob
//         <name>.bin     -- one binary file per saveBuffer() call
//
// The format is intentionally simple so Python can reload it:
//     import json, numpy as np
//     meta = json.load(open('capture_0001/state.json'))
//     particles = np.fromfile('capture_0001/particles.bin', dtype=np.float32)

class StateWriter {
public:
    explicit StateWriter(std::filesystem::path root_dir);

    // Begin a new capture. Creates capture_<frame_idx_padded>/ subdir.
    void beginFrame(std::uint32_t frame_idx);

    // Set arbitrary metadata; copied into state.json under the "meta" key.
    void setMeta(const std::string& key, const nlohmann::json& value);

    // Save a binary blob.
    //   name: identifier (used as filename stem and key in state.json)
    //   data: pointer to bytes
    //   bytes: byte count
    //   meta: per-buffer metadata (e.g., {"count": 1000000, "stride": 32})
    //         describes how to interpret the binary data on reload.
    void saveBuffer(const std::string&    name,
                    const void*           data,
                    std::size_t           bytes,
                    const nlohmann::json& meta = {});

    // Write state.json and close the frame's directory.
    void endFrame();

    // Path to current capture directory (valid between begin/endFrame).
    const std::filesystem::path& currentDir() const { return current_dir_; }

private:
    std::filesystem::path root_;
    std::filesystem::path current_dir_;
    nlohmann::json        state_;
    bool                  in_frame_ = false;
};

}  // namespace gpusims
