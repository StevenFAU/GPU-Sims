#include <gpusims/vk/graphics_pipeline.hpp>
#include <gpusims/vk/compute_pipeline.hpp>  // for DescriptorBinding

#include <stdexcept>

#include <gpusims/log.hpp>
#include <gpusims/vk/context.hpp>
#include <gpusims/vk/frame.hpp>
#include <gpusims/vk/shader_compiler.hpp>

namespace gpusims::vk {

namespace {

VkDescriptorSetLayout buildLayout(VkDevice device,
                                  const std::vector<DescriptorBinding>& bindings) {
    if (bindings.empty()) {
        VkDescriptorSetLayoutCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        VkDescriptorSetLayout l = VK_NULL_HANDLE;
        vkCreateDescriptorSetLayout(device, &ci, nullptr, &l);
        return l;
    }
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
        throw std::runtime_error("GraphicsPipeline: vkCreateDescriptorSetLayout failed");
    }
    return l;
}

VkDescriptorPool buildPool(VkDevice device,
                           const std::vector<DescriptorBinding>& bindings,
                           std::uint32_t max_sets) {
    if (bindings.empty()) {
        VkDescriptorPoolSize ps{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
        VkDescriptorPoolCreateInfo ci{};
        ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        ci.maxSets       = max_sets;
        ci.poolSizeCount = 1;
        ci.pPoolSizes    = &ps;
        VkDescriptorPool p = VK_NULL_HANDLE;
        vkCreateDescriptorPool(device, &ci, nullptr, &p);
        return p;
    }
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
        throw std::runtime_error("GraphicsPipeline: vkCreateDescriptorPool failed");
    }
    return p;
}

VkPipeline buildPipeline(Context& ctx,
                         const GraphicsPipelineDesc& d,
                         VkPipelineLayout            layout,
                         VkShaderModule              vert,
                         VkShaderModule              frag) {
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName  = "main";

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount   = static_cast<std::uint32_t>(d.vertex_bindings.size());
    vi.pVertexBindingDescriptions      = d.vertex_bindings.empty() ? nullptr : d.vertex_bindings.data();
    vi.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(d.vertex_attributes.size());
    vi.pVertexAttributeDescriptions    = d.vertex_attributes.empty() ? nullptr : d.vertex_attributes.data();

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = d.topology;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = d.polygon;
    rs.cullMode    = d.cull_mode;
    rs.frontFace   = d.front_face;
    rs.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = d.depth_test  ? VK_TRUE : VK_FALSE;
    ds.depthWriteEnable = d.depth_write ? VK_TRUE : VK_FALSE;
    ds.depthCompareOp   = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState cb_att{};
    cb_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cb_att.blendEnable    = d.blend_enable ? VK_TRUE : VK_FALSE;
    if (d.blend_enable) {
        cb_att.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cb_att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        cb_att.colorBlendOp        = VK_BLEND_OP_ADD;
        cb_att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        cb_att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cb_att.alphaBlendOp        = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments    = &cb_att;

    VkDynamicState dyn_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn{};
    dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates    = dyn_states;

    // Dynamic rendering: no VkRenderPass needed.
    VkPipelineRenderingCreateInfoKHR rendering{};
    rendering.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    rendering.colorAttachmentCount    = static_cast<std::uint32_t>(d.color_formats.size());
    rendering.pColorAttachmentFormats = d.color_formats.data();
    rendering.depthAttachmentFormat   = d.depth_format;

    VkGraphicsPipelineCreateInfo gpi{};
    gpi.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpi.pNext               = &rendering;
    gpi.stageCount          = 2;
    gpi.pStages             = stages;
    gpi.pVertexInputState   = &vi;
    gpi.pInputAssemblyState = &ia;
    gpi.pViewportState      = &vp;
    gpi.pRasterizationState = &rs;
    gpi.pMultisampleState   = &ms;
    gpi.pDepthStencilState  = &ds;
    gpi.pColorBlendState    = &cb;
    gpi.pDynamicState       = &dyn;
    gpi.layout              = layout;
    gpi.renderPass          = VK_NULL_HANDLE;  // dynamic rendering

    VkPipeline pipe = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(ctx.device(), VK_NULL_HANDLE, 1, &gpi, nullptr, &pipe)
        != VK_SUCCESS) {
        throw std::runtime_error("GraphicsPipeline: vkCreateGraphicsPipelines failed");
    }
    return pipe;
}

}  // namespace

GraphicsPipeline GraphicsPipeline::create(Context&                    ctx,
                                          ShaderCompiler&             compiler,
                                          const GraphicsPipelineDesc& desc) {
    GraphicsPipeline p;
    p.ctx_  = &ctx;
    p.desc_ = desc;

    auto vert = compiler.compileFile(desc.vertex_shader_path,   ShaderStage::Vertex);
    auto frag = compiler.compileFile(desc.fragment_shader_path, ShaderStage::Fragment);
    if (!vert.ok) throw std::runtime_error("GraphicsPipeline: vert: " + vert.error);
    if (!frag.ok) throw std::runtime_error("GraphicsPipeline: frag: " + frag.error);

    // Merge include lists.
    p.last_includes_ = vert.includes;
    for (auto& i : frag.includes) p.last_includes_.push_back(i);

    p.vert_module_ = ShaderCompiler::createShaderModule(ctx.device(), vert.spirv);
    p.frag_module_ = ShaderCompiler::createShaderModule(ctx.device(), frag.spirv);
    if (!p.vert_module_ || !p.frag_module_) {
        throw std::runtime_error("GraphicsPipeline: createShaderModule failed");
    }

    p.ds_layout_ = buildLayout(ctx.device(), desc.bindings);

    VkPipelineLayoutCreateInfo lci{};
    lci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    lci.setLayoutCount         = 1;
    lci.pSetLayouts            = &p.ds_layout_;
    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcr.offset     = 0;
    pcr.size       = desc.push_constant_size;
    if (desc.push_constant_size > 0) {
        lci.pushConstantRangeCount = 1;
        lci.pPushConstantRanges    = &pcr;
    }
    if (vkCreatePipelineLayout(ctx.device(), &lci, nullptr, &p.pipeline_layout_) != VK_SUCCESS) {
        throw std::runtime_error("GraphicsPipeline: vkCreatePipelineLayout failed");
    }

    p.pipeline_ = buildPipeline(ctx, desc, p.pipeline_layout_, p.vert_module_, p.frag_module_);
    p.ds_pool_  = buildPool(ctx.device(), desc.bindings, /*max_sets=*/16);
    return p;
}

GraphicsPipeline::~GraphicsPipeline() {
    if (!ctx_) return;
    if (ds_pool_)         vkDestroyDescriptorPool(ctx_->device(), ds_pool_, nullptr);
    if (pipeline_)        vkDestroyPipeline(ctx_->device(), pipeline_, nullptr);
    if (pipeline_layout_) vkDestroyPipelineLayout(ctx_->device(), pipeline_layout_, nullptr);
    if (ds_layout_)       vkDestroyDescriptorSetLayout(ctx_->device(), ds_layout_, nullptr);
    if (vert_module_)     vkDestroyShaderModule(ctx_->device(), vert_module_, nullptr);
    if (frag_module_)     vkDestroyShaderModule(ctx_->device(), frag_module_, nullptr);
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& o) noexcept { *this = std::move(o); }

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& o) noexcept {
    if (this != &o) {
        if (ctx_) {
            if (ds_pool_)         vkDestroyDescriptorPool(ctx_->device(), ds_pool_, nullptr);
            if (pipeline_)        vkDestroyPipeline(ctx_->device(), pipeline_, nullptr);
            if (pipeline_layout_) vkDestroyPipelineLayout(ctx_->device(), pipeline_layout_, nullptr);
            if (ds_layout_)       vkDestroyDescriptorSetLayout(ctx_->device(), ds_layout_, nullptr);
            if (vert_module_)     vkDestroyShaderModule(ctx_->device(), vert_module_, nullptr);
            if (frag_module_)     vkDestroyShaderModule(ctx_->device(), frag_module_, nullptr);
        }
        ctx_              = o.ctx_;
        desc_             = std::move(o.desc_);
        pipeline_         = o.pipeline_;
        pipeline_layout_  = o.pipeline_layout_;
        ds_layout_        = o.ds_layout_;
        vert_module_      = o.vert_module_;
        frag_module_      = o.frag_module_;
        ds_pool_          = o.ds_pool_;
        last_includes_    = std::move(o.last_includes_);
        o.ctx_              = nullptr;
        o.pipeline_         = VK_NULL_HANDLE;
        o.pipeline_layout_  = VK_NULL_HANDLE;
        o.ds_layout_        = VK_NULL_HANDLE;
        o.vert_module_      = VK_NULL_HANDLE;
        o.frag_module_      = VK_NULL_HANDLE;
        o.ds_pool_          = VK_NULL_HANDLE;
    }
    return *this;
}

bool GraphicsPipeline::reload(Context&        ctx,
                              ShaderCompiler& compiler,
                              Frame&          current_frame,
                              std::string*    out_error) {
    auto vert = compiler.compileFile(desc_.vertex_shader_path,   ShaderStage::Vertex);
    if (!vert.ok)  { if (out_error) *out_error = vert.error; return false; }
    auto frag = compiler.compileFile(desc_.fragment_shader_path, ShaderStage::Fragment);
    if (!frag.ok)  { if (out_error) *out_error = frag.error; return false; }

    auto new_v = ShaderCompiler::createShaderModule(ctx.device(), vert.spirv);
    auto new_f = ShaderCompiler::createShaderModule(ctx.device(), frag.spirv);
    if (!new_v || !new_f) {
        if (new_v) vkDestroyShaderModule(ctx.device(), new_v, nullptr);
        if (new_f) vkDestroyShaderModule(ctx.device(), new_f, nullptr);
        if (out_error) *out_error = "createShaderModule failed";
        return false;
    }

    VkPipeline new_pipe = VK_NULL_HANDLE;
    try {
        new_pipe = buildPipeline(ctx, desc_, pipeline_layout_, new_v, new_f);
    } catch (const std::exception& e) {
        vkDestroyShaderModule(ctx.device(), new_v, nullptr);
        vkDestroyShaderModule(ctx.device(), new_f, nullptr);
        if (out_error) *out_error = e.what();
        return false;
    }

    VkDevice device  = ctx.device();
    VkPipeline old_p = pipeline_;
    VkShaderModule old_v = vert_module_;
    VkShaderModule old_f = frag_module_;
    current_frame.deletion_queue.push([device, old_p, old_v, old_f]() {
        if (old_p) vkDestroyPipeline(device, old_p, nullptr);
        if (old_v) vkDestroyShaderModule(device, old_v, nullptr);
        if (old_f) vkDestroyShaderModule(device, old_f, nullptr);
    });

    pipeline_     = new_pipe;
    vert_module_  = new_v;
    frag_module_  = new_f;
    last_includes_ = vert.includes;
    for (auto& i : frag.includes) last_includes_.push_back(i);
    return true;
}

VkDescriptorSet GraphicsPipeline::allocateDescriptorSet() {
    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = ds_pool_;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts        = &ds_layout_;
    VkDescriptorSet ds = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(ctx_->device(), &ai, &ds) != VK_SUCCESS) {
        logError("GraphicsPipeline: vkAllocateDescriptorSets failed");
        return VK_NULL_HANDLE;
    }
    return ds;
}

void GraphicsPipeline::bind(VkCommandBuffer cmd,
                            VkDescriptorSet ds,
                            const void*     push_constants,
                            std::uint32_t   push_size) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    if (ds != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_,
                                0, 1, &ds, 0, nullptr);
    }
    if (push_constants && push_size > 0) {
        vkCmdPushConstants(cmd, pipeline_layout_,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, push_size, push_constants);
    }
}

}  // namespace gpusims::vk
