#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace gpusims::vk {

class Context;

// GLFW window + VkSurfaceKHR + VkSwapchainKHR.
//
// Recreates the swapchain transparently on resize / out-of-date conditions.

class Window {
public:
    Window(Context& ctx, std::uint32_t width, std::uint32_t height,
           const std::string& title);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    // Per-frame
    bool shouldClose() const;
    void pollEvents();
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void waitEvents();

    // Acquire the next swapchain image. Returns:
    //   - imageIndex on success
    //   - std::nullopt if swapchain is out-of-date and was recreated; caller
    //     should skip this frame and retry next iteration.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    std::optional<std::uint32_t> acquireNextImage(VkSemaphore image_available);

    // Present the rendered swapchain image. wait_semaphore signals when render
    // is complete. Returns false if the swapchain became out-of-date during
    // presentation; caller should recreate next frame.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    bool present(std::uint32_t image_index, VkSemaphore wait_semaphore);

    // Force-recreate the swapchain (e.g., after window-resize message).
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    void recreateSwapchain();

    // Handles
    GLFWwindow*       glfwWindow()   const { return window_; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    VkSurfaceKHR      surface()      const { return surface_; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    VkSwapchainKHR    swapchain()    const { return swapchain_; }
    VkFormat          colorFormat()  const { return color_format_; }
    VkExtent2D        extent()       const { return extent_; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    std::uint32_t     imageCount()   const { return static_cast<std::uint32_t>(images_.size()); }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    VkImage           image(std::uint32_t i)     const { return images_[i]; }
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    VkImageView       imageView(std::uint32_t i) const { return image_views_[i]; }

    // Aspect ratio for camera projection.
    float aspect() const {
        return extent_.height == 0 ? 1.0f
            : static_cast<float>(extent_.width) / static_cast<float>(extent_.height);
    }

private:
    void createSurface();
    void createSwapchain();
    void destroySwapchainResources();

    Context*                        ctx_      = nullptr;
    GLFWwindow*                     window_   = nullptr;
    VkSurfaceKHR                    surface_  = VK_NULL_HANDLE;
    VkSwapchainKHR                  swapchain_= VK_NULL_HANDLE;

    VkFormat                        color_format_ = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR                 color_space_  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR                present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D                      extent_{};

    std::vector<VkImage>            images_;
    std::vector<VkImageView>        image_views_;
};

}  // namespace gpusims::vk
