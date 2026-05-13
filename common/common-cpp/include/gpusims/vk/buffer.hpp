#pragma once

#include <cstddef>
#include <cstdint>

#include <vulkan/vulkan.h>

typedef struct VmaAllocation_T* VmaAllocation;

namespace gpusims::vk {

class Context;

// Memory residency hint passed at creation. Maps to VMA usage flags.
enum class MemoryUsage {
    DeviceLocal,            // GPU-only; fastest for compute/render reads. Use staging to upload.
    HostVisibleSequential,  // CPU-mapped, write-combined; ideal for upload buffers (one-pass writes).
    HostVisibleRandom,      // CPU-mapped with cached reads; for readback or per-frame dynamic data.
};

class Buffer {
public:
    static Buffer create(Context&         ctx,
                         std::size_t      bytes,
                         VkBufferUsageFlags usage,
                         MemoryUsage      mem,
                         const char*      debug_name = nullptr);

    Buffer() = default;
    ~Buffer();

    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    // Handles
    VkBuffer      handle()      const { return buffer_; }
    VmaAllocation allocation()  const { return allocation_; }
    std::size_t   sizeBytes()   const { return size_; }

    // Mapped pointer; non-null only for HostVisible* memory usages.
    void*         mapped()      const { return mapped_; }

    // Convenience: copy `bytes` from src into the mapped pointer. Aborts in
    // Debug if this buffer is not host-visible.
    void uploadDirect(const void* src, std::size_t bytes, std::size_t offset = 0);

    // Stage-and-copy upload for DeviceLocal buffers. Allocates a transient
    // staging buffer, copies src into it, and submits a copy on the graphics
    // queue. Synchronous (waits for copy to complete).
    void stage(Context& ctx, const void* src, std::size_t bytes, std::size_t offset = 0);

    // Symmetric counterpart to stage(): copy bytes out of a DeviceLocal buffer
    // into host memory. Allocates a transient host-visible staging buffer,
    // submits a vkCmdCopyBuffer on the graphics queue, waits, and memcpys
    // the staging contents into dst. Synchronous. Phase 11 sph-water consumer
    // for F5 capture-save + Alembic-export per-frame readback.
    void readback(Context& ctx, void* dst, std::size_t bytes, std::size_t offset = 0);

    // Get a VkBufferDeviceAddress (for buffer device addresses).
    VkDeviceAddress deviceAddress(VkDevice device) const;

private:
    Context*      ctx_        = nullptr;
    VkBuffer      buffer_     = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
    std::size_t   size_       = 0;
    void*         mapped_     = nullptr;
};

}  // namespace gpusims::vk
