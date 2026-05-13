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

    // Phase 11 (sph-water) consumer #1 of subgroup-size-control.
    // When true: enables VkPhysicalDeviceVulkan13Features::subgroupSizeControl
    // and queries VkPhysicalDeviceSubgroupSizeControlProperties at device-create
    // time. Throws if the device does not support the feature — fail-loud rather
    // than silently producing platform-dependent wavefront-size behavior.
    bool                       enable_subgroup_size_control = false;
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

    // Subgroup-size-control properties.
    //
    // Populated at device-create time when ContextCreateInfo::
    // enable_subgroup_size_control was true. When the feature was NOT requested
    // these return 0 / 0 / 0 / false — consult them only when the feature was
    // requested.
    //
    // Values originate from VkPhysicalDeviceSubgroupSizeControlProperties
    // queried via vkGetPhysicalDeviceProperties2 during Context construction.
    std::uint32_t subgroupSizeMin()              const { return subgroup_size_min_; }
    std::uint32_t subgroupSizeMax()              const { return subgroup_size_max_; }
    std::uint32_t requiredSubgroupSizeStages()   const { return required_subgroup_size_stages_; }
    bool          subgroupSizeControlEnabled()   const { return subgroup_size_control_enabled_; }

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

    // Cached subgroup-size-control properties (populated when
    // ContextCreateInfo::enable_subgroup_size_control was true; otherwise zeros).
    std::uint32_t subgroup_size_min_              = 0;
    std::uint32_t subgroup_size_max_              = 0;
    std::uint32_t required_subgroup_size_stages_  = 0;
    bool          subgroup_size_control_enabled_  = false;
};

}  // namespace gpusims::vk
