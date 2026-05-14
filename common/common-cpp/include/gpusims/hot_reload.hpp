#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gpusims {

// File-watcher-based hot-reload with include-graph awareness, save-during-write
// retry, and graceful failure (keeps last good state on errors).
//
// Usage:
//     HotReloader hr;
//     hr.watch("shader.comp.glsl", [&](const auto& path){
//         try_recompile_and_swap(path);
//     });
//     while (running) {
//         hr.poll();   // call from main thread once per frame
//         ...
//     }
//
// Internally a worker thread polls last-write-times every ~100ms. On a change,
// it queues a callback that the main thread picks up via poll(). The callback
// is invoked synchronously inside poll() so it's safe to touch GPU resources.

class HotReloader {
public:
    using Callback = std::function<void(const std::filesystem::path&)>;

    explicit HotReloader(std::chrono::milliseconds poll_interval =
                             std::chrono::milliseconds(100));
    ~HotReloader();

    HotReloader(const HotReloader&)            = delete;
    HotReloader& operator=(const HotReloader&) = delete;

    // Register a file for watching. The callback fires whenever the file
    // (or any file it #includes, transitively) changes on disk.
    //
    // `cb` is invoked from the main thread inside poll().
    void watch(const std::filesystem::path& file, Callback cb);

    // Stop watching a previously-registered file.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void unwatch(const std::filesystem::path& file);

    // Process pending change events. Call once per frame from the main thread.
    void poll();

    // Number of files currently being watched (including transitive includes).
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    std::size_t watchCount() const;

    // Recently-fired events (for ImGui overlay). Each entry is a path + a
    // success/failure flag + an optional message. The renderer keeps these
    // alive for ~3 seconds after they fire.
    struct Event {
        std::filesystem::path path;
        bool                  ok;
        std::string           message;
        std::chrono::steady_clock::time_point t;
    };
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    std::vector<Event> recentEvents() const;

    // Called from the consumer's reload callback to record success/failure
    // for ImGui display.
    void reportSuccess(const std::filesystem::path& path);
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void reportFailure(const std::filesystem::path& path, std::string message);

private:
    struct WatchedFile {
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
        std::filesystem::path                  path;
        Callback                               callback;       // user callback
        std::filesystem::file_time_type        last_mtime;
        // Files included by this file (one level; resolved transitively).
        std::vector<std::filesystem::path>     includes;
        std::filesystem::file_time_type        last_include_check;
    };

    void                 workerLoop();
    void                 rescanIncludes(WatchedFile& wf);
    bool                 readMtimeWithRetry(const std::filesystem::path& p,
                                            std::filesystem::file_time_type& out);

    std::chrono::milliseconds                  poll_interval_;
    std::atomic<bool>                          running_{false};
    std::thread                                worker_;

    mutable std::mutex                         mu_;
    std::vector<WatchedFile>                   watched_;
    std::vector<std::filesystem::path>         pending_callbacks_;  // populated by worker, drained by poll()
    std::vector<Event>                         events_;             // for ImGui display
};

}  // namespace gpusims
