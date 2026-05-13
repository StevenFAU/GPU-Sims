#include <gpusims/vk/compute_pipeline.hpp>

#include <stdexcept>

#include <gpusims/log.hpp>
#include <gpusims/vk/context.hpp>
#include <gpusims/vk/frame.hpp>
#include <gpusims/vk/shader_compiler.hpp>

namespace gpusims::vk {

namespace {

VkDescriptorSetLayout buildDescriptorSetLayout(VkDevice device,
                                               const std::vector<DescriptorBinding>& bindings) {
    std::vector<VkDescriptorSetLayoutBinding> v;
    v.reserve(bindings.size());
    for (const auto& b : bindings) {
        VkDescriptorSetLayoutBinding lb{};
        lb.binding         = b.binding;
        lb.descriptorType  = b.type;
        lb.descriptorCount = b.count;
        lb.stageFlags      = b.stages;
        v.push_back(lb);
    }
    VkDescriptorSetLayoutCreateInfo ci{};
    ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.bindingCount = static_cast<std::uint32_t>(v.size());
    ci.pBindings    = v.data();
    VkDescriptorSetLayout l = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(device, &ci, nullptr, &l) != VK_SUCCESS) {
        throw std::runtime_error("ComputePipeline: vkCreateDescriptorSetLayout failed");
    }
    return l;
}

VkDescriptorPool buildDescriptorPool(VkDevice device,
                                     const std::vector<DescriptorBinding>& bindings,
                                     std::uint32_t max_sets) {
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(bindings.size());
    for (const auto& b : bindings) {
        VkDescriptorPoolSize ps{};
        ps.type            = b.type;
        ps.descriptorCount = b.count * max_sets;
        sizes.push_back(ps);
    }
    VkDescriptorPoolCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    ci.maxSets       = max_sets;
    ci.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
    ci.pPoolSizes    = sizes.data();
    VkDescriptorPool p = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(device, &ci, nullptr, &p) != VK_SUCCESS) {
        throw std::runtime_error("ComputePipeline: vkCreateDescriptorPool failed");
    }
    return p;
}

}  // namespace

ComputePipeline ComputePipeline::create(Context&                   ctx,
                                        ShaderCompiler&            compiler,
                                        const ComputePipelineDesc& desc) {
    ComputePipeline p;
    p.ctx_  = &ctx;
    p.desc_ = desc;

    auto compile = compiler.compileFile(desc.shader_path, ShaderStage::Compute);
    if (!compile.ok) {
        throw std::runtime_error("ComputePipeline: shader compile failed: " + compile.error);
    }
    p.last_includes_ = compile.includes;

    p.shader_module_ = ShaderCompiler::createShaderModule(ctx.device(), compile.spirv);
    if (p.shader_module_ == VK_NULL_HANDLE) {
        throw std::runtime_error("ComputePipeline: createShaderModule failed");
    }

    p.ds_layout_ = buildDescriptorSetLayout(ctx.device(), desc.bindings);

    VkPipelineLayoutCreateInfo lci{};
    lci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    lci.setLayoutCount         = 1;
    lci.pSetLayouts            = &p.ds_layout_;
    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset     = 0;
    pcr.size       = desc.push_constant_size;
    if (desc.push_constant_size > 0) {
        lci.pushConstantRangeCount = 1;
        lci.pPushConstantRanges    = &pcr;
    }
    if (vkCreatePipelineLayout(ctx.device(), &lci, nullptr, &p.pipeline_layout_) != VK_SUCCESS) {
        throw std::runtime_error("ComputePipeline: vkCreatePipelineLayout failed");
    }

    VkPipelineShaderStageCreateInfo ss{};
    ss.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    ss.module = p.shader_module_;
    ss.pName  = "main";

    // Phase 11 sph-water: subgroup-size-control extension. See INVARIANT in
    // compute_pipeline.hpp. The extension struct is stack-allocated; its
    // lifetime must span vkCreateComputePipelines below, so it lives in the
    // enclosing scope.
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup_size_ci{};
    if (desc.required_subgroup_size != 0) {
        subgroup_size_ci.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
        subgroup_size_ci.requiredSubgroupSize = desc.required_subgroup_size;
        subgroup_size_ci.pNext = const_cast<void*>(ss.pNext);
        ss.pNext = &subgroup_size_ci;
    }
    if (desc.require_full_subgroups) {
        ss.flags |= VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT;
    }

    VkComputePipelineCreateInfo cpi{};
    cpi.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpi.stage  = ss;
    cpi.layout = p.pipeline_layout_;
    if (vkCreateComputePipelines(ctx.device(), VK_NULL_HANDLE, 1, &cpi, nullptr, &p.pipeline_)
        != VK_SUCCESS) {
        throw std::runtime_error("ComputePipeline: vkCreateComputePipelines failed");
    }

    p.ds_pool_ = buildDescriptorPool(ctx.device(), desc.bindings, /*max_sets=*/16);

    return p;
}

ComputePipeline::~ComputePipeline() {
    if (!ctx_) return;
    if (ds_pool_       != VK_NULL_HANDLE) vkDestroyDescriptorPool(ctx_->device(), ds_pool_, nullptr);
    if (pipeline_      != VK_NULL_HANDLE) vkDestroyPipeline(ctx_->device(), pipeline_, nullptr);
    if (pipeline_layout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(ctx_->device(), pipeline_layout_, nullptr);
    if (ds_layout_     != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(ctx_->device(), ds_layout_, nullptr);
    if (shader_module_ != VK_NULL_HANDLE) vkDestroyShaderModule(ctx_->device(), shader_module_, nullptr);
}

ComputePipeline::ComputePipeline(ComputePipeline&& other) noexcept { *this = std::move(other); }

ComputePipeline& ComputePipeline::operator=(ComputePipeline&& other) noexcept {
    if (this != &other) {
        // Destroy our existing
        if (ctx_) {
            if (ds_pool_)         vkDestroyDescriptorPool(ctx_->device(), ds_pool_, nullptr);
            if (pipeline_)        vkDestroyPipeline(ctx_->device(), pipeline_, nullptr);
            if (pipeline_layout_) vkDestroyPipelineLayout(ctx_->device(), pipeline_layout_, nullptr);
            if (ds_layout_)       vkDestroyDescriptorSetLayout(ctx_->device(), ds_layout_, nullptr);
            if (shader_module_)   vkDestroyShaderModule(ctx_->device(), shader_module_, nullptr);
        }
        ctx_              = other.ctx_;
        desc_             = std::move(other.desc_);
        pipeline_         = other.pipeline_;
        pipeline_layout_  = other.pipeline_layout_;
        ds_layout_        = other.ds_layout_;
        shader_module_    = other.shader_module_;
        ds_pool_          = other.ds_pool_;
        last_includes_    = std::move(other.last_includes_);
        other.ctx_              = nullptr;
        other.pipeline_         = VK_NULL_HANDLE;
        other.pipeline_layout_  = VK_NULL_HANDLE;
        other.ds_layout_        = VK_NULL_HANDLE;
        other.shader_module_    = VK_NULL_HANDLE;
        other.ds_pool_          = VK_NULL_HANDLE;
    }
    return *this;
}

bool ComputePipeline::reload(Context&        ctx,
                             ShaderCompiler& compiler,
                             Frame&          current_frame,
                             std::string*    out_error) {
    auto compile = compiler.compileFile(desc_.shader_path, ShaderStage::Compute);
    if (!compile.ok) {
        if (out_error) *out_error = compile.error;
        return false;
    }

    auto new_module = ShaderCompiler::createShaderModule(ctx.device(), compile.spirv);
    if (new_module == VK_NULL_HANDLE) {
        if (out_error) *out_error = "createShaderModule failed";
        return false;
    }

    VkPipelineShaderStageCreateInfo ss{};
    ss.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    ss.module = new_module;
    ss.pName  = "main";

    // Phase 11 sph-water: preserve subgroup-size pin on hot reload. Same
    // conditional invariant as ComputePipeline::create (see header).
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup_size_ci{};
    if (desc_.required_subgroup_size != 0) {
        subgroup_size_ci.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
        subgroup_size_ci.requiredSubgroupSize = desc_.required_subgroup_size;
        subgroup_size_ci.pNext = const_cast<void*>(ss.pNext);
        ss.pNext = &subgroup_size_ci;
    }
    if (desc_.require_full_subgroups) {
        ss.flags |= VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT;
    }

    VkComputePipelineCreateInfo cpi{};
    cpi.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpi.stage  = ss;
    cpi.layout = pipeline_layout_;

    VkPipeline new_pipeline = VK_NULL_HANDLE;
    if (vkCreateComputePipelines(ctx.device(), VK_NULL_HANDLE, 1, &cpi, nullptr, &new_pipeline)
        != VK_SUCCESS) {
        vkDestroyShaderModule(ctx.device(), new_module, nullptr);
        if (out_error) *out_error = "vkCreateComputePipelines failed";
        return false;
    }

    // Defer destruction of old resources until in-flight frames finish.
    VkDevice       device     = ctx.device();
    VkPipeline     old_pipe   = pipeline_;
    VkShaderModule old_module = shader_module_;
    current_frame.deletion_queue.push([device, old_pipe, old_module]() {
        if (old_pipe   != VK_NULL_HANDLE) vkDestroyPipeline(device, old_pipe,   nullptr);
        if (old_module != VK_NULL_HANDLE) vkDestroyShaderModule(device, old_module, nullptr);
    });

    pipeline_      = new_pipeline;
    shader_module_ = new_module;
    last_includes_ = compile.includes;
    return true;
}

VkDescriptorSet ComputePipeline::allocateDescriptorSet() {
    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = ds_pool_;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts        = &ds_layout_;
    VkDescriptorSet ds = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(ctx_->device(), &ai, &ds) != VK_SUCCESS) {
        logError("ComputePipeline: vkAllocateDescriptorSets failed");
        return VK_NULL_HANDLE;
    }
    return ds;
}

void ComputePipeline::dispatch(VkCommandBuffer cmd,
                               VkDescriptorSet ds,
                               std::uint32_t   gx,
                               std::uint32_t   gy,
                               std::uint32_t   gz,
                               const void*     push_constants,
                               std::uint32_t   push_size) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    if (ds != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_,
                                0, 1, &ds, 0, nullptr);
    }
    if (push_constants && push_size > 0) {
        vkCmdPushConstants(cmd, pipeline_layout_,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, push_size, push_constants);
    }
    vkCmdDispatch(cmd, gx, gy, gz);
}

}  // namespace gpusims::vk
