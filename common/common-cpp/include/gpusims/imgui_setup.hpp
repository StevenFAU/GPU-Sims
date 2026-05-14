#pragma once

// Helpers for wiring Dear ImGui (docking branch) to GLFW + Vulkan.
//
// The standard ImGui Vulkan recipe is many lines of careful initialization;
// this collapses it to two calls per consumer.
//
//     gpusims::ui::initImGui(window, ctx, renderer);  // once at startup
//     // per-frame: ImGui_ImplVulkan_NewFrame(); ImGui_ImplGlfw_NewFrame();
//     //            ImGui::NewFrame(); ... ImGui::Render();
//     //            gpusims::ui::renderImGui(cmd);
//     gpusims::ui::shutdownImGui();   // once at shutdown

#include <cstdint>

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace gpusims {

namespace vk {
class Context;
class Window;
class Renderer;
}  // namespace vk

namespace ui {

struct ImGuiInit {
    GLFWwindow*       glfw_window;
    VkInstance        instance;
    VkPhysicalDevice  physical_device;
    VkDevice          device;
    std::uint32_t     queue_family;
    VkQueue           queue;
    VkDescriptorPool  descriptor_pool;   // common-cpp creates and owns this
    VkFormat          color_format;      // swapchain format
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    std::uint32_t     min_image_count = 2;
    std::uint32_t     image_count     = 2;
};

// Initialize ImGui with GLFW + Vulkan backends. Allocates a small
// descriptor pool internally if you don't pass one (descriptor_pool == VK_NULL_HANDLE).
// Returns false on failure.
bool initImGui(const ImGuiInit& cfg);

// Begin a new ImGui frame. Call after Window::pollEvents and before any
// ImGui::Begin / ImGui::End.
void newImGuiFrame();

// Record ImGui's draw data into the given command buffer. Call inside an
// active dynamic-rendering pass (or render pass).
void renderImGui(VkCommandBuffer cmd);

// Shut down ImGui. Call once before destroying the Vulkan device.
void shutdownImGui();

// Convenience: create + own a small descriptor pool sized for ImGui's needs.
// Returned pool must be destroyed by the caller via vkDestroyDescriptorPool.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
VkDescriptorPool createImGuiDescriptorPool(VkDevice device);

// Top-right toast list, used by the renderer to show hot-reload events.
// Each call adds one toast that fades out over `lifetime_seconds`.
void pushToast(const char* text, bool success, float lifetime_seconds = 3.0f);

// Render any pending toasts at top-right. Call from inside an ImGui frame.
void drawToasts();

}  // namespace ui
}  // namespace gpusims
