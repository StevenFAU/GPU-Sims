#pragma once

#include <cstdint>
#include <optional>

#include <vulkan/vulkan.h>

#include <gpusims/gpu_profiler.hpp>  // for kMaxFramesInFlight
#include <gpusims/vk/frame.hpp>

namespace gpusims::vk {

class Context;
class Window;

// Top-level frame orchestration.
//
// Each frame is exactly:
//     auto frame = renderer.beginFrame();
//     if (!frame) continue;          // swapchain out of date; skip
//     // record commands into frame->command_buffer ...
//     renderer.beginRendering(*frame);   // begin dynamic rendering pass
//     // draw ...
//     renderer.endRendering(*frame);
//     renderer.endFrame(*frame);
//
// beginRendering / endRendering are convenience wrappers around
// vkCmdBeginRenderingKHR / vkCmdEndRenderingKHR with the swapchain image
// as the color attachment.

class Renderer {
public:
    Renderer(Context& ctx, Window& window);
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Begin a frame. Acquires the next swapchain image, resets the command
    // buffer, runs the deletion queue. Returns nullopt on swapchain
    // out-of-date; caller skips this iteration.
    Frame* beginFrame();

    // Begin / end a dynamic rendering pass that writes to the current
    // swapchain image. Caller may issue draws between these calls.
    void beginRendering(Frame& frame, VkClearColorValue clear = {{0.05f, 0.05f, 0.07f, 1.0f}});
    void endRendering(Frame& frame);

    // Finish the frame: end command buffer, submit, present.
    void endFrame(Frame& frame);

    // Wait for all in-flight frames to complete. Use at shutdown.
    void waitIdle();

    // Accessors
    Context& ctx()    const { return *ctx_; }
    Window&  window() const { return *window_; }

    // Frames-in-flight slots (kMaxFramesInFlight of them).
    Frame& frame(std::uint32_t i) { return frames_[i]; }
    std::uint32_t framesInFlight() const { return kMaxFramesInFlight; }

private:
    Context*      ctx_      = nullptr;
    Window*       window_   = nullptr;
    Frame         frames_[kMaxFramesInFlight];
    std::uint32_t current_frame_ = 0;
};

}  // namespace gpusims::vk
