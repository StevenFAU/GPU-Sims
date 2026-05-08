#include <gpusims/vk/renderer.hpp>

#include <stdexcept>

#include <gpusims/log.hpp>
#include <gpusims/vk/context.hpp>
#include <gpusims/vk/image.hpp>
#include <gpusims/vk/window.hpp>

namespace gpusims::vk {

Renderer::Renderer(Context& ctx, Window& window) : ctx_(&ctx), window_(&window) {
    for (std::uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        initFrame(ctx, frames_[i], i);
    }
}

Renderer::~Renderer() {
    waitIdle();
    for (auto& f : frames_) destroyFrame(*ctx_, f);
}

Frame* Renderer::beginFrame() {
    Frame& f = frames_[current_frame_];

    // Wait for this slot's previous submission to finish (so we can reuse
    // command pool, sync prims, and run deletion queue).
    vkWaitForFences(ctx_->device(), 1, &f.fence, VK_TRUE, UINT64_MAX);
    f.flushDeletions();

    auto image_index = window_->acquireNextImage(f.image_available);
    if (!image_index) return nullptr;  // swapchain recreated; skip frame

    f.swapchain_index = *image_index;

    vkResetFences(ctx_->device(), 1, &f.fence);
    vkResetCommandPool(ctx_->device(), f.command_pool, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(f.command_buffer, &bi) != VK_SUCCESS) {
        logError("Renderer: vkBeginCommandBuffer failed");
        return nullptr;
    }
    return &f;
}

void Renderer::beginRendering(Frame& frame, VkClearColorValue clear) {
    // Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL.
    Image::transitionLayout(frame.command_buffer,
                            window_->image(frame.swapchain_index),
                            VK_IMAGE_ASPECT_COLOR_BIT,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfoKHR color{};
    color.sType         = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color.imageView     = window_->imageView(frame.swapchain_index);
    color.imageLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
    color.clearValue.color = clear;

    VkRenderingInfoKHR ri{};
    ri.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    ri.renderArea.offset    = {0, 0};
    ri.renderArea.extent    = window_->extent();
    ri.layerCount           = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments    = &color;

    vkCmdBeginRendering(frame.command_buffer, &ri);

    VkViewport vp{};
    vp.x        = 0.0f;
    vp.y        = 0.0f;
    vp.width    = static_cast<float>(window_->extent().width);
    vp.height   = static_cast<float>(window_->extent().height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport(frame.command_buffer, 0, 1, &vp);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = window_->extent();
    vkCmdSetScissor(frame.command_buffer, 0, 1, &scissor);
}

void Renderer::endRendering(Frame& frame) {
    vkCmdEndRendering(frame.command_buffer);
}

void Renderer::endFrame(Frame& frame) {
    // Transition for present.
    Image::transitionLayout(frame.command_buffer,
                            window_->image(frame.swapchain_index),
                            VK_IMAGE_ASPECT_COLOR_BIT,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    if (vkEndCommandBuffer(frame.command_buffer) != VK_SUCCESS) {
        logError("Renderer: vkEndCommandBuffer failed");
        return;
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = &frame.image_available;
    si.pWaitDstStageMask    = &wait_stage;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &frame.command_buffer;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = &frame.render_finished;
    if (vkQueueSubmit(ctx_->graphicsQueue(), 1, &si, frame.fence) != VK_SUCCESS) {
        logError("Renderer: vkQueueSubmit failed");
        return;
    }

    window_->present(frame.swapchain_index, frame.render_finished);

    current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
}

void Renderer::waitIdle() {
    if (ctx_) ctx_->waitIdle();
}

}  // namespace gpusims::vk
