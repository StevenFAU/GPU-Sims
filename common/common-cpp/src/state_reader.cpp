#include <gpusims/state_reader.hpp>

#include <algorithm>
#include <fstream>
#include <system_error>
#include <vector>

#include <gpusims/log.hpp>

namespace gpusims {

std::optional<std::filesystem::path>
StateReader::findLatest(const std::filesystem::path& root) {
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) {
        return std::nullopt;
    }
    std::vector<std::filesystem::path> matches;
    for (const auto& e : std::filesystem::directory_iterator(root, ec)) {
        if (ec) break;
        if (!e.is_directory()) continue;
        const auto name = e.path().filename().string();
        if (name.rfind("capture_", 0) != 0) continue;
        matches.push_back(e.path());
    }
    if (matches.empty()) return std::nullopt;
    std::sort(matches.begin(), matches.end());
    return matches.back();
}

std::optional<StateReader> StateReader::open(const std::filesystem::path& capture_dir) {
    StateReader r;
    r.dir_ = capture_dir;
    const std::filesystem::path json_path = capture_dir / "state.json";
    std::ifstream in(json_path);
    if (!in) {
        logError("state-reader: cannot open {}", json_path.string());
        return std::nullopt;
    }
    try {
        in >> r.state_;
    } catch (const std::exception& e) {
        logError("state-reader: invalid JSON at {}: {}", json_path.string(), e.what());
        return std::nullopt;
    }
    r.frame_idx_ = r.state_.value("frame", 0u);
    return r;
}

nlohmann::json StateReader::meta(const std::string& key) const {
    if (!state_.contains("meta")) return nlohmann::json{};
    const auto& m = state_["meta"];
    if (!m.contains(key)) return nlohmann::json{};
    return m[key];
}

nlohmann::json StateReader::bufferMeta(const std::string& name) const {
    if (!state_.contains("buffers")) return nlohmann::json{};
    for (const auto& b : state_["buffers"]) {
        if (b.value("name", std::string{}) == name) return b;
    }
    return nlohmann::json{};
}

std::vector<std::uint8_t> StateReader::buffer(const std::string& name) const {
    auto desc = bufferMeta(name);
    if (desc.is_null()) return {};
    const std::string filename = desc.value("file", name + ".bin");
    const std::filesystem::path bin = dir_ / filename;
    std::ifstream in(bin, std::ios::binary | std::ios::ate);
    if (!in) {
        logError("state-reader: cannot open {}", bin.string());
        return {};
    }
    const std::streamsize size = in.tellg();
    in.seekg(0);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!in.read(reinterpret_cast<char*>(bytes.data()), size)) {
        logError("state-reader: short read from {}", bin.string());
        bytes.clear();
    }
    return bytes;
}

}  // namespace gpusims
