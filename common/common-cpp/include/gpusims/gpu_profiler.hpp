#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace gpusims {

// Frames-in-flight count. Must match the renderer's count exactly.
inline constexpr std::uint32_t kMaxFramesInFlight = 2;

namespace vk {
class Context;
}  // namespace vk

// GPU profiler with RAII scopes, ring-buffered timestamp queries, ImGui
// overlay, and CSV dump.
//
// Usage:
//     GpuProfiler profiler(ctx);
//     while (running) {
//         profiler.beginFrame(cmd, frame_idx);
//         {
//             auto _ = profiler.scope(cmd, "advect");
//             // dispatch ...
//         }
//         {
//             auto _ = profiler.scope(cmd, "render");
//             // draws ...
//         }
//         profiler.endFrame(cmd);
//     }
//     profiler.drawImGui();    // call once per frame outside the cmd buffer
//
// GPU times lag by `kMaxFramesInFlight` frames because we never read query
// results before the GPU has finished writing them; the lag is invisible at
// 60fps and the alternative (pipeline stall) is unacceptable.

class GpuProfiler {
public:
    static constexpr std::uint32_t kMaxPasses = 64;

    explicit GpuProfiler(vk::Context& ctx);
    ~GpuProfiler();

    GpuProfiler(const GpuProfiler&)            = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;

    // Per-frame
    void beginFrame(VkCommandBuffer cmd, std::uint32_t frame_in_flight_idx);
    void endFrame(VkCommandBuffer cmd);

    // RAII scope for a named pass.
    class Scope {
    public:
        Scope(GpuProfiler* p, VkCommandBuffer cmd, const char* name);
        ~Scope();
        Scope(const Scope&)            = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        GpuProfiler*    p_;
        VkCommandBuffer cmd_;
        std::uint32_t   slot_;
    };

    [[nodiscard]] Scope scope(VkCommandBuffer cmd, const char* name) {
        return Scope(this, cmd, name);
    }

    // Per-pass result (read from queries `kMaxFramesInFlight` frames ago).
    struct PassResult {
        std::string name;
        double      cpu_ms = 0.0;
        double      gpu_ms = 0.0;
    };

    // Most recent results read from the GPU. Updated by beginFrame.
    [[nodiscard]] const std::vector<PassResult>& lastResults() const { return last_results_; }

    // Render an ImGui collapsible window with timing bars.
    void drawImGui(const char* label = "GPU Profiler");

    // Append the most recent frame's results to a CSV file. Header is
    // written on first call; subsequent calls append rows.
    void appendCsv(const std::filesystem::path& path);

private:
    // Begin/end a single pass. Called by Scope.
    std::uint32_t beginPass(VkCommandBuffer cmd, const char* name);
    void          endPass(VkCommandBuffer cmd, std::uint32_t slot);

    void readBackResults(std::uint32_t frame_in_flight_idx);

    vk::Context*        ctx_ = nullptr;
    VkQueryPool         pools_[kMaxFramesInFlight]{};

    // CPU-side bookkeeping per frame in flight.
    struct FrameData {
        std::array<std::string, kMaxPasses>                                    pass_names;
        std::array<std::chrono::steady_clock::time_point, kMaxPasses>          cpu_begin;
        std::array<std::chrono::steady_clock::time_point, kMaxPasses>          cpu_end;
        std::uint32_t                                                          pass_count = 0;
        bool                                                                   submitted  = false;
    };
    FrameData frames_[kMaxFramesInFlight];

    std::uint32_t            current_frame_idx_ = 0;
    std::vector<PassResult>  last_results_;
    bool                     csv_header_written_ = false;
    std::uint64_t            frame_counter_ = 0;
};

}  // namespace gpusims
