#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

typedef struct VmaAllocation_T* VmaAllocation;

namespace gpusims::vk {

class Context;

// Image dimensionality.
enum class ImageType {
    e2D,
    e3D,
};

struct ImageCreateInfo {
    ImageType         type   = ImageType::e2D;
    VkExtent3D        extent{};                              // depth=1 for 2D
    VkFormat          format = VK_FORMAT_R8G8B8A8_UNORM;
    std::uint32_t     mip_levels   = 1;
    std::uint32_t     array_layers = 1;                       // ignored for 3D
    VkSampleCountFlagBits samples  = VK_SAMPLE_COUNT_1_BIT;
    VkImageUsageFlags usage    = 0;                           // caller must set
    VkImageLayout     initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    const char*       debug_name = nullptr;
};

class Image {
public:
    static Image create(Context& ctx, const ImageCreateInfo& info);

    Image() = default;
    ~Image();

    Image(const Image&)            = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    // Handles
    VkImage           handle()    const { return image_; }
    VkImageView       view()      const { return view_; }
    VmaAllocation     allocation()const { return allocation_; }
    VkExtent3D        extent()    const { return info_.extent; }
    VkFormat          format()    const { return info_.format; }
    ImageType         type()      const { return info_.type; }

    // Helper for transitioning an image's layout. Records pipeline barriers
    // into `cmd`. The user is responsible for tracking current layout — we
    // don't automatically remember (explicit > implicit).
    static void transitionLayout(VkCommandBuffer cmd,
                                 VkImage         image,
                                 VkImageAspectFlags aspect,
                                 VkImageLayout   old_layout,
                                 VkImageLayout   new_layout,
                                 std::uint32_t   mip_levels = 1,
                                 std::uint32_t   array_layers = 1);

private:
    Context*           ctx_        = nullptr;
    VkImage            image_      = VK_NULL_HANDLE;
    VkImageView        view_       = VK_NULL_HANDLE;
    VmaAllocation      allocation_ = VK_NULL_HANDLE;
    ImageCreateInfo    info_{};
};

}  // namespace gpusims::vk
