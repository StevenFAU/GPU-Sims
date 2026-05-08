#include <gpusims/vk/shader_compiler.hpp>

#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_set>

#include <shaderc/shaderc.hpp>

#include <gpusims/log.hpp>
#include <gpusims/vk/context.hpp>

namespace gpusims::vk {

namespace {

shaderc_shader_kind toShadercKind(ShaderStage s) {
    switch (s) {
        case ShaderStage::Compute:  return shaderc_glsl_compute_shader;
        case ShaderStage::Vertex:   return shaderc_glsl_vertex_shader;
        case ShaderStage::Fragment: return shaderc_glsl_fragment_shader;
    }
    return shaderc_glsl_compute_shader;
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// shaderc include resolver that tracks every successfully-resolved include.
// We pass a pointer to a path-set in user_data; the resolver appends to it.
class IncludeResolver : public shaderc::CompileOptions::IncluderInterface {
public:
    IncludeResolver(std::vector<std::filesystem::path>& include_dirs,
                    std::unordered_set<std::string>&    out_paths)
        : dirs_(include_dirs), out_(out_paths) {}

    shaderc_include_result* GetInclude(const char* requested_source,
                                       shaderc_include_type type,
                                       const char* requesting_source,
                                       size_t /*include_depth*/) override {
        std::filesystem::path resolved;
        if (type == shaderc_include_type_relative) {
            std::filesystem::path base(requesting_source);
            resolved = base.parent_path() / requested_source;
        } else {
            for (const auto& d : dirs_) {
                auto candidate = d / requested_source;
                if (std::filesystem::exists(candidate)) {
                    resolved = candidate;
                    break;
                }
            }
        }
        auto* result = new shaderc_include_result{};
        if (resolved.empty() || !std::filesystem::exists(resolved)) {
            const char* err = "include not found";
            result->source_name        = "";
            result->source_name_length = 0;
            result->content            = err;
            result->content_length     = std::strlen(err);
            result->user_data          = nullptr;
            return result;
        }
        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(resolved, ec);
        out_.insert(canonical.string());

        auto* holder = new IncludeBlob{};
        holder->name    = canonical.string();
        holder->content = readFile(canonical);
        result->source_name        = holder->name.c_str();
        result->source_name_length = holder->name.size();
        result->content            = holder->content.data();
        result->content_length     = holder->content.size();
        result->user_data          = holder;
        return result;
    }

    void ReleaseInclude(shaderc_include_result* result) override {
        if (result) {
            if (result->user_data) {
                delete static_cast<IncludeBlob*>(result->user_data);
            }
            delete result;
        }
    }

private:
    struct IncludeBlob {
        std::string name;
        std::string content;
    };
    std::vector<std::filesystem::path>& dirs_;
    std::unordered_set<std::string>&    out_;
};

}  // namespace

struct ShaderCompiler::Impl {
    shaderc::Compiler                   compiler;
    std::vector<std::filesystem::path>  include_dirs;
};

ShaderCompiler::ShaderCompiler(Context& /*ctx*/) : impl_(new Impl) {}
ShaderCompiler::~ShaderCompiler() { delete impl_; }

void ShaderCompiler::addIncludeDir(std::filesystem::path dir) {
    impl_->include_dirs.push_back(std::move(dir));
}

CompileResult ShaderCompiler::compileFile(const std::filesystem::path& path, ShaderStage stage) {
    std::string src = readFile(path);
    if (src.empty()) {
        CompileResult r;
        r.error = "shader_compiler: cannot read " + path.string();
        return r;
    }
    return compileSource(src, stage, path);
}

CompileResult ShaderCompiler::compileSource(const std::string&             source,
                                            ShaderStage                    stage,
                                            const std::filesystem::path&   nominal_path) {
    CompileResult result;

    shaderc::CompileOptions opts;
    opts.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    opts.SetTargetSpirv(shaderc_spirv_version_1_6);
    opts.SetSourceLanguage(shaderc_source_language_glsl);
#ifdef NDEBUG
    opts.SetOptimizationLevel(shaderc_optimization_level_performance);
#else
    opts.SetGenerateDebugInfo();
    opts.SetOptimizationLevel(shaderc_optimization_level_zero);
#endif

    std::unordered_set<std::string> include_set;
    opts.SetIncluder(std::make_unique<IncludeResolver>(impl_->include_dirs, include_set));

    auto canonical_nominal = std::filesystem::weakly_canonical(nominal_path);

    auto module = impl_->compiler.CompileGlslToSpv(
        source, toShadercKind(stage), canonical_nominal.string().c_str(), opts);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        result.ok    = false;
        result.error = module.GetErrorMessage();
        return result;
    }
    result.ok    = true;
    result.spirv.assign(module.cbegin(), module.cend());
    result.includes.reserve(include_set.size());
    for (auto& s : include_set) result.includes.emplace_back(s);
    return result;
}

VkShaderModule ShaderCompiler::createShaderModule(VkDevice                          device,
                                                  const std::vector<std::uint32_t>& spirv) {
    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = spirv.size() * sizeof(std::uint32_t);
    ci.pCode    = spirv.data();
    VkShaderModule m = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &ci, nullptr, &m) != VK_SUCCESS) {
        logError("shader_compiler: vkCreateShaderModule failed");
        return VK_NULL_HANDLE;
    }
    return m;
}

}  // namespace gpusims::vk
