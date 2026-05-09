#pragma once

#include <cstdint>
#include <functional>
#include <queue>

#include <vulkan/vulkan.h>

namespace gpusims::vk {

class Context;

// Per-in-flight-frame state. Owned by Renderer; passed by reference to
// per-sim render code so it can append commands and queue deletions.
struct Frame {
    // Indices
    std::uint32_t   in_flight_index = 0;   // 0..kMaxFramesInFlight-1
    std::uint32_t   swapchain_index = 0;   // index into Window's image array

    // Synchronization
    VkFence         fence              = VK_NULL_HANDLE;  // signaled when this frame's GPU work is done
    VkSemaphore     image_available    = VK_NULL_HANDLE;  // signaled by acquire
    VkSemaphore     render_finished    = VK_NULL_HANDLE;  // signaled when this frame's commands are done

    // Command buffer for this frame.
    VkCommandPool   command_pool   = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;

    // Deletion queue: callbacks executed when this frame's fence next signals.
    // Used by hot-reload to defer destruction of replaced pipelines/shaders.
    std::queue<std::function<void()>> deletion_queue;

    // Process the deletion queue. Called by Renderer at frame begin once the
    // fence has been waited on.
    void flushDeletions() {
        while (!deletion_queue.empty()) {
            deletion_queue.front()();
            deletion_queue.pop();
        }
    }
};

// Allocate a Frame's resources. Called by Renderer once per in-flight slot.
void initFrame(Context& ctx, Frame& frame, std::uint32_t in_flight_index);

// Free a Frame's resources. Called by Renderer at shutdown.
void destroyFrame(Context& ctx, Frame& frame);

// Issue a single global VkMemoryBarrier2 via vkCmdPipelineBarrier2. Used by
// per-sim code at hazard sites between compute writes and subsequent shader
// reads (which common-cpp does not auto-insert). Global rather than per-image
// is correct when all of the application's resources move together; the
// over-broad scope costs nothing in practice for typical per-sim workloads.
void memoryBarrier(VkCommandBuffer       cmd,
                   VkPipelineStageFlags2 src_stage,
                   VkAccessFlags2        src_access,
                   VkPipelineStageFlags2 dst_stage,
                   VkAccessFlags2        dst_access);

}  // namespace gpusims::vk
