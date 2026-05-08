#include <gpusims/vk/debug.hpp>

#include <cstring>
#include <vector>

#include <gpusims/log.hpp>

namespace gpusims::vk {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*                                        /*user*/) {
    if (!data || !data->pMessage) return VK_FALSE;
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            logTrace("vk: {}", data->pMessage); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            logInfo("vk: {}", data->pMessage);  break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            logWarn("vk: {}", data->pMessage);  break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            logError("vk: {}", data->pMessage); break;
        default:
            logInfo("vk: {}", data->pMessage);  break;
    }
    return VK_FALSE;  // do not abort the offending Vulkan call
}

}  // namespace

bool checkValidationLayerSupport() {
#if GPU_SIMS_VALIDATION_LAYERS
    std::uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> available(count);
    vkEnumerateInstanceLayerProperties(&count, available.data());
    for (const auto& layer : available) {
        if (std::strcmp(layer.layerName, kValidationLayerName) == 0) return true;
    }
    return false;
#else
    return false;
#endif
}

const char* const* requiredDebugExtensions(std::uint32_t* out_count) {
#if GPU_SIMS_VALIDATION_LAYERS
    static const char* exts[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    if (out_count) *out_count = 1;
    return exts;
#else
    if (out_count) *out_count = 0;
    return nullptr;
#endif
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& ci) {
    ci = {};
    ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    ci.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ci.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci.pfnUserCallback = debugCallback;
}

VkResult createDebugMessenger(VkInstance instance,
                              const VkDebugUtilsMessengerCreateInfoEXT& ci,
                              VkDebugUtilsMessengerEXT* out_messenger) {
#if GPU_SIMS_VALIDATION_LAYERS
    auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (!fn) return VK_ERROR_EXTENSION_NOT_PRESENT;
    return fn(instance, &ci, nullptr, out_messenger);
#else
    (void)instance; (void)ci; (void)out_messenger;
    return VK_SUCCESS;
#endif
}

void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) {
#if GPU_SIMS_VALIDATION_LAYERS
    if (messenger == VK_NULL_HANDLE) return;
    auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (fn) fn(instance, messenger, nullptr);
#else
    (void)instance; (void)messenger;
#endif
}

void setObjectName(VkDevice device, VkObjectType type, std::uint64_t handle, const char* name) {
#if GPU_SIMS_VALIDATION_LAYERS
    if (!name || handle == 0) return;
    auto fn = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
    if (!fn) return;
    VkDebugUtilsObjectNameInfoEXT info{};
    info.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType   = type;
    info.objectHandle = handle;
    info.pObjectName  = name;
    fn(device, &info);
#else
    (void)device; (void)type; (void)handle; (void)name;
#endif
}

}  // namespace gpusims::vk
