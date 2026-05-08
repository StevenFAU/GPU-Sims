#include <gpusims/vk/buffer.hpp>

#include <cassert>
#include <cstring>
#include <stdexcept>

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#include <gpusims/log.hpp>
#include <gpusims/vk/context.hpp>
#include <gpusims/vk/debug.hpp>

namespace gpusims::vk {

namespace {

VmaAllocationCreateInfo makeAllocInfo(MemoryUsage mem) {
    VmaAllocationCreateInfo ai{};
    switch (mem) {
        case MemoryUsage::DeviceLocal:
            ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;
        case MemoryUsage::HostVisibleSequential:
            ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        case MemoryUsage::HostVisibleRandom:
            ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
    }
    return ai;
}

}  // namespace

Buffer Buffer::create(Context&            ctx,
                      std::size_t         bytes,
                      VkBufferUsageFlags  usage,
                      MemoryUsage         mem,
                      const char*         debug_name) {
    Buffer b;
    b.ctx_  = &ctx;
    b.size_ = bytes;

    VkBufferCreateInfo ci{};
    ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size        = bytes;
    ci.usage       = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto alloc_ci = makeAllocInfo(mem);

    VmaAllocationInfo info{};
    if (vmaCreateBuffer(ctx.allocator(), &ci, &alloc_ci, &b.buffer_, &b.allocation_, &info)
        != VK_SUCCESS) {
        throw std::runtime_error("Buffer: vmaCreateBuffer failed");
    }
    b.mapped_ = info.pMappedData;

    if (debug_name) {
        setObjectName(ctx.device(), VK_OBJECT_TYPE_BUFFER,
                      reinterpret_cast<std::uint64_t>(b.buffer_), debug_name);
    }
    return b;
}

Buffer::~Buffer() {
    if (ctx_ && buffer_ != VK_NULL_HANDLE && allocation_ != VK_NULL_HANDLE) {
        vmaDestroyBuffer(ctx_->allocator(), buffer_, allocation_);
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : ctx_(other.ctx_),
      buffer_(other.buffer_),
      allocation_(other.allocation_),
      size_(other.size_),
      mapped_(other.mapped_) {
    other.ctx_        = nullptr;
    other.buffer_     = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.size_       = 0;
    other.mapped_     = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        if (ctx_ && buffer_ != VK_NULL_HANDLE) {
            vmaDestroyBuffer(ctx_->allocator(), buffer_, allocation_);
        }
        ctx_        = other.ctx_;
        buffer_     = other.buffer_;
        allocation_ = other.allocation_;
        size_       = other.size_;
        mapped_     = other.mapped_;
        other.ctx_        = nullptr;
        other.buffer_     = VK_NULL_HANDLE;
        other.allocation_ = VK_NULL_HANDLE;
        other.size_       = 0;
        other.mapped_     = nullptr;
    }
    return *this;
}

void Buffer::uploadDirect(const void* src, std::size_t bytes, std::size_t offset) {
    assert(mapped_ != nullptr && "uploadDirect requires HostVisible* memory");
    assert(offset + bytes <= size_);
    std::memcpy(static_cast<std::uint8_t*>(mapped_) + offset, src, bytes);
    if (allocation_) {
        vmaFlushAllocation(ctx_->allocator(), allocation_, offset, bytes);
    }
}

void Buffer::stage(Context& ctx, const void* src, std::size_t bytes, std::size_t offset) {
    Buffer staging = Buffer::create(ctx, bytes,
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    MemoryUsage::HostVisibleSequential,
                                    "buffer-stage");
    staging.uploadDirect(src, bytes, 0);

    ctx.runOneShot([&](VkCommandBuffer cb) {
        VkBufferCopy region{};
        region.srcOffset = 0;
        region.dstOffset = offset;
        region.size      = bytes;
        vkCmdCopyBuffer(cb, staging.handle(), buffer_, 1, &region);
    });
}

VkDeviceAddress Buffer::deviceAddress(VkDevice device) const {
    if (buffer_ == VK_NULL_HANDLE) return 0;
    VkBufferDeviceAddressInfo bi{};
    bi.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bi.buffer = buffer_;
    return vkGetBufferDeviceAddress(device, &bi);
}

}  // namespace gpusims::vk
