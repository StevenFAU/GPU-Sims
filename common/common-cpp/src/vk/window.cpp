#include <gpusims/vk/window.hpp>

#include <algorithm>
#include <stdexcept>
#include <vector>

#include <GLFW/glfw3.h>

#include <gpusims/log.hpp>
#include <gpusims/vk/context.hpp>

namespace gpusims::vk {

namespace {

VkSurfaceFormatKHR chooseSurfaceFormat(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    std::uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &count, formats.data());
    for (const auto& f : formats) {
        if ((f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }
    return formats[0];
}

VkPresentModeKHR choosePresentMode(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    std::uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &count, modes.data());
    VkPresentModeKHR chosen = VK_PRESENT_MODE_FIFO_KHR;  // guaranteed available, vsync'd
    for (auto m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) { chosen = m; break; }
    }
    if (chosen == VK_PRESENT_MODE_FIFO_KHR) {
        for (auto m : modes) {
            if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) { chosen = m; break; }
        }
    }
    logInfo("vk-window: present mode = {}", static_cast<int>(chosen));
    return chosen;
}

VkExtent2D chooseExtent(GLFWwindow* w, const VkSurfaceCapabilitiesKHR& caps) {
    if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
    int width = 0, height = 0;
    glfwGetFramebufferSize(w, &width, &height);
    VkExtent2D actual{ static_cast<std::uint32_t>(width),
                       static_cast<std::uint32_t>(height) };
    actual.width  = std::clamp(actual.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
    actual.height = std::clamp(actual.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return actual;
}

}  // namespace

Window::Window(Context& ctx, std::uint32_t width, std::uint32_t height,
               const std::string& title)
    : ctx_(&ctx) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // Vulkan, not OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                               title.c_str(), nullptr, nullptr);
    if (!window_) {
        throw std::runtime_error("Window: glfwCreateWindow failed");
    }
    createSurface();
    createSwapchain();
}

Window::~Window() {
    destroySwapchainResources();
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(ctx_->instance(), surface_, nullptr);
    }
    if (window_) glfwDestroyWindow(window_);
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_) != 0; }
void Window::pollEvents()        { glfwPollEvents(); }
void Window::waitEvents()        { glfwWaitEvents(); }

void Window::createSurface() {
    if (glfwCreateWindowSurface(ctx_->instance(), window_, nullptr, &surface_) != VK_SUCCESS) {
        throw std::runtime_error("Window: glfwCreateWindowSurface failed");
    }
}

void Window::createSwapchain() {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx_->physicalDevice(), surface_, &caps);

    auto format = chooseSurfaceFormat(ctx_->physicalDevice(), surface_);
    color_format_  = format.format;
    color_space_   = format.colorSpace;
    present_mode_  = choosePresentMode(ctx_->physicalDevice(), surface_);
    extent_        = chooseExtent(window_, caps);

    // +2 over min so at least one swapchain image is free to acquire while
    // kMaxFramesInFlight frames are queued; otherwise vkAcquireNextImageKHR
    // blocks the main thread, stalling glfwPollEvents and freezing the cursor.
    std::uint32_t image_count = caps.minImageCount + 2;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR ci{};
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = surface_;
    ci.minImageCount    = image_count;
    ci.imageFormat      = color_format_;
    ci.imageColorSpace  = color_space_;
    ci.imageExtent      = extent_;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform     = caps.currentTransform;
    ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode      = present_mode_;
    ci.clipped          = VK_TRUE;
    ci.oldSwapchain     = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(ctx_->device(), &ci, nullptr, &swapchain_) != VK_SUCCESS) {
        throw std::runtime_error("Window: vkCreateSwapchainKHR failed");
    }

    std::uint32_t got = 0;
    vkGetSwapchainImagesKHR(ctx_->device(), swapchain_, &got, nullptr);
    images_.resize(got);
    vkGetSwapchainImagesKHR(ctx_->device(), swapchain_, &got, images_.data());

    image_views_.resize(got);
    for (std::uint32_t i = 0; i < got; ++i) {
        VkImageViewCreateInfo vci{};
        vci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image    = images_[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format   = color_format_;
        vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.baseMipLevel   = 0;
        vci.subresourceRange.levelCount     = 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount     = 1;
        if (vkCreateImageView(ctx_->device(), &vci, nullptr, &image_views_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Window: vkCreateImageView failed");
        }
    }
}

void Window::destroySwapchainResources() {
    for (auto v : image_views_) {
        if (v != VK_NULL_HANDLE) vkDestroyImageView(ctx_->device(), v, nullptr);
    }
    image_views_.clear();
    images_.clear();
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ctx_->device(), swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

void Window::recreateSwapchain() {
    int w = 0, h = 0;
    glfwGetFramebufferSize(window_, &w, &h);
    while (w == 0 || h == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window_, &w, &h);
    }
    ctx_->waitIdle();
    destroySwapchainResources();
    createSwapchain();
}

std::optional<std::uint32_t> Window::acquireNextImage(VkSemaphore image_available) {
    // 16ms ≈ 1 frame at 60Hz. If swapchain is busy, return null and let the
    // caller skip this frame so the main thread keeps servicing GLFW input.
    constexpr std::uint64_t kAcquireTimeoutNs = 16'000'000;
    std::uint32_t idx = 0;
    VkResult r = vkAcquireNextImageKHR(ctx_->device(), swapchain_, kAcquireTimeoutNs,
                                       image_available, VK_NULL_HANDLE, &idx);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return std::nullopt;
    }
    if (r == VK_TIMEOUT || r == VK_NOT_READY) {
        return std::nullopt;
    }
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        logError("Window: vkAcquireNextImageKHR returned {}", static_cast<int>(r));
        return std::nullopt;
    }
    return idx;
}

bool Window::present(std::uint32_t image_index, VkSemaphore wait_semaphore) {
    VkPresentInfoKHR pi{};
    pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &wait_semaphore;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &swapchain_;
    pi.pImageIndices      = &image_index;
    VkResult r = vkQueuePresentKHR(ctx_->graphicsQueue(), &pi);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
        return false;
    }
    if (r != VK_SUCCESS) {
        logError("Window: vkQueuePresentKHR returned {}", static_cast<int>(r));
        return false;
    }
    return true;
}

}  // namespace gpusims::vk
