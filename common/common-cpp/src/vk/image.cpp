#include <gpusims/vk/image.hpp>

#include <stdexcept>

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#include <gpusims/log.hpp>
#include <gpusims/vk/context.hpp>
#include <gpusims/vk/debug.hpp>

namespace gpusims::vk {

namespace {

VkImageType toVkImageType(ImageType t) {
    return t == ImageType::e3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
}

VkImageViewType toVkImageViewType(ImageType t, std::uint32_t array_layers) {
    if (t == ImageType::e3D) return VK_IMAGE_VIEW_TYPE_3D;
    return array_layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
}

}  // namespace

Image Image::create(Context& ctx, const ImageCreateInfo& info) {
    Image img;
    img.ctx_  = &ctx;
    img.info_ = info;

    VkImageCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType     = toVkImageType(info.type);
    ci.format        = info.format;
    ci.extent        = info.extent;
    ci.mipLevels     = info.mip_levels;
    ci.arrayLayers   = info.type == ImageType::e3D ? 1 : info.array_layers;
    ci.samples       = info.samples;
    ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ci.usage         = info.usage;
    ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo ai{};
    ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (vmaCreateImage(ctx.allocator(), &ci, &ai, &img.image_, &img.allocation_, nullptr)
        != VK_SUCCESS) {
        throw std::runtime_error("Image: vmaCreateImage failed");
    }

    if (info.debug_name) {
        setObjectName(ctx.device(), VK_OBJECT_TYPE_IMAGE,
                      reinterpret_cast<std::uint64_t>(img.image_), info.debug_name);
    }

    VkImageViewCreateInfo vci{};
    vci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vci.image    = img.image_;
    vci.viewType = toVkImageViewType(info.type, info.array_layers);
    vci.format   = info.format;
    vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vci.subresourceRange.baseMipLevel   = 0;
    vci.subresourceRange.levelCount     = info.mip_levels;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount     = ci.arrayLayers;

    if (vkCreateImageView(ctx.device(), &vci, nullptr, &img.view_) != VK_SUCCESS) {
        vmaDestroyImage(ctx.allocator(), img.image_, img.allocation_);
        throw std::runtime_error("Image: vkCreateImageView failed");
    }

    if (info.initial_layout != VK_IMAGE_LAYOUT_UNDEFINED) {
        ctx.runOneShot([&](VkCommandBuffer cb) {
            transitionLayout(cb, img.image_, VK_IMAGE_ASPECT_COLOR_BIT,
                             VK_IMAGE_LAYOUT_UNDEFINED, info.initial_layout,
                             info.mip_levels, ci.arrayLayers);
        });
    }
    return img;
}

Image::~Image() {
    if (ctx_ && view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(ctx_->device(), view_, nullptr);
    }
    if (ctx_ && image_ != VK_NULL_HANDLE && allocation_ != VK_NULL_HANDLE) {
        vmaDestroyImage(ctx_->allocator(), image_, allocation_);
    }
}

Image::Image(Image&& other) noexcept
    : ctx_(other.ctx_),
      image_(other.image_),
      view_(other.view_),
      allocation_(other.allocation_),
      info_(other.info_) {
    other.ctx_        = nullptr;
    other.image_      = VK_NULL_HANDLE;
    other.view_       = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.info_       = {};
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        if (ctx_ && view_ != VK_NULL_HANDLE)  vkDestroyImageView(ctx_->device(), view_, nullptr);
        if (ctx_ && image_ != VK_NULL_HANDLE) vmaDestroyImage(ctx_->allocator(), image_, allocation_);
        ctx_        = other.ctx_;
        image_      = other.image_;
        view_       = other.view_;
        allocation_ = other.allocation_;
        info_       = other.info_;
        other.ctx_        = nullptr;
        other.image_      = VK_NULL_HANDLE;
        other.view_       = VK_NULL_HANDLE;
        other.allocation_ = VK_NULL_HANDLE;
        other.info_       = {};
    }
    return *this;
}

void Image::transitionLayout(VkCommandBuffer    cmd,
                             VkImage            image,
                             VkImageAspectFlags aspect,
                             VkImageLayout      old_layout,
                             VkImageLayout      new_layout,
                             std::uint32_t      mip_levels,
                             std::uint32_t      array_layers) {
    VkImageMemoryBarrier2 b{};
    b.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    b.srcStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.srcAccessMask    = VK_ACCESS_2_MEMORY_WRITE_BIT;
    b.dstStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.dstAccessMask    = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    b.oldLayout        = old_layout;
    b.newLayout        = new_layout;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image            = image;
    b.subresourceRange.aspectMask     = aspect;
    b.subresourceRange.baseMipLevel   = 0;
    b.subresourceRange.levelCount     = mip_levels;
    b.subresourceRange.baseArrayLayer = 0;
    b.subresourceRange.layerCount     = array_layers;

    VkDependencyInfo di{};
    di.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    di.imageMemoryBarrierCount  = 1;
    di.pImageMemoryBarriers     = &b;
    vkCmdPipelineBarrier2(cmd, &di);
}

}  // namespace gpusims::vk
