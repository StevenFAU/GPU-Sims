#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

namespace gpusims::vk {

// Validation layer + debug utils messenger management.
//
// Compile-time-gated by GPU_SIMS_VALIDATION_LAYERS (defined to 1 in Debug,
// 0 in Release). When disabled, all functions are no-ops and return success.

// Validation layer name used when GPU_SIMS_VALIDATION_LAYERS=1.
constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation";

// Check that the validation layer is available on this system.
bool checkValidationLayerSupport();

// Names of instance extensions required when validation is enabled.
// Returns nullptr-terminated list compatible with VkInstanceCreateInfo.
const char* const* requiredDebugExtensions(std::uint32_t* out_count);

// Populate a VkDebugUtilsMessengerCreateInfoEXT with our preferred settings.
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& ci);

// Create the debug messenger after the instance is created.
VkResult createDebugMessenger(VkInstance                                instance,
                              const VkDebugUtilsMessengerCreateInfoEXT& ci,
                              VkDebugUtilsMessengerEXT*                 out_messenger);

void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);

// Tag a Vulkan object with a debug name (visible in RenderDoc, RGP, etc.).
// No-op when validation is disabled.
void setObjectName(VkDevice device, VkObjectType type, std::uint64_t handle, const char* name);

}  // namespace gpusims::vk
