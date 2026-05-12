#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

// Forward-declare VMA allocator handle so consumers don't need vma headers.
typedef struct VmaAllocator_T* VmaAllocator;

namespace gpusims::vk {

// Top-level Vulkan state shared by every subsystem in a sim:
//   - VkInstance + debug messenger (validation in Debug)
//   - VkPhysicalDevice (chosen via score-based heuristic)
//   - VkDevice + queues (graphics + compute, may be the same family)
//   - VmaAllocator
//
// Construction is zero-config for hello-world and most sims; advanced sims
// can request additional device extensions or features via ContextCreateInfo.

struct ContextCreateInfo {
    std::string                application_name = "gpu_sims";
    std::uint32_t              application_version = VK_MAKE_API_VERSION(0, 0, 1, 0);

    // Additional instance extensions to enable beyond what GLFW + debug needs.
    std::vector<const char*>   extra_instance_extensions;

    // Additional device extensions to enable beyond the defaults.
    std::vector<const char*>   extra_device_extensions;

    // Set true to require a discrete GPU. Default false (any conformant GPU
    // is acceptable; integrated is fine for hello-world on AMD).
    bool                       require_discrete_gpu = false;

    // Enable VK_KHR_swapchain. true unless this is a headless render-only sim.
    bool                       enable_swapchain = true;
};

class Context {
public:
    Context();
    explicit Context(const ContextCreateInfo& info);
    ~Context();

    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

    // ----------------------------------------------------------------------
    // Handles
    // ----------------------------------------------------------------------
    VkInstance       instance()        const { return instance_; }
    VkPhysicalDevice physicalDevice()  const { return physical_device_; }
    VkDevice         device()          const { return device_; }
    VkQueue          graphicsQueue()   const { return graphics_queue_; }
    VkQueue          computeQueue()    const { return compute_queue_; }
    std::uint32_t    graphicsQueueFamily() const { return graphics_queue_family_; }
    std::uint32_t    computeQueueFamily()  const { return compute_queue_family_; }
    VmaAllocator     allocator()       const { return allocator_; }

    // Properties of the chosen physical device, available for sims that
    // want to scale parameters by hardware capability.
    const VkPhysicalDeviceProperties&    deviceProperties()    const { return props_; }
    const VkPhysicalDeviceMemoryProperties& memoryProperties() const { return mem_props_; }

    // Wait until all work submitted to all queues is complete. Use sparingly
    // (mostly at shutdown or before reloading large resources).
    void waitIdle() const;

    // Convenience: allocate a transient command buffer, run callback, submit,
    // wait, and free. Used for one-shot setup work (image transitions, etc.).
    void runOneShot(const std::function<void(VkCommandBuffer)>& fn);

private:
    void createInstance(const ContextCreateInfo& info);
    void initDebugMessenger();
    void pickPhysicalDevice(const ContextCreateInfo& info);
    void createDevice(const ContextCreateInfo& info);
    void createAllocator();
    void createOneShotPool();

    VkInstance                       instance_         = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT         debug_messenger_  = VK_NULL_HANDLE;
    VkPhysicalDevice                 physical_device_  = VK_NULL_HANDLE;
    VkDevice                         device_           = VK_NULL_HANDLE;

    std::uint32_t                    graphics_queue_family_ = ~0u;
    std::uint32_t                    compute_queue_family_  = ~0u;
    VkQueue                          graphics_queue_   = VK_NULL_HANDLE;
    VkQueue                          compute_queue_    = VK_NULL_HANDLE;

    VmaAllocator                     allocator_        = VK_NULL_HANDLE;

    VkCommandPool                    one_shot_pool_    = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties             props_{};
    VkPhysicalDeviceMemoryProperties       mem_props_{};
};

}  // namespace gpusims::vk
