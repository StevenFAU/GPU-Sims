#include <gpusims/vk/context.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#include <GLFW/glfw3.h>

#include <gpusims/log.hpp>
#include <gpusims/vk/debug.hpp>

namespace gpusims::vk {

namespace {

bool extensionsSupported(VkPhysicalDevice dev,
                        const std::vector<const char*>& required) {
    std::uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, available.data());
    std::set<std::string> remaining(required.begin(), required.end());
    for (const auto& e : available) remaining.erase(e.extensionName);
    return remaining.empty();
}

struct QueueFamilies {
    std::uint32_t graphics = ~0u;
    std::uint32_t compute  = ~0u;
    bool complete() const { return graphics != ~0u && compute != ~0u; }
};

QueueFamilies findQueueFamilies(VkPhysicalDevice dev) {
    QueueFamilies q;
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, props.data());
    // Prefer a single family that supports both graphics + compute (typical on
    // every modern GPU). Fall back to separate families if needed.
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto& p = props[i];
        const bool gfx  = (p.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        const bool comp = (p.queueFlags & VK_QUEUE_COMPUTE_BIT)  != 0;
        if (gfx && comp) {
            q.graphics = i;
            q.compute  = i;
            return q;
        }
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && q.graphics == ~0u) q.graphics = i;
        if ((props[i].queueFlags & VK_QUEUE_COMPUTE_BIT)  && q.compute  == ~0u) q.compute  = i;
    }
    return q;
}

std::uint64_t scoreDevice(VkPhysicalDevice dev,
                          const std::vector<const char*>& required_exts,
                          bool require_discrete) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(dev, &props);

    if (props.apiVersion < VK_API_VERSION_1_3) return 0;
    if (!extensionsSupported(dev, required_exts)) return 0;
    if (!findQueueFamilies(dev).complete()) return 0;

    std::uint64_t score = 0;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)   score += 100000;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 10000;
    if (require_discrete && props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return 0;
    score += props.limits.maxComputeWorkGroupInvocations;
    return score;
}

std::vector<const char*> defaultDeviceExtensions(bool with_swapchain) {
    std::vector<const char*> e;
    if (with_swapchain) {
        e.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    // Vulkan 1.3 promotes synchronization2 and dynamic rendering, but on
    // some drivers the extension must still be enabled explicitly. Adding
    // them is harmless if they're already promoted.
    e.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    e.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    return e;
}

}  // namespace

Context::Context() : Context(ContextCreateInfo{}) {}

Context::Context(const ContextCreateInfo& info) {
    initLogger();
    // Force GLFW's X11 backend instead of Wayland. On Wayland + Vulkan, cursor
    // events round-trip through the compositor and stall glfwPollEvents for
    // 5-20ms per call when the cursor is over the window, freezing input.
    // X11 (via XWayland on Wayland sessions) doesn't have this issue.
#ifdef GLFW_PLATFORM
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    if (!glfwInit()) {
        throw std::runtime_error("Context: glfwInit failed");
    }
    if (!glfwVulkanSupported()) {
        throw std::runtime_error("Context: GLFW reports Vulkan unsupported on this system");
    }
    createInstance(info);
    initDebugMessenger();
    pickPhysicalDevice(info);
    createDevice(info);

    // Phase 11 sph-water: query VkPhysicalDeviceSubgroupSizeControlProperties
    // and cache results. Only fires when the consumer requested the feature
    // (and createDevice's pre-flight verified the device supports it);
    // otherwise the private cache members stay at their zero defaults.
    if (info.enable_subgroup_size_control) {
        VkPhysicalDeviceSubgroupSizeControlProperties sgs_props{};
        sgs_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES;
        VkPhysicalDeviceProperties2 props2{};
        props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        props2.pNext = &sgs_props;
        vkGetPhysicalDeviceProperties2(physical_device_, &props2);

        subgroup_size_min_              = sgs_props.minSubgroupSize;
        subgroup_size_max_              = sgs_props.maxSubgroupSize;
        required_subgroup_size_stages_  = sgs_props.requiredSubgroupSizeStages;
        subgroup_size_control_enabled_  = true;

        logInfo("vk-context: subgroup-size-control enabled (min={}, max={}, stages=0x{:x})",
                subgroup_size_min_, subgroup_size_max_, required_subgroup_size_stages_);
    }

    createAllocator();
    createOneShotPool();
    logInfo("vk-context: ready ({}, {} MiB VRAM heap0)",
            props_.deviceName,
            mem_props_.memoryHeapCount > 0
                ? static_cast<unsigned long long>(mem_props_.memoryHeaps[0].size / (1024ull * 1024ull))
                : 0ull);
}

Context::~Context() {
    if (one_shot_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, one_shot_pool_, nullptr);
    }
    if (allocator_ != VK_NULL_HANDLE) {
        vmaDestroyAllocator(allocator_);
    }
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
    }
    if (debug_messenger_ != VK_NULL_HANDLE) {
        destroyDebugMessenger(instance_, debug_messenger_);
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
    // Note: glfwTerminate() is called by Window destructor or main; we don't
    // call it here because multiple Contexts could be live. In practice only
    // one Context exists per process, but the symmetry is left to the app.
}

void Context::createInstance(const ContextCreateInfo& info) {
    VkApplicationInfo app{};
    app.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName   = info.application_name.c_str();
    app.applicationVersion = info.application_version;
    app.pEngineName        = "gpu_sims";
    app.engineVersion      = VK_MAKE_API_VERSION(0, 0, 1, 0);
    app.apiVersion         = VK_API_VERSION_1_3;

    // Required by GLFW for window-system integration.
    std::uint32_t glfw_count = 0;
    const char**  glfw_exts  = glfwGetRequiredInstanceExtensions(&glfw_count);
    std::vector<const char*> exts(glfw_exts, glfw_exts + glfw_count);

    // Debug extensions if validation enabled.
    std::uint32_t       dbg_count = 0;
    const char* const*  dbg_exts  = requiredDebugExtensions(&dbg_count);
    for (std::uint32_t i = 0; i < dbg_count; ++i) exts.push_back(dbg_exts[i]);

    for (auto e : info.extra_instance_extensions) exts.push_back(e);

    std::vector<const char*> layers;
    bool want_validation = false;
#if GPU_SIMS_VALIDATION_LAYERS
    if (checkValidationLayerSupport()) {
        layers.push_back(kValidationLayerName);
        want_validation = true;
    } else {
        logWarn("vk-context: validation requested but VK_LAYER_KHRONOS_validation not available");
    }
#endif

    VkInstanceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo        = &app;
    ci.enabledExtensionCount   = static_cast<std::uint32_t>(exts.size());
    ci.ppEnabledExtensionNames = exts.data();
    ci.enabledLayerCount       = static_cast<std::uint32_t>(layers.size());
    ci.ppEnabledLayerNames     = layers.data();

    // Chain a debug messenger creation info so messages from instance
    // creation/destruction itself are captured.
    VkDebugUtilsMessengerCreateInfoEXT dbg_ci{};
    if (want_validation) {
        populateDebugMessengerCreateInfo(dbg_ci);
        ci.pNext = &dbg_ci;
    }

    if (vkCreateInstance(&ci, nullptr, &instance_) != VK_SUCCESS) {
        throw std::runtime_error("Context: vkCreateInstance failed");
    }
}

void Context::initDebugMessenger() {
#if GPU_SIMS_VALIDATION_LAYERS
    if (!checkValidationLayerSupport()) return;
    VkDebugUtilsMessengerCreateInfoEXT ci{};
    populateDebugMessengerCreateInfo(ci);
    if (createDebugMessenger(instance_, ci, &debug_messenger_) != VK_SUCCESS) {
        logWarn("vk-context: createDebugMessenger failed; continuing without");
        debug_messenger_ = VK_NULL_HANDLE;
    }
#endif
}

void Context::pickPhysicalDevice(const ContextCreateInfo& info) {
    std::uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (count == 0) {
        throw std::runtime_error("Context: no Vulkan-capable GPUs found");
    }
    std::vector<VkPhysicalDevice> devs(count);
    vkEnumeratePhysicalDevices(instance_, &count, devs.data());

    auto required = defaultDeviceExtensions(info.enable_swapchain);
    for (auto e : info.extra_device_extensions) required.push_back(e);

    VkPhysicalDevice best  = VK_NULL_HANDLE;
    std::uint64_t    best_score = 0;
    for (auto d : devs) {
        std::uint64_t s = scoreDevice(d, required, info.require_discrete_gpu);
        if (s > best_score) {
            best_score = s;
            best       = d;
        }
    }
    if (best == VK_NULL_HANDLE) {
        throw std::runtime_error("Context: no Vulkan 1.3 GPU with required extensions found");
    }
    physical_device_ = best;
    vkGetPhysicalDeviceProperties(physical_device_,        &props_);
    vkGetPhysicalDeviceMemoryProperties(physical_device_,  &mem_props_);
}

void Context::createDevice(const ContextCreateInfo& info) {
    auto qf = findQueueFamilies(physical_device_);
    graphics_queue_family_ = qf.graphics;
    compute_queue_family_  = qf.compute;

    std::set<std::uint32_t> unique_families = {graphics_queue_family_, compute_queue_family_};
    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    const float qprio = 1.0f;
    for (auto fam : unique_families) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = fam;
        qi.queueCount       = 1;
        qi.pQueuePriorities = &qprio;
        queue_infos.push_back(qi);
    }

    auto exts = defaultDeviceExtensions(info.enable_swapchain);
    for (auto e : info.extra_device_extensions) exts.push_back(e);

    // Vulkan 1.3 features chain. We require dynamic rendering, sync2, and
    // timestamp queries on compute & graphics.
    VkPhysicalDeviceVulkan13Features f13{};
    f13.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    f13.dynamicRendering = VK_TRUE;
    f13.synchronization2 = VK_TRUE;

    // Phase 11 sph-water (consumer #1 of subgroup-size-control). When the
    // consumer requested the feature, pre-flight verify the device actually
    // supports it before setting the f13 flag — vkCreateDevice with an
    // unsupported feature returns a generic error, so doing the check here
    // lets us surface a descriptive fail-loud message instead. Silent fallback
    // would defeat the architectural point of consumer-#1 requesting
    // predictable wavefront-size.
    if (info.enable_subgroup_size_control) {
        VkPhysicalDeviceVulkan13Features actual_f13{};
        actual_f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        VkPhysicalDeviceFeatures2 actual_f2{};
        actual_f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        actual_f2.pNext = &actual_f13;
        vkGetPhysicalDeviceFeatures2(physical_device_, &actual_f2);

        if (!actual_f13.subgroupSizeControl) {
            throw std::runtime_error(
                "Context: ContextCreateInfo::enable_subgroup_size_control = true, "
                "but VkPhysicalDeviceVulkan13Features::subgroupSizeControl is not "
                "supported by this device. Cross-vendor wavefront-size "
                "predictability cannot be guaranteed without it; failing loud "
                "rather than silently producing platform-dependent behavior.");
        }
        f13.subgroupSizeControl = VK_TRUE;
        f13.computeFullSubgroups = VK_TRUE;
    }

    VkPhysicalDeviceVulkan12Features f12{};
    f12.sType                                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    f12.pNext                                = &f13;
    f12.timelineSemaphore                    = VK_TRUE;
    f12.bufferDeviceAddress                  = VK_TRUE;
    f12.descriptorIndexing                   = VK_TRUE;
    f12.scalarBlockLayout                    = VK_TRUE;

    VkPhysicalDeviceFeatures2 f2{};
    f2.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    f2.pNext    = &f12;
    f2.features.shaderInt64 = VK_TRUE;
    f2.features.fillModeNonSolid = VK_TRUE;

    VkDeviceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.pNext                   = &f2;
    ci.queueCreateInfoCount    = static_cast<std::uint32_t>(queue_infos.size());
    ci.pQueueCreateInfos       = queue_infos.data();
    ci.enabledExtensionCount   = static_cast<std::uint32_t>(exts.size());
    ci.ppEnabledExtensionNames = exts.data();

    if (vkCreateDevice(physical_device_, &ci, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("Context: vkCreateDevice failed");
    }
    vkGetDeviceQueue(device_, graphics_queue_family_, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, compute_queue_family_,  0, &compute_queue_);
}

void Context::createAllocator() {
    VmaAllocatorCreateInfo ci{};
    ci.flags             = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    ci.physicalDevice    = physical_device_;
    ci.device            = device_;
    ci.instance          = instance_;
    ci.vulkanApiVersion  = VK_API_VERSION_1_3;
    if (vmaCreateAllocator(&ci, &allocator_) != VK_SUCCESS) {
        throw std::runtime_error("Context: vmaCreateAllocator failed");
    }
}

void Context::createOneShotPool() {
    VkCommandPoolCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = graphics_queue_family_;
    if (vkCreateCommandPool(device_, &ci, nullptr, &one_shot_pool_) != VK_SUCCESS) {
        throw std::runtime_error("Context: failed to create one-shot command pool");
    }
}

void Context::waitIdle() const {
    vkDeviceWaitIdle(device_);
}

void Context::runOneShot(const std::function<void(VkCommandBuffer)>& fn) {
    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = one_shot_pool_;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cb = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device_, &ai, &cb);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);
    fn(cb);
    vkEndCommandBuffer(cb);

    VkSubmitInfo si{};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cb;

    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device_, &fi, nullptr, &fence);

    vkQueueSubmit(graphics_queue_, 1, &si, fence);
    vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device_, fence, nullptr);
    vkFreeCommandBuffers(device_, one_shot_pool_, 1, &cb);
}

}  // namespace gpusims::vk
