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
struct DescriptorBinding;

// Graphics pipeline using VK_KHR_dynamic_rendering (no VkRenderPass needed).
// Phase 1 only needs a fullscreen triangle (vertex + fragment), so the
// fixed-function state is minimized to that case. Per-sim code that wants
// vertex buffers / instancing extends this descriptor.

struct GraphicsPipelineDesc {
    std::filesystem::path                      vertex_shader_path;
    std::filesystem::path                      fragment_shader_path;

    std::vector<DescriptorBinding>             bindings;
    std::uint32_t                              push_constant_size = 0;

    // Color attachment formats (dynamic rendering); typically swapchain format.
    std::vector<VkFormat>                      color_formats;
    VkFormat                                   depth_format = VK_FORMAT_UNDEFINED;

    // Common toggles.
    VkPrimitiveTopology                        topology   = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode                              polygon    = VK_POLYGON_MODE_FILL;
    VkCullModeFlags                            cull_mode  = VK_CULL_MODE_NONE;
    VkFrontFace                                front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    bool                                       depth_test  = false;
    bool                                       depth_write = false;
    bool                                       blend_enable= false;

    // Blend factors / op. Read by graphics_pipeline.cpp ONLY when blend_enable
    // is true. Defaults match the historical hardcoded behavior (premultiplied-
    // alpha blend), so consumers that just flip blend_enable=true get the same
    // result as before. Phase 11 sph-water thickness pass needs additive blend
    // (SRC=ONE, DST=ONE, OP=ADD) and uses this surface to opt in.
    VkBlendFactor                              src_color_blend_factor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor                              dst_color_blend_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp                                  color_blend_op         = VK_BLEND_OP_ADD;
    VkBlendFactor                              src_alpha_blend_factor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor                              dst_alpha_blend_factor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp                                  alpha_blend_op         = VK_BLEND_OP_ADD;

    // No vertex input by default (suitable for fullscreen triangle drawn
    // from gl_VertexIndex). Per-sim code overrides as needed.
    std::vector<VkVertexInputBindingDescription>   vertex_bindings;
    std::vector<VkVertexInputAttributeDescription> vertex_attributes;
};

class GraphicsPipeline {
public:
    static GraphicsPipeline create(Context&                    ctx,
                                   ShaderCompiler&             compiler,
                                   const GraphicsPipelineDesc& desc);

    GraphicsPipeline() = default;
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&)            = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

    // Hot-reload (recompiles both vertex and fragment shaders).
    bool reload(Context&                    ctx,
                ShaderCompiler&             compiler,
                Frame&                      current_frame,
                std::string*                out_error = nullptr);

    VkDescriptorSet allocateDescriptorSet();

    // Bind. Caller follows with vkCmdDraw / vkCmdDrawIndexed.
    void bind(VkCommandBuffer cmd,
              VkDescriptorSet ds,
              const void*     push_constants = nullptr,
              std::uint32_t   push_size      = 0);

    VkPipeline            handle()             const { return pipeline_; }
    VkPipelineLayout      pipelineLayout()     const { return pipeline_layout_; }
    VkDescriptorSetLayout descriptorSetLayout()const { return ds_layout_; }
    const std::vector<std::filesystem::path>& includes() const { return last_includes_; }

private:
    Context*                                  ctx_              = nullptr;
    GraphicsPipelineDesc                      desc_{};
    VkPipeline                                pipeline_         = VK_NULL_HANDLE;
    VkPipelineLayout                          pipeline_layout_  = VK_NULL_HANDLE;
    VkDescriptorSetLayout                     ds_layout_        = VK_NULL_HANDLE;
    VkShaderModule                            vert_module_      = VK_NULL_HANDLE;
    VkShaderModule                            frag_module_      = VK_NULL_HANDLE;
    VkDescriptorPool                          ds_pool_          = VK_NULL_HANDLE;
    std::vector<std::filesystem::path>        last_includes_;
};

}  // namespace gpusims::vk
