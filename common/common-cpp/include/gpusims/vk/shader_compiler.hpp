#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace gpusims::vk {

class Context;

// GLSL → SPIR-V at runtime via shaderc.
//
// Used by hot-reload: when a .glsl file changes, we recompile to SPIR-V and
// rebuild the affected pipeline. Compile errors are surfaced as a string so
// the renderer can show them in an ImGui toast and keep the old pipeline.

enum class ShaderStage {
    Compute,
    Vertex,
    Fragment,
};

struct CompileResult {
    bool                       ok = false;
    std::vector<std::uint32_t> spirv;     // valid if ok == true
    std::string                error;     // non-empty if !ok
    // Files included transitively, for hot-reload include-graph tracking.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    std::vector<std::filesystem::path> includes;
};

class ShaderCompiler {
public:
    explicit ShaderCompiler(Context& ctx);
    ~ShaderCompiler();

    ShaderCompiler(const ShaderCompiler&)            = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

    // Add a directory to search for #include files.
    void addIncludeDir(std::filesystem::path dir);

    // Compile a GLSL source file. The result holds either valid SPIR-V or an
    // error string. The result also lists the files transitively included,
    // so hot-reload can watch them.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    CompileResult compileFile(const std::filesystem::path& path, ShaderStage stage);

    // Compile from in-memory source. `nominal_path` is used in error messages
    // and also for resolving #include relatively.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    CompileResult compileSource(const std::string&             source,
                                ShaderStage                    stage,
                                const std::filesystem::path&   nominal_path);

    // Build a VkShaderModule from compiled SPIR-V.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    static VkShaderModule createShaderModule(VkDevice                          device,
                                             const std::vector<std::uint32_t>& spirv);

private:
    struct Impl;
    Impl* impl_;  // pImpl — keeps shaderc::Compiler etc. out of public header.
};

}  // namespace gpusims::vk
