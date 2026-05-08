#include <gpusims/hot_reload.hpp>

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_set>

#include <gpusims/log.hpp>

namespace gpusims {

namespace {

const std::regex& includeRegex() {
    static const std::regex re(R"(^[ \t]*#[ \t]*include[ \t]+[\"<]([^\">]+)[\">])");
    return re;
}

std::vector<std::filesystem::path> scanIncludes(const std::filesystem::path& path) {
    std::vector<std::filesystem::path> result;
    std::ifstream in(path);
    if (!in) return result;
    const std::filesystem::path base = path.parent_path();
    std::string line;
    while (std::getline(in, line)) {
        std::smatch m;
        if (std::regex_search(line, m, includeRegex()) && m.size() >= 2) {
            const std::string included = m[1].str();
            std::filesystem::path resolved = base / included;
            std::error_code ec;
            if (std::filesystem::exists(resolved, ec)) {
                result.push_back(std::filesystem::weakly_canonical(resolved, ec));
            }
        }
    }
    return result;
}

}  // namespace

HotReloader::HotReloader(std::chrono::milliseconds poll_interval)
    : poll_interval_(poll_interval) {
    running_.store(true);
    worker_ = std::thread(&HotReloader::workerLoop, this);
}

HotReloader::~HotReloader() {
    running_.store(false);
    if (worker_.joinable()) worker_.join();
}

void HotReloader::watch(const std::filesystem::path& file, Callback cb) {
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(file, ec);
    WatchedFile wf;
    wf.path     = canonical.empty() ? file : canonical;
    wf.callback = std::move(cb);
    if (!readMtimeWithRetry(wf.path, wf.last_mtime)) {
        wf.last_mtime = std::filesystem::file_time_type::min();
    }
    rescanIncludes(wf);
    std::lock_guard<std::mutex> lock(mu_);
    watched_.push_back(std::move(wf));
}

void HotReloader::unwatch(const std::filesystem::path& file) {
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(file, ec);
    const std::filesystem::path& target = canonical.empty() ? file : canonical;
    std::lock_guard<std::mutex> lock(mu_);
    watched_.erase(std::remove_if(watched_.begin(), watched_.end(),
                                  [&](const WatchedFile& w) { return w.path == target; }),
                   watched_.end());
}

void HotReloader::poll() {
    std::vector<std::filesystem::path> to_fire;
    {
        std::lock_guard<std::mutex> lock(mu_);
        to_fire.swap(pending_callbacks_);
    }
    std::unordered_set<std::string> seen;
    for (const auto& path : to_fire) {
        const std::string key = path.string();
        if (!seen.insert(key).second) continue;
        Callback cb;
        std::filesystem::path fire_path = path;
        {
            std::lock_guard<std::mutex> lock(mu_);
            for (const auto& w : watched_) {
                if (w.path == path) {
                    cb = w.callback;
                    fire_path = w.path;
                    break;
                }
                if (std::find(w.includes.begin(), w.includes.end(), path) != w.includes.end()) {
                    if (w.callback) w.callback(w.path);
                }
            }
        }
        if (cb) cb(fire_path);
    }
    {
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mu_);
        events_.erase(std::remove_if(events_.begin(), events_.end(),
                                     [&](const Event& e) {
                                         return now - e.t > std::chrono::seconds(3);
                                     }),
                      events_.end());
    }
}

std::size_t HotReloader::watchCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::size_t n = watched_.size();
    for (const auto& w : watched_) n += w.includes.size();
    return n;
}

std::vector<HotReloader::Event> HotReloader::recentEvents() const {
    std::lock_guard<std::mutex> lock(mu_);
    return events_;
}

void HotReloader::reportSuccess(const std::filesystem::path& path) {
    Event e{path, true, {}, std::chrono::steady_clock::now()};
    std::lock_guard<std::mutex> lock(mu_);
    events_.push_back(std::move(e));
}

void HotReloader::reportFailure(const std::filesystem::path& path, std::string message) {
    Event e{path, false, std::move(message), std::chrono::steady_clock::now()};
    std::lock_guard<std::mutex> lock(mu_);
    events_.push_back(std::move(e));
}

void HotReloader::workerLoop() {
    while (running_.load()) {
        std::this_thread::sleep_for(poll_interval_);
        if (!running_.load()) break;
        std::vector<WatchedFile> snapshot;
        {
            std::lock_guard<std::mutex> lock(mu_);
            snapshot = watched_;
        }
        std::vector<std::filesystem::path> changed;
        for (auto& wf : snapshot) {
            std::filesystem::file_time_type t;
            if (readMtimeWithRetry(wf.path, t) && t != wf.last_mtime) {
                wf.last_mtime = t;
                rescanIncludes(wf);
                changed.push_back(wf.path);
                continue;
            }
            for (const auto& inc : wf.includes) {
                std::filesystem::file_time_type ti;
                if (readMtimeWithRetry(inc, ti) && ti > wf.last_mtime) {
                    wf.last_mtime = ti;
                    changed.push_back(inc);
                }
            }
        }
        if (!changed.empty()) {
            std::lock_guard<std::mutex> lock(mu_);
            for (auto& target : watched_) {
                for (const auto& src : snapshot) {
                    if (target.path == src.path) {
                        target.last_mtime = src.last_mtime;
                        target.includes   = src.includes;
                        break;
                    }
                }
            }
            for (auto& p : changed) pending_callbacks_.push_back(std::move(p));
        }
    }
}

void HotReloader::rescanIncludes(WatchedFile& wf) {
    wf.includes = scanIncludes(wf.path);
    wf.last_include_check = wf.last_mtime;
}

bool HotReloader::readMtimeWithRetry(const std::filesystem::path& p,
                                     std::filesystem::file_time_type& out) {
    for (int attempt = 0; attempt < 5; ++attempt) {
        std::error_code ec;
        auto t = std::filesystem::last_write_time(p, ec);
        if (!ec) {
            out = t;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}

}  // namespace gpusims
