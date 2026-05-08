#include <gpusims/state_writer.hpp>

#include <cstdio>
#include <fstream>

#include <gpusims/log.hpp>

namespace gpusims {

namespace {
std::filesystem::path frameDirName(std::uint32_t frame_idx) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "capture_%04u", frame_idx);
    return std::filesystem::path(buf);
}
}

StateWriter::StateWriter(std::filesystem::path root_dir) : root_(std::move(root_dir)) {
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
    if (ec) {
        logError("state-writer: failed to create directory {}: {}", root_.string(), ec.message());
    }
}

void StateWriter::beginFrame(std::uint32_t frame_idx) {
    if (in_frame_) {
        logWarn("state-writer: beginFrame called while already in a frame; flushing first");
        endFrame();
    }
    current_dir_ = root_ / frameDirName(frame_idx);
    std::error_code ec;
    std::filesystem::create_directories(current_dir_, ec);
    state_      = nlohmann::json::object();
    state_["frame"]   = frame_idx;
    state_["meta"]    = nlohmann::json::object();
    state_["buffers"] = nlohmann::json::array();
    in_frame_   = true;
}

void StateWriter::setMeta(const std::string& key, const nlohmann::json& value) {
    if (!in_frame_) {
        logWarn("state-writer: setMeta('{}') called outside a frame", key);
        return;
    }
    state_["meta"][key] = value;
}

void StateWriter::saveBuffer(const std::string&    name,
                             const void*           data,
                             std::size_t           bytes,
                             const nlohmann::json& meta) {
    if (!in_frame_) {
        logWarn("state-writer: saveBuffer('{}') called outside a frame", name);
        return;
    }
    const std::filesystem::path bin = current_dir_ / (name + ".bin");
    std::ofstream out(bin, std::ios::binary);
    if (!out) {
        logError("state-writer: failed to open {}", bin.string());
        return;
    }
    out.write(static_cast<const char*>(data), static_cast<std::streamsize>(bytes));
    if (!out.good()) {
        logError("state-writer: failed to write {} bytes to {}", bytes, bin.string());
        return;
    }
    nlohmann::json desc = meta.is_null() ? nlohmann::json::object() : meta;
    desc["name"]  = name;
    desc["file"]  = name + ".bin";
    desc["bytes"] = bytes;
    state_["buffers"].push_back(std::move(desc));
}

void StateWriter::endFrame() {
    if (!in_frame_) return;
    const std::filesystem::path json_path = current_dir_ / "state.json";
    std::ofstream out(json_path);
    if (!out) {
        logError("state-writer: failed to open {}", json_path.string());
    } else {
        out << state_.dump(2);
    }
    in_frame_ = false;
}

}  // namespace gpusims
