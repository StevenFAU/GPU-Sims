#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace gpusims::vk {

class Context;
class ShaderCompiler;
struct Frame;

// Description of a compute pipeline's resource bindings.
//
// We use traditional VkDescriptorSetLayout + VkPipelineLayout because it's
// the most documented path and what every per-sim chat will be familiar with.
// The wrapper below hides pool / set allocation; per-sim code writes
// descriptors via WriteDescriptorSet and dispatches with explicit binding.

struct DescriptorBinding {
    std::uint32_t      binding;
    VkDescriptorType   type;
    std::uint32_t      count = 1;
    VkShaderStageFlags stages = VK_SHADER_STAGE_COMPUTE_BIT;
};

struct ComputePipelineDesc {
    std::filesystem::path        shader_path;       // for hot-reload tracking
    std::vector<DescriptorBinding> bindings;
    std::uint32_t                push_constant_size = 0;       // bytes; 0 = none

    // Phase 11 sph-water: subgroup-size-control fields. Both default to
    // sentinel "unconstrained" values; the existing default-null pNext-chain
    // path is preserved when both fields are at their defaults.
    //
    // INVARIANT (must be preserved by all future maintainers):
    //   If required_subgroup_size != 0 OR require_full_subgroups == true,
    //   compute_pipeline.cpp builds VkPipelineShaderStageRequiredSubgroupSize
    //   CreateInfo and chains it into VkPipelineShaderStageCreateInfo.pNext
    //   (and/or sets the REQUIRE_FULL_SUBGROUPS_BIT flag). Otherwise pNext
    //   stays null and the flag stays zero.
    //
    //   Collapsing the conditional (always building the extension struct
    //   regardless of values) would force every compute pipeline to carry the
    //   subgroup-size extension, breaking compatibility with drivers/devices
    //   that don't support VK_EXT_subgroup_size_control.
    std::uint32_t                required_subgroup_size = 0;     // 0 = unconstrained
    bool                         require_full_subgroups = false; // false = unconstrained
};

class ComputePipeline {
public:
    static ComputePipeline create(Context&                   ctx,
                                  ShaderCompiler&            compiler,
                                  const ComputePipelineDesc& desc);

    ComputePipeline() = default;
    ~ComputePipeline();

    ComputePipeline(const ComputePipeline&)            = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;
    ComputePipeline(ComputePipeline&& other) noexcept;
    ComputePipeline& operator=(ComputePipeline&& other) noexcept;

    // Reload from disk. Returns true on success; on failure the existing
    // pipeline remains valid and the error is in `out_error`.
    //
    // Old VkShaderModule / VkPipeline are appended to the current frame's
    // deletion queue (passed in) so they're destroyed safely once in-flight
    // frames complete.
    bool reload(Context&                   ctx,
                ShaderCompiler&            compiler,
                Frame&                     current_frame,
                std::string*               out_error = nullptr);

    // Allocate a descriptor set from the wrapper's internal pool.
    VkDescriptorSet allocateDescriptorSet();

    // Bind & dispatch. Caller is responsible for inserting any required
    // memory barriers around the dispatch.
    void dispatch(VkCommandBuffer cmd,
                  VkDescriptorSet ds,
                  std::uint32_t   gx,
                  std::uint32_t   gy,
                  std::uint32_t   gz,
                  const void*     push_constants = nullptr,
                  std::uint32_t   push_size      = 0);

    // Handles for callers that want to bind/dispatch directly.
    VkPipeline            handle()        const { return pipeline_; }
    VkPipelineLayout      pipelineLayout()const { return pipeline_layout_; }
    VkDescriptorSetLayout descriptorSetLayout() const { return ds_layout_; }
    const std::filesystem::path& shaderPath() const { return desc_.shader_path; }
    const std::vector<std::filesystem::path>& includes() const { return last_includes_; }

private:
    Context*                                  ctx_              = nullptr;
    ComputePipelineDesc                       desc_{};
    VkPipeline                                pipeline_         = VK_NULL_HANDLE;
    VkPipelineLayout                          pipeline_layout_  = VK_NULL_HANDLE;
    VkDescriptorSetLayout                     ds_layout_        = VK_NULL_HANDLE;
    VkShaderModule                            shader_module_    = VK_NULL_HANDLE;
    VkDescriptorPool                          ds_pool_          = VK_NULL_HANDLE;
    std::vector<std::filesystem::path>        last_includes_;
};

}  // namespace gpusims::vk
