#include <gpusims/gpu_profiler.hpp>

#include <cstdio>
#include <fstream>

#include <imgui.h>

#include <gpusims/log.hpp>
#include <gpusims/vk/context.hpp>

namespace gpusims {

namespace {
constexpr std::uint32_t kQueriesPerFrame = 2 * GpuProfiler::kMaxPasses;
}

GpuProfiler::GpuProfiler(vk::Context& ctx) : ctx_(&ctx) {
    VkQueryPoolCreateInfo ci{};
    ci.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    ci.queryType  = VK_QUERY_TYPE_TIMESTAMP;
    ci.queryCount = kQueriesPerFrame;
    for (std::uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (vkCreateQueryPool(ctx.device(), &ci, nullptr, &pools_[i]) != VK_SUCCESS) {
            logError("gpu-profiler: vkCreateQueryPool failed for frame slot {}", i);
            pools_[i] = VK_NULL_HANDLE;
        }
    }
}

GpuProfiler::~GpuProfiler() {
    if (!ctx_) return;
    for (std::uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (pools_[i] != VK_NULL_HANDLE) {
            vkDestroyQueryPool(ctx_->device(), pools_[i], nullptr);
        }
    }
}

void GpuProfiler::beginFrame(VkCommandBuffer cmd, std::uint32_t frame_in_flight_idx) {
    current_frame_idx_ = frame_in_flight_idx;
    auto& f = frames_[current_frame_idx_];
    f.pass_count = 0;
    readBackResults(current_frame_idx_);
    if (pools_[current_frame_idx_] != VK_NULL_HANDLE) {
        vkCmdResetQueryPool(cmd, pools_[current_frame_idx_], 0, kQueriesPerFrame);
    }
    f.submitted = true;
    ++frame_counter_;
}

void GpuProfiler::endFrame(VkCommandBuffer /*cmd*/) {
    // Reads happen in next frame's beginFrame (delayed-read pattern).
}

std::uint32_t GpuProfiler::beginPass(VkCommandBuffer cmd, const char* name) {
    auto& f = frames_[current_frame_idx_];
    if (f.pass_count >= kMaxPasses) {
        logWarn("gpu-profiler: pass count exceeded; '{}' ignored", name);
        return UINT32_MAX;
    }
    const std::uint32_t slot = f.pass_count++;
    f.pass_names[slot] = name ? name : "";
    f.cpu_begin[slot]  = std::chrono::steady_clock::now();
    if (pools_[current_frame_idx_] != VK_NULL_HANDLE) {
        vkCmdWriteTimestamp2(cmd,
                             VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                             pools_[current_frame_idx_],
                             2 * slot + 0);
    }
    return slot;
}

void GpuProfiler::endPass(VkCommandBuffer cmd, std::uint32_t slot) {
    if (slot == UINT32_MAX) return;
    auto& f = frames_[current_frame_idx_];
    f.cpu_end[slot] = std::chrono::steady_clock::now();
    if (pools_[current_frame_idx_] != VK_NULL_HANDLE) {
        vkCmdWriteTimestamp2(cmd,
                             VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                             pools_[current_frame_idx_],
                             2 * slot + 1);
    }
}

GpuProfiler::Scope::Scope(GpuProfiler* p, VkCommandBuffer cmd, const char* name)
    : p_(p), cmd_(cmd), slot_(p->beginPass(cmd, name)) {}

GpuProfiler::Scope::~Scope() {
    p_->endPass(cmd_, slot_);
}

void GpuProfiler::readBackResults(std::uint32_t frame_in_flight_idx) {
    auto& f = frames_[frame_in_flight_idx];
    if (!f.submitted || f.pass_count == 0) {
        last_results_.clear();
        return;
    }
    const VkQueryPool pool = pools_[frame_in_flight_idx];
    if (pool == VK_NULL_HANDLE) {
        last_results_.clear();
        return;
    }
    std::vector<std::uint64_t> timestamps(2 * f.pass_count, 0);
    const VkResult r = vkGetQueryPoolResults(
        ctx_->device(), pool, 0,
        static_cast<std::uint32_t>(timestamps.size()),
        timestamps.size() * sizeof(std::uint64_t),
        timestamps.data(),
        sizeof(std::uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    if (r != VK_SUCCESS) {
        last_results_.clear();
        return;
    }
    const float ts_period_ns = ctx_->deviceProperties().limits.timestampPeriod;
    last_results_.clear();
    last_results_.reserve(f.pass_count);
    for (std::uint32_t i = 0; i < f.pass_count; ++i) {
        PassResult pr;
        pr.name = f.pass_names[i];
        const auto cpu_us = std::chrono::duration_cast<std::chrono::microseconds>(
            f.cpu_end[i] - f.cpu_begin[i]).count();
        pr.cpu_ms = cpu_us / 1000.0;
        const std::uint64_t t0 = timestamps[2 * i + 0];
        const std::uint64_t t1 = timestamps[2 * i + 1];
        if (t1 > t0) {
            const double ns = static_cast<double>(t1 - t0) * ts_period_ns;
            pr.gpu_ms = ns * 1.0e-6;
        }
        last_results_.push_back(std::move(pr));
    }
}

void GpuProfiler::drawImGui(const char* label) {
    ImGui::SetNextWindowPos(ImVec2(340.0f, 10.0f),    ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380.0f, 300.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(label)) {
        ImGui::End();
        return;
    }
    ImGui::Text("Frame %llu", static_cast<unsigned long long>(frame_counter_));
    ImGui::Separator();
    if (last_results_.empty()) {
        ImGui::TextDisabled("No results yet (warming up)...");
        ImGui::End();
        return;
    }
    double total_gpu = 0.0;
    for (const auto& r : last_results_) total_gpu += r.gpu_ms;
    if (total_gpu <= 0.0) total_gpu = 1.0;
    if (ImGui::BeginTable("##profiler", 4,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Pass");
        ImGui::TableSetupColumn("GPU (ms)");
        ImGui::TableSetupColumn("CPU (ms)");
        ImGui::TableSetupColumn("% GPU");
        ImGui::TableHeadersRow();
        for (const auto& r : last_results_) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(r.name.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%.3f", r.gpu_ms);
            ImGui::TableNextColumn(); ImGui::Text("%.3f", r.cpu_ms);
            ImGui::TableNextColumn();
            ImGui::ProgressBar(static_cast<float>(r.gpu_ms / total_gpu), ImVec2(-1.0f, 0.0f));
        }
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::Text("Total GPU: %.3f ms", total_gpu);
    ImGui::End();
}

void GpuProfiler::appendCsv(const std::filesystem::path& path) {
    if (last_results_.empty()) return;
    const bool need_header = !csv_header_written_;
    std::ofstream out(path, std::ios::app);
    if (!out) {
        logError("gpu-profiler: failed to open CSV at {}", path.string());
        return;
    }
    if (need_header) {
        out << "frame,pass,gpu_ms,cpu_ms\n";
        csv_header_written_ = true;
    }
    for (const auto& r : last_results_) {
        out << frame_counter_ << ',' << r.name << ',' << r.gpu_ms << ',' << r.cpu_ms << '\n';
    }
}

}  // namespace gpusims
