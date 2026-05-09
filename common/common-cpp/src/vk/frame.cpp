#include <gpusims/vk/frame.hpp>

#include <stdexcept>

#include <gpusims/vk/context.hpp>

namespace gpusims::vk {

void initFrame(Context& ctx, Frame& frame, std::uint32_t in_flight_index) {
    frame.in_flight_index = in_flight_index;

    // Per-frame command pool. RESET_COMMAND_BUFFER_BIT lets us reset the
    // single command buffer at frame start without freeing/reallocating.
    VkCommandPoolCreateInfo pi{};
    pi.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pi.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pi.queueFamilyIndex = ctx.graphicsQueueFamily();
    if (vkCreateCommandPool(ctx.device(), &pi, nullptr, &frame.command_pool) != VK_SUCCESS) {
        throw std::runtime_error("Frame: vkCreateCommandPool failed");
    }

    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = frame.command_pool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(ctx.device(), &ai, &frame.command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("Frame: vkAllocateCommandBuffers failed");
    }

    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // first beginFrame doesn't block
    if (vkCreateFence(ctx.device(), &fi, nullptr, &frame.fence) != VK_SUCCESS) {
        throw std::runtime_error("Frame: vkCreateFence failed");
    }

    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(ctx.device(), &si, nullptr, &frame.image_available) != VK_SUCCESS ||
        vkCreateSemaphore(ctx.device(), &si, nullptr, &frame.render_finished) != VK_SUCCESS) {
        throw std::runtime_error("Frame: vkCreateSemaphore failed");
    }
}

void destroyFrame(Context& ctx, Frame& frame) {
    frame.flushDeletions();
    if (frame.image_available != VK_NULL_HANDLE) {
        vkDestroySemaphore(ctx.device(), frame.image_available, nullptr);
        frame.image_available = VK_NULL_HANDLE;
    }
    if (frame.render_finished != VK_NULL_HANDLE) {
        vkDestroySemaphore(ctx.device(), frame.render_finished, nullptr);
        frame.render_finished = VK_NULL_HANDLE;
    }
    if (frame.fence != VK_NULL_HANDLE) {
        vkDestroyFence(ctx.device(), frame.fence, nullptr);
        frame.fence = VK_NULL_HANDLE;
    }
    if (frame.command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(ctx.device(), frame.command_pool, nullptr);
        frame.command_pool   = VK_NULL_HANDLE;
        frame.command_buffer = VK_NULL_HANDLE;
    }
}

void memoryBarrier(VkCommandBuffer       cmd,
                   VkPipelineStageFlags2 src_stage,
                   VkAccessFlags2        src_access,
                   VkPipelineStageFlags2 dst_stage,
                   VkAccessFlags2        dst_access) {
    VkMemoryBarrier2 mb{};
    mb.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    mb.srcStageMask  = src_stage;
    mb.srcAccessMask = src_access;
    mb.dstStageMask  = dst_stage;
    mb.dstAccessMask = dst_access;

    VkDependencyInfo di{};
    di.sType                = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    di.memoryBarrierCount   = 1;
    di.pMemoryBarriers      = &mb;
    vkCmdPipelineBarrier2(cmd, &di);
}

}  // namespace gpusims::vk
