// Reaction-Diffusion 3D — first GPU-Sims Stack C sim.
//
// 256³ Gray-Scott RD on a periodic 3D grid, Forward Euler integration with
// fixed substep dt, visualized via volume raymarching with HDR + Reinhard
// tonemap (inline in the raymarch fragment shader). Six Pearson 1993 named
// parameter presets shipped as the headline dropdown.

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <gpusims/camera.hpp>
#include <gpusims/gpu_profiler.hpp>
#include <gpusims/hot_reload.hpp>
#include <gpusims/imgui_setup.hpp>
#include <gpusims/log.hpp>
#include <gpusims/state_reader.hpp>
#include <gpusims/state_writer.hpp>
#include <gpusims/vk/buffer.hpp>
#include <gpusims/vk/compute_pipeline.hpp>
#include <gpusims/vk/context.hpp>
#include <gpusims/vk/frame.hpp>
#include <gpusims/vk/graphics_pipeline.hpp>
#include <gpusims/vk/image.hpp>
#include <gpusims/vk/renderer.hpp>
#include <gpusims/vk/shader_compiler.hpp>
#include <gpusims/vk/window.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace gv = gpusims::vk;
using gpusims::Camera;
using gpusims::CameraInputState;
using gpusims::HotReloader;
using gpusims::GpuProfiler;
using gpusims::StateReader;
using gpusims::StateWriter;
using gpusims::initLogger;
using gpusims::logInfo;
using gpusims::logWarn;
using gpusims::logError;
using nlohmann::json;

// ============================================================================
// Constants
// ============================================================================

constexpr uint32_t GRID_SIZE                 = 256;
constexpr uint32_t WG_DIM                    = 8;     // workgroup_size in shader
constexpr int      SUBSTEPS_DEFAULT          = 4;
constexpr float    SIM_DT_DEFAULT            = 0.5f;
constexpr uint32_t INIT_SEED_DEFAULT         = 0xC0FFEEu;
constexpr uint32_t SEED_BLOCK_DEF            = 16;
constexpr float    NOISE_AMP_DEFAULT         = 0.05f;
constexpr int      RAYMARCH_STEPS_DF         = 96;
constexpr float    DENSITY_THRESHOLD         = 0.20f;
constexpr float    DENSITY_INTENSITY         = 1.0f;
constexpr float    EXPOSURE_DEFAULT          = 1.0f;
constexpr float    BLOOM_INTENSITY_DEFAULT   = 0.15f;
constexpr float    ORBIT_DEFAULT_DEG_PER_SEC = 6.0f;
constexpr float    ORBIT_DEFAULT_RADIUS      = 1.5f;
constexpr float    FOV_DEG_DEFAULT           = 50.0f;
constexpr float    NEAR_PLANE                = 0.05f;
constexpr float    FAR_PLANE                 = 50.0f;

// ============================================================================
// Pearson 1993 named presets
// ============================================================================

struct PearsonPreset {
    const char* name;
    float F;
    float k;
    float Du;
    float Dv;
};

constexpr std::array<PearsonPreset, 6> PEARSON_PRESETS = {{
    {"\xCE\xBB - Irregular spots",      0.026f, 0.061f, 0.16f, 0.08f},  // λ
    {"\xCF\x83 - Stripes",              0.037f, 0.060f, 0.16f, 0.08f},  // σ
    {"\xCE\xB1 - Chaotic",              0.014f, 0.047f, 0.16f, 0.08f},  // α
    {"\xCE\xB2 - Uniform-ish",          0.026f, 0.055f, 0.16f, 0.08f},  // β
    {"\xCE\xBE - Moving spots",         0.018f, 0.051f, 0.16f, 0.08f},  // ξ
    {"\xCF\x84 - U-skate",              0.020f, 0.052f, 0.16f, 0.08f},  // τ
}};

// ============================================================================
// Colormap LUT — Inigo Quilez polynomial fits (same coefficients as Phase 2)
// ============================================================================

namespace colormap {

struct Vec3 { float x, y, z; };

constexpr std::array<Vec3, 7> magma = {{
    {-0.002136485053939582f, -0.000749655052795221f, -0.005386127855323933f},
    { 0.2516605407371642f,    0.6775232436837668f,    2.494026599312351f},
    { 8.353717279216625f,    -3.577719514958484f,     0.3144679030132573f},
    {-27.66873308576866f,     14.26473078096533f,    -13.64921318813922f},
    { 52.17613981234068f,    -27.94360607168351f,     12.94416944238394f},
    {-50.76852536473588f,     29.04658282127291f,     4.23415299384598f},
    { 18.65570506591883f,    -11.48977351997711f,    -5.601961508734096f},
}};

constexpr std::array<Vec3, 7> inferno = {{
    { 0.0002189403691192265f,  0.001651004631001012f, -0.01948089843709184f},
    { 0.1065134194856116f,     0.5639564367884091f,    3.932712388889277f},
    { 11.60249308247187f,     -3.972853965665698f,    -15.9423941062914f},
    {-41.70399613139459f,      17.43639888205313f,     44.35414519872813f},
    { 77.162935699427f,       -33.40235894210092f,    -81.80730925738993f},
    {-71.31942824499214f,      32.62606426397723f,     73.20951985803202f},
    { 25.13112622477341f,    -12.24266895238567f,    -23.07032500287172f},
}};

constexpr std::array<Vec3, 7> viridis = {{
    { 0.2777273272234177f,   0.005407344544966578f,  0.3340998053353061f},
    { 0.1050930431085774f,   1.404613529898575f,     1.384590162594685f},
    {-0.3308618287255563f,   0.214847559468213f,     0.09509516302823659f},
    {-4.634230498983486f,   -5.799100973351585f,    -19.33244095627987f},
    { 6.228269936347081f,    14.17993336680509f,     56.69055260068105f},
    { 4.776384997670288f,   -13.74514537774601f,    -65.35303263337234f},
    {-5.435455855934631f,    4.645852612178535f,     26.3124352495832f},
}};

inline std::array<float, 3> eval(const std::array<Vec3, 7>& c, float t) {
    float r = c[6].x, g = c[6].y, b = c[6].z;
    for (int i = 5; i >= 0; --i) {
        r = c[i].x + t * r;
        g = c[i].y + t * g;
        b = c[i].z + t * b;
    }
    return {r, g, b};
}

inline std::array<float, 3> hsv01(float t) {
    float h = std::fmod(std::fmod(t, 1.0f) + 1.0f, 1.0f);
    int   i = int(h * 6.0f);
    float f = h * 6.0f - float(i);
    float q = 1.0f - f;
    switch (i % 6) {
        case 0: return {1.0f, f,    0.0f};
        case 1: return {q,    1.0f, 0.0f};
        case 2: return {0.0f, 1.0f, f};
        case 3: return {0.0f, q,    1.0f};
        case 4: return {f,    0.0f, 1.0f};
        case 5: return {1.0f, 0.0f, q};
    }
    return {0, 0, 0};
}

inline std::vector<uint8_t> build_lut_data() {
    // 256 wide × 4 high RGBA8. Rows: 0=magma, 1=inferno, 2=viridis, 3=hsv.
    std::vector<uint8_t> out(256 * 4 * 4);

    auto write_row = [&](int row, auto sampler) {
        for (int i = 0; i < 256; ++i) {
            float t = float(i) / 255.0f;
            auto rgb = sampler(t);
            int o = (row * 256 + i) * 4;
            out[o + 0] = uint8_t(std::clamp(rgb[0], 0.0f, 1.0f) * 255.0f);
            out[o + 1] = uint8_t(std::clamp(rgb[1], 0.0f, 1.0f) * 255.0f);
            out[o + 2] = uint8_t(std::clamp(rgb[2], 0.0f, 1.0f) * 255.0f);
            out[o + 3] = 255;
        }
    };

    write_row(0, [](float t){ return eval(magma,   t); });
    write_row(1, [](float t){ return eval(inferno, t); });
    write_row(2, [](float t){ return eval(viridis, t); });
    write_row(3, [](float t){ return hsv01(t); });

    return out;
}

}  // namespace colormap

// ============================================================================
// Xorshift32 (deterministic noise) — matches Phase 2's attractors.ts
// ============================================================================

struct Xorshift32 {
    uint32_t state;
    explicit Xorshift32(uint32_t seed) : state(seed ? seed : 0x9E3779B9u) {}
    float next01() {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return float(x) / 4294967296.0f;
    }
};

// ============================================================================
// Initial-condition synthesis: u = 1 + noise, v = 0 + noise everywhere;
// centered seed_block: u = 0.5 + noise, v = 0.25 + noise.
// ============================================================================

void seed_initial_conditions(std::vector<float>& u_field,
                             std::vector<float>& v_field,
                             uint32_t grid,
                             uint32_t seed_block,
                             float noise_amp,
                             uint32_t init_seed) {
    const size_t N = size_t(grid) * grid * grid;
    if (u_field.size() != N || v_field.size() != N) {
        throw std::runtime_error("seed_initial_conditions: field size mismatch");
    }
    Xorshift32 rng(init_seed);

    int half = int(seed_block) / 2;
    int center = int(grid) / 2;
    int lo = center - half;
    int hi = center + half;

    for (uint32_t z = 0; z < grid; ++z) {
        for (uint32_t y = 0; y < grid; ++y) {
            for (uint32_t x = 0; x < grid; ++x) {
                size_t idx = size_t(z) * grid * grid + size_t(y) * grid + x;
                float nu = (rng.next01() - 0.5f) * noise_amp;
                float nv = (rng.next01() - 0.5f) * noise_amp;
                bool in_block = int(x) >= lo && int(x) < hi
                             && int(y) >= lo && int(y) < hi
                             && int(z) >= lo && int(z) < hi;
                if (in_block) {
                    u_field[idx] = 0.5f  + nu;
                    v_field[idx] = 0.25f + nv;
                } else {
                    u_field[idx] = 1.0f + nu;
                    v_field[idx] = 0.0f + nv;
                }
            }
        }
    }
}

// ============================================================================
// Runtime state
// ============================================================================

struct Runtime {
    int       presetIndex   = 0;
    bool      isCustom      = false;
    float     F  = PEARSON_PRESETS[0].F;
    float     k  = PEARSON_PRESETS[0].k;
    float     Du = PEARSON_PRESETS[0].Du;
    float     Dv = PEARSON_PRESETS[0].Dv;
    int       substeps      = SUBSTEPS_DEFAULT;
    int       raymarchSteps = RAYMARCH_STEPS_DF;
    float     densityThreshold = DENSITY_THRESHOLD;
    float     densityIntensity = DENSITY_INTENSITY;
    int       colormap      = 0;
    float     bloomIntensity = BLOOM_INTENSITY_DEFAULT;
    float     exposure      = EXPOSURE_DEFAULT;
    bool      autoOrbit     = true;
    float     orbitSpeedDegPerSec = ORBIT_DEFAULT_DEG_PER_SEC;
    float     orbitRadius   = ORBIT_DEFAULT_RADIUS;
    uint32_t  initSeed      = INIT_SEED_DEFAULT;
    uint32_t  seedBlockSize = SEED_BLOCK_DEF;
    float     noiseAmp      = NOISE_AMP_DEFAULT;
    bool      windowFullscreen = false;   // not used in v1; tracked for capture compatibility
    uint64_t  iteration     = 0;
};

void apply_preset(Runtime& rt, int idx) {
    rt.presetIndex = idx;
    rt.isCustom    = false;
    rt.F  = PEARSON_PRESETS[size_t(idx)].F;
    rt.k  = PEARSON_PRESETS[size_t(idx)].k;
    rt.Du = PEARSON_PRESETS[size_t(idx)].Du;
    rt.Dv = PEARSON_PRESETS[size_t(idx)].Dv;
}

// ============================================================================
// Uniform layouts — std140, must match GLSL
// ============================================================================

struct alignas(16) RdUniformsHost {
    float    Du;
    float    Dv;
    float    F;
    float    k;
    float    dt;
    uint32_t gridSize;
    uint32_t _pad0;
    uint32_t _pad1;
};

struct alignas(16) RaymarchUniformsHost {
    glm::mat4 invViewProj;
    glm::vec4 cameraPos;
    glm::vec4 volumeMin;
    glm::vec4 volumeMax;
    int32_t   stepCount;
    float     densityThreshold;
    float     densityIntensity;
    float     colormapIndex;
    float     exposure;
    float     bloomIntensity;
    float     _pad0;
    float     _pad1;
};

// ============================================================================
// GLFW input snapshot (mirrors hello-world)
// ============================================================================

struct InputState {
    double last_x = 0.0, last_y = 0.0;
    double scroll = 0.0;
    bool   first  = true;

    CameraInputState snapshot(GLFWwindow* w) {
        CameraInputState s;
        s.key_w      = glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
        s.key_a      = glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
        s.key_s      = glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
        s.key_d      = glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
        s.key_q      = glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS;
        s.key_e      = glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS;
        s.shift_held = (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS) ||
                       (glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        s.mouse_left   = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT)   == GLFW_PRESS;
        s.mouse_right  = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT)  == GLFW_PRESS;
        s.mouse_middle = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

        double x = 0.0, y = 0.0;
        glfwGetCursorPos(w, &x, &y);
        if (first) { last_x = x; last_y = y; first = false; }
        s.mouse_dx = float(x - last_x);
        s.mouse_dy = float(y - last_y);
        last_x = x; last_y = y;

        s.scroll_dy = float(scroll);
        scroll = 0.0;

        if (ImGui::GetIO().WantCaptureMouse) {
            s.mouse_left = s.mouse_right = s.mouse_middle = false;
            s.mouse_dx = s.mouse_dy = 0.0f;
            s.scroll_dy = 0.0f;
        }
        if (ImGui::GetIO().WantCaptureKeyboard) {
            s.key_w = s.key_a = s.key_s = s.key_d = s.key_q = s.key_e = false;
        }
        return s;
    }
};
static InputState g_input;
static void scrollCallback(GLFWwindow*, double, double dy) { g_input.scroll += dy; }

// ============================================================================
// Descriptor write helpers
// ============================================================================

static void writeRdComputeDescriptor(VkDevice        device,
                                     VkDescriptorSet ds,
                                     VkImageView     u_curr_view,
                                     VkImageView     v_curr_view,
                                     VkImageView     u_next_view,
                                     VkImageView     v_next_view,
                                     VkSampler       nearest_sampler,
                                     VkBuffer        rd_uniform) {
    VkDescriptorImageInfo u_curr_i{};
    u_curr_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    u_curr_i.imageView   = u_curr_view;
    u_curr_i.sampler     = nearest_sampler;

    VkDescriptorImageInfo v_curr_i{};
    v_curr_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_curr_i.imageView   = v_curr_view;
    v_curr_i.sampler     = nearest_sampler;

    VkDescriptorImageInfo u_next_i{};
    u_next_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    u_next_i.imageView   = u_next_view;
    u_next_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorImageInfo v_next_i{};
    v_next_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_next_i.imageView   = v_next_view;
    v_next_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = rd_uniform;
    ub_i.offset = 0;
    ub_i.range  = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].pImageInfo = &u_curr_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &v_curr_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[2].pImageInfo = &u_next_i;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[3].pImageInfo = &v_next_i;
    w[4].dstSet = ds; w[4].dstBinding = 4; w[4].descriptorCount = 1;
    w[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[4].pBufferInfo = &ub_i;

    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeRaymarchDescriptor(VkDevice        device,
                                    VkDescriptorSet ds,
                                    VkImageView     v_field_view,
                                    VkImageView     lut_view,
                                    VkSampler       linear_sampler,
                                    VkSampler       lut_sampler,
                                    VkBuffer        rm_uniform) {
    VkDescriptorImageInfo v_i{};
    v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_i.imageView   = v_field_view;
    v_i.sampler     = linear_sampler;

    VkDescriptorImageInfo lut_i{};
    lut_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    lut_i.imageView   = lut_view;
    lut_i.sampler     = lut_sampler;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = rm_uniform;
    ub_i.offset = 0;
    ub_i.range  = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &lut_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[2].pBufferInfo = &ub_i;

    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

// ============================================================================
// main
// ============================================================================

int main() {
    initLogger();
    logInfo("reaction-diffusion-3d: starting up");

    // ---- Vulkan + window + renderer ---------------------------------------
    gv::Context  ctx;
    gv::Window   window(ctx, 1920, 1080, "Reaction-Diffusion 3D - GPU-Sims");
    gv::Renderer renderer(ctx, window);

    glfwSetScrollCallback(window.glfwWindow(), scrollCallback);

    // ---- Shader compiler + hot-reload --------------------------------------
    gv::ShaderCompiler compiler(ctx);
    const fs::path shader_dir = GPU_SIMS_RD3D_SHADER_DIR;
    compiler.addIncludeDir(shader_dir);

    HotReloader hot;

    // ---- Field textures: u, v with ping-pong (3D r32f, 256³, GENERAL) ------
    auto make_field = [&](const char* dbg) {
        gv::ImageCreateInfo info{};
        info.type           = gv::ImageType::e3D;
        info.extent         = { GRID_SIZE, GRID_SIZE, GRID_SIZE };
        info.format         = VK_FORMAT_R32_SFLOAT;
        info.usage          = VK_IMAGE_USAGE_STORAGE_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT
                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                            | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.initial_layout = VK_IMAGE_LAYOUT_GENERAL;
        info.debug_name     = dbg;
        return gv::Image::create(ctx, info);
    };
    auto u_ping = make_field("u_ping");
    auto u_pong = make_field("u_pong");
    auto v_ping = make_field("v_ping");
    auto v_pong = make_field("v_pong");

    // ---- Samplers (raw VkSampler) ------------------------------------------
    auto make_sampler = [&](VkFilter filt, VkSamplerAddressMode addr) {
        VkSamplerCreateInfo sci{};
        sci.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sci.magFilter    = filt;
        sci.minFilter    = filt;
        sci.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sci.addressModeU = addr;
        sci.addressModeV = addr;
        sci.addressModeW = addr;
        sci.maxLod       = 1.0f;
        VkSampler s = VK_NULL_HANDLE;
        if (vkCreateSampler(ctx.device(), &sci, nullptr, &s) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateSampler failed");
        }
        return s;
    };
    VkSampler sampler_nearest = make_sampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    VkSampler sampler_linear  = make_sampler(VK_FILTER_LINEAR,  VK_SAMPLER_ADDRESS_MODE_REPEAT);
    VkSampler sampler_lut     = make_sampler(VK_FILTER_LINEAR,  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    // ---- Colormap LUT (256x4 RGBA8) ---------------------------------------
    auto lut_bytes = colormap::build_lut_data();
    gv::ImageCreateInfo lut_ci{};
    lut_ci.type           = gv::ImageType::e2D;
    lut_ci.extent         = { 256, 4, 1 };
    lut_ci.format         = VK_FORMAT_R8G8B8A8_UNORM;
    lut_ci.usage          = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    lut_ci.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;  // upload() will transition
    lut_ci.debug_name     = "colormap_lut";
    auto lut_tex = gv::Image::create(ctx, lut_ci);
    lut_tex.upload(lut_bytes.data(), lut_bytes.size());

    // ---- Uniform buffers (one per in-flight slot to avoid host/GPU race) ---
    constexpr uint32_t kSlots = gpusims::kMaxFramesInFlight;
    std::array<gv::Buffer, kSlots> rd_uniform_per_slot;
    std::array<gv::Buffer, kSlots> rm_uniform_per_slot;
    for (uint32_t s = 0; s < kSlots; ++s) {
        rd_uniform_per_slot[s] = gv::Buffer::create(
            ctx, sizeof(RdUniformsHost),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            gv::MemoryUsage::HostVisibleSequential,
            "rd_uniform");
        rm_uniform_per_slot[s] = gv::Buffer::create(
            ctx, sizeof(RaymarchUniformsHost),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            gv::MemoryUsage::HostVisibleSequential,
            "rm_uniform");
    }

    // ---- Pipelines ---------------------------------------------------------
    gv::ComputePipelineDesc rd_desc{};
    rd_desc.shader_path = shader_dir / "rd_update.comp.glsl";
    rd_desc.bindings = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1, VK_SHADER_STAGE_COMPUTE_BIT},
        {3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1, VK_SHADER_STAGE_COMPUTE_BIT},
        {4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_COMPUTE_BIT},
    };
    auto rd_pipeline = gv::ComputePipeline::create(ctx, compiler, rd_desc);

    gv::GraphicsPipelineDesc rm_desc{};
    rm_desc.vertex_shader_path   = shader_dir / "fullscreen.vert.glsl";
    rm_desc.fragment_shader_path = shader_dir / "raymarch.frag.glsl";
    rm_desc.color_formats        = { window.colorFormat() };
    rm_desc.cull_mode            = VK_CULL_MODE_NONE;
    rm_desc.depth_test           = false;
    rm_desc.bindings = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_FRAGMENT_BIT},
    };
    auto rm_pipeline = gv::GraphicsPipeline::create(ctx, compiler, rm_desc);

    // ---- Descriptor sets:
    //   Compute: cs[ping_reads ? 0 : 1][slot]
    //   Graphics: gs[v_curr_is_ping ? 0 : 1][slot]
    std::array<std::array<VkDescriptorSet, kSlots>, 2> cs{};
    std::array<std::array<VkDescriptorSet, kSlots>, 2> gs{};
    for (int p = 0; p < 2; ++p) {
        for (uint32_t s = 0; s < kSlots; ++s) {
            cs[p][s] = rd_pipeline.allocateDescriptorSet();
            gs[p][s] = rm_pipeline.allocateDescriptorSet();
        }
    }
    // Wire descriptors. Compute set p=0 reads ping/writes pong; p=1 swaps.
    // Graphics set p=0 reads ping; p=1 reads pong.
    for (uint32_t s = 0; s < kSlots; ++s) {
        writeRdComputeDescriptor(ctx.device(), cs[0][s],
                                 u_ping.view(), v_ping.view(),
                                 u_pong.view(), v_pong.view(),
                                 sampler_nearest, rd_uniform_per_slot[s].handle());
        writeRdComputeDescriptor(ctx.device(), cs[1][s],
                                 u_pong.view(), v_pong.view(),
                                 u_ping.view(), v_ping.view(),
                                 sampler_nearest, rd_uniform_per_slot[s].handle());
        writeRaymarchDescriptor(ctx.device(), gs[0][s],
                                v_ping.view(), lut_tex.view(),
                                sampler_linear, sampler_lut,
                                rm_uniform_per_slot[s].handle());
        writeRaymarchDescriptor(ctx.device(), gs[1][s],
                                v_pong.view(), lut_tex.view(),
                                sampler_linear, sampler_lut,
                                rm_uniform_per_slot[s].handle());
    }

    // ---- Initial conditions -----------------------------------------------
    Runtime rt;
    apply_preset(rt, 0);

    auto reseed = [&]() {
        std::vector<float> u_data(size_t(GRID_SIZE) * GRID_SIZE * GRID_SIZE);
        std::vector<float> v_data(size_t(GRID_SIZE) * GRID_SIZE * GRID_SIZE);
        seed_initial_conditions(u_data, v_data, GRID_SIZE,
                                rt.seedBlockSize, rt.noiseAmp, rt.initSeed);
        // Wait for the GPU to be quiescent before clobbering field memory.
        renderer.waitIdle();
        u_ping.upload(u_data.data(), u_data.size() * sizeof(float));
        v_ping.upload(v_data.data(), v_data.size() * sizeof(float));
        // Clear the pong half so the first read after a swap is sane.
        std::vector<float> zeros(u_data.size(), 0.0f);
        u_pong.upload(zeros.data(), zeros.size() * sizeof(float));
        v_pong.upload(zeros.data(), zeros.size() * sizeof(float));
        rt.iteration = 0;
        logInfo("reseeded fields with init_seed={}", rt.initSeed);
    };
    reseed();

    // ---- Camera ------------------------------------------------------------
    Camera camera;
    camera.setMode(Camera::Mode::Orbit);
    camera.setTarget(glm::vec3(0.0f));
    camera.setOrbitDistance(rt.orbitRadius);
    camera.setOrbitSpeed(rt.orbitSpeedDegPerSec);
    camera.setFovDeg(FOV_DEG_DEFAULT);
    camera.setNearFar(NEAR_PLANE, FAR_PLANE);
    camera.setAspect(window.aspect());

    // ---- Profiler, capture I/O, ImGui --------------------------------------
    GpuProfiler profiler(ctx);
    StateWriter state_writer(fs::current_path() / "captures");
    uint32_t next_capture = 0;

    {
        gpusims::ui::ImGuiInit ic{};
        ic.glfw_window     = window.glfwWindow();
        ic.instance        = ctx.instance();
        ic.physical_device = ctx.physicalDevice();
        ic.device          = ctx.device();
        ic.queue_family    = ctx.graphicsQueueFamily();
        ic.queue           = ctx.graphicsQueue();
        ic.descriptor_pool = VK_NULL_HANDLE;     // common-cpp creates its own
        ic.color_format    = window.colorFormat();
        ic.min_image_count = 2;
        ic.image_count     = window.imageCount();
        if (!gpusims::ui::initImGui(ic)) {
            logError("reaction-diffusion-3d: ui::initImGui failed");
            return 1;
        }
    }

    // ---- Capture helpers ---------------------------------------------------
    auto runtime_meta_json = [&](const Camera& cam) -> json {
        json j;
        j["schemaVersion"]       = 1;
        json cam_j; cam.toJson(cam_j);
        j["camera"]              = cam_j;
        j["presetIndex"]         = rt.presetIndex;
        j["presetName"]          = rt.isCustom ? std::string("Custom")
                                              : std::string(PEARSON_PRESETS[size_t(rt.presetIndex)].name);
        j["F"]                   = rt.F;
        j["k"]                   = rt.k;
        j["Du"]                  = rt.Du;
        j["Dv"]                  = rt.Dv;
        j["gridSize"]            = GRID_SIZE;
        j["substeps"]            = rt.substeps;
        j["iteration"]           = rt.iteration;
        j["raymarchSteps"]       = rt.raymarchSteps;
        j["densityThreshold"]    = rt.densityThreshold;
        j["densityIntensity"]    = rt.densityIntensity;
        j["colormap"]            = rt.colormap;
        j["bloomIntensity"]      = rt.bloomIntensity;
        j["bloomThreshold"]      = 1.0f;
        j["bloomSoftKnee"]       = 0.5f;
        j["exposure"]            = rt.exposure;
        j["autoOrbit"]           = rt.autoOrbit;
        j["orbitSpeedDegPerSec"] = rt.orbitSpeedDegPerSec;
        j["orbitRadius"]         = rt.orbitRadius;
        j["initSeed"]            = rt.initSeed;
        j["seedBlockSize"]       = rt.seedBlockSize;
        j["noiseAmp"]            = rt.noiseAmp;
        j["windowFullscreen"]    = rt.windowFullscreen;
        return j;
    };

    auto save_capture = [&]() {
        // Decide which buffer holds the most-recent valid contents.
        const bool curr_is_ping = (rt.iteration % 2u == 0u);
        gv::Image& u_curr = curr_is_ping ? u_ping : u_pong;
        gv::Image& v_curr = curr_is_ping ? v_ping : v_pong;

        renderer.waitIdle();

        const size_t N = size_t(GRID_SIZE) * GRID_SIZE * GRID_SIZE;
        std::vector<float> u_buf(N);
        std::vector<float> v_buf(N);
        u_curr.readback(u_buf.data(), u_buf.size() * sizeof(float));
        v_curr.readback(v_buf.data(), v_buf.size() * sizeof(float));

        state_writer.beginFrame(next_capture);
        state_writer.setMeta("reactionDiffusion3d", runtime_meta_json(camera));

        json u_meta = {
            {"count",  uint64_t(N)},
            {"stride", uint32_t(sizeof(float))},
            {"format", "r32f"},
            {"shape",  {GRID_SIZE, GRID_SIZE, GRID_SIZE}}
        };
        json v_meta = u_meta;
        state_writer.saveBuffer("u", u_buf.data(), u_buf.size() * sizeof(float), u_meta);
        state_writer.saveBuffer("v", v_buf.data(), v_buf.size() * sizeof(float), v_meta);
        state_writer.endFrame();

        gpusims::ui::pushToast(("Saved capture_" + std::to_string(next_capture)).c_str(), true);
        logInfo("captured capture_{}", next_capture);
        ++next_capture;
    };

    auto load_capture = [&](const fs::path& dir) {
        auto cap = StateReader::open(dir);
        if (!cap) {
            logError("load_capture: cannot open {}", dir.string());
            return;
        }
        auto meta_j = cap->meta("reactionDiffusion3d");
        if (meta_j.is_null()) {
            logError("load_capture: missing reactionDiffusion3d meta in {}", dir.string());
            return;
        }
        auto u_bytes = cap->buffer("u");
        auto v_bytes = cap->buffer("v");
        if (u_bytes.empty() || v_bytes.empty()) {
            logError("load_capture: missing u/v buffers in {}", dir.string());
            return;
        }

        rt.presetIndex     = meta_j.value("presetIndex", rt.presetIndex);
        rt.isCustom        = (rt.presetIndex < 0 || rt.presetIndex >= int(PEARSON_PRESETS.size()));
        rt.F               = meta_j.value("F",  rt.F);
        rt.k               = meta_j.value("k",  rt.k);
        rt.Du              = meta_j.value("Du", rt.Du);
        rt.Dv              = meta_j.value("Dv", rt.Dv);
        rt.substeps        = meta_j.value("substeps", rt.substeps);
        rt.iteration       = meta_j.value("iteration", uint64_t(0));
        rt.raymarchSteps   = meta_j.value("raymarchSteps", rt.raymarchSteps);
        rt.densityThreshold = meta_j.value("densityThreshold", rt.densityThreshold);
        rt.densityIntensity = meta_j.value("densityIntensity", rt.densityIntensity);
        rt.colormap        = meta_j.value("colormap", rt.colormap);
        rt.bloomIntensity  = meta_j.value("bloomIntensity", rt.bloomIntensity);
        rt.exposure        = meta_j.value("exposure", rt.exposure);
        rt.autoOrbit       = meta_j.value("autoOrbit", rt.autoOrbit);
        rt.orbitSpeedDegPerSec = meta_j.value("orbitSpeedDegPerSec", rt.orbitSpeedDegPerSec);
        rt.orbitRadius     = meta_j.value("orbitRadius", rt.orbitRadius);
        rt.initSeed        = meta_j.value("initSeed", rt.initSeed);
        rt.seedBlockSize   = meta_j.value("seedBlockSize", rt.seedBlockSize);
        rt.noiseAmp        = meta_j.value("noiseAmp", rt.noiseAmp);
        if (meta_j.contains("camera")) camera.fromJson(meta_j["camera"]);

        renderer.waitIdle();
        // The capture stored the "current" buffer at save time. Force iteration
        // even so the next-frame parity matches: ping is curr.
        rt.iteration = (rt.iteration % 2u == 0u) ? rt.iteration : (rt.iteration + 1);
        u_ping.upload(u_bytes.data(), u_bytes.size());
        v_ping.upload(v_bytes.data(), v_bytes.size());
        std::vector<float> zeros(size_t(GRID_SIZE) * GRID_SIZE * GRID_SIZE, 0.0f);
        u_pong.upload(zeros.data(), zeros.size() * sizeof(float));
        v_pong.upload(zeros.data(), zeros.size() * sizeof(float));

        gpusims::ui::pushToast(("Loaded " + dir.filename().string()).c_str(), true);
        logInfo("loaded capture {}", dir.string());
    };

    // ---- Hot-reload watches (rebuild flagged; applied inside the frame) ----
    bool reload_compute = false, reload_graphics = false;
    hot.watch(rd_desc.shader_path,            [&](const fs::path&){ reload_compute  = true; });
    hot.watch(rm_desc.vertex_shader_path,     [&](const fs::path&){ reload_graphics = true; });
    hot.watch(rm_desc.fragment_shader_path,   [&](const fs::path&){ reload_graphics = true; });

    // ---- F5/F9 edge tracking ----------------------------------------------
    bool prev_f5 = false, prev_f9 = false;

    // ---- Main loop ---------------------------------------------------------
    auto last = std::chrono::steady_clock::now();

    while (!window.shouldClose()) {
        window.pollEvents();
        hot.poll();

        const auto  now = std::chrono::steady_clock::now();
        const float dt_sec = std::chrono::duration<float>(now - last).count();
        last = now;

        // F5 / F9 keys
        const bool f5 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F5) == GLFW_PRESS;
        const bool f9 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F9) == GLFW_PRESS;
        if (f5 && !prev_f5) save_capture();
        if (f9 && !prev_f9) {
            auto latest = StateReader::findLatest("captures");
            if (latest) load_capture(*latest);
            else        logWarn("F9: no captures found");
        }
        prev_f5 = f5; prev_f9 = f9;

        // Camera
        camera.setMode(rt.autoOrbit ? Camera::Mode::Orbit : Camera::Mode::FreeFly);
        camera.setOrbitDistance(rt.orbitRadius);
        camera.setOrbitSpeed(rt.orbitSpeedDegPerSec);
        camera.setAspect(window.aspect());
        camera.update(dt_sec, g_input.snapshot(window.glfwWindow()));

        // Begin GPU frame
        gv::Frame* frame = renderer.beginFrame();
        if (!frame) continue;

        // Apply pending hot-reloads (pipelines rebuilt; descriptor layouts
        // unchanged so previously-written descriptor sets remain valid).
        if (reload_compute) {
            std::string err;
            if (rd_pipeline.reload(ctx, compiler, *frame, &err)) {
                hot.reportSuccess(rd_desc.shader_path);
                gpusims::ui::pushToast("rd_update reloaded", true);
            } else {
                hot.reportFailure(rd_desc.shader_path, err);
                gpusims::ui::pushToast(("rd reload failed: " + err).substr(0, 256).c_str(), false, 6.0f);
            }
            reload_compute = false;
        }
        if (reload_graphics) {
            std::string err;
            if (rm_pipeline.reload(ctx, compiler, *frame, &err)) {
                hot.reportSuccess(rm_desc.fragment_shader_path);
                gpusims::ui::pushToast("raymarch reloaded", true);
            } else {
                hot.reportFailure(rm_desc.fragment_shader_path, err);
                gpusims::ui::pushToast(("rm reload failed: " + err).substr(0, 256).c_str(), false, 6.0f);
            }
            reload_graphics = false;
        }

        VkCommandBuffer cmd = frame->command_buffer;
        const uint32_t  slot = frame->in_flight_index;

        profiler.beginFrame(cmd, slot);

        // Upload uniforms into this slot's per-slot buffers.
        {
            RdUniformsHost rd{};
            rd.Du       = rt.Du; rd.Dv = rt.Dv; rd.F = rt.F; rd.k = rt.k;
            rd.dt       = SIM_DT_DEFAULT;
            rd.gridSize = GRID_SIZE;
            rd_uniform_per_slot[slot].uploadDirect(&rd, sizeof(rd));
        }
        {
            RaymarchUniformsHost rm{};
            const glm::mat4 view_proj = camera.viewProjection();
            rm.invViewProj      = glm::inverse(view_proj);
            rm.cameraPos        = glm::vec4(camera.position(), 0.0f);
            rm.volumeMin        = glm::vec4(-0.5f, -0.5f, -0.5f, 0.0f);
            rm.volumeMax        = glm::vec4( 0.5f,  0.5f,  0.5f, 0.0f);
            rm.stepCount        = rt.raymarchSteps;
            rm.densityThreshold = rt.densityThreshold;
            rm.densityIntensity = rt.densityIntensity;
            rm.colormapIndex    = float(rt.colormap);
            rm.exposure         = rt.exposure;
            rm.bloomIntensity   = rt.bloomIntensity;
            rm_uniform_per_slot[slot].uploadDirect(&rm, sizeof(rm));
        }

        // ---- Frame-boundary barrier (per § 2.15 — self-documenting; mostly
        // redundant with the queue-submit serialization). Last frame's
        // raymarch fragment-stage sampled-reads must complete before this
        // frame's first substep storage-writes touch the field images.
        gv::memoryBarrier(cmd,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);

        // ---- RD substeps (compute) ----
        for (int s = 0; s < rt.substeps; ++s) {
            auto _scope = profiler.scope(cmd, "substep");
            const bool ping_reads = (rt.iteration % 2u == 0u);
            const int  set_idx    = ping_reads ? 0 : 1;
            rd_pipeline.dispatch(cmd, cs[set_idx][slot],
                                 GRID_SIZE / WG_DIM,
                                 GRID_SIZE / WG_DIM,
                                 GRID_SIZE / WG_DIM);
            // Inter-substep barrier (compute write → compute sampled read).
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
            ++rt.iteration;
        }

        // ---- Substep-to-raymarch barrier (compute write → fragment read) --
        gv::memoryBarrier(cmd,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);

        // ---- Raymarch + ImGui (graphics pass) -----------------------------
        const bool curr_is_ping = (rt.iteration % 2u == 0u);
        const int  g_set_idx    = curr_is_ping ? 0 : 1;

        // Build the ImGui frame BEFORE beginRendering so ImGui draws come last
        // in the pass (after the raymarch). Same pattern as hello-world.
        gpusims::ui::newImGuiFrame();
        ImGui::SetNextWindowPos(ImVec2(10, 10),    ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(380, 720), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("reaction-diffusion-3d")) {
            const char* current_label =
                rt.isCustom ? "Custom" : PEARSON_PRESETS[size_t(rt.presetIndex)].name;
            if (ImGui::BeginCombo("Preset", current_label)) {
                for (size_t i = 0; i < PEARSON_PRESETS.size(); ++i) {
                    const bool selected = (!rt.isCustom && int(i) == rt.presetIndex);
                    if (ImGui::Selectable(PEARSON_PRESETS[i].name, selected)) {
                        apply_preset(rt, int(i));
                        reseed();
                    }
                }
                if (ImGui::Selectable("Custom", rt.isCustom)) rt.isCustom = true;
                ImGui::EndCombo();
            }
            if (ImGui::Button("Reset state to preset IC")) {
                if (!rt.isCustom) apply_preset(rt, rt.presetIndex);
                reseed();
            }

            if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool changed = false;
                changed |= ImGui::SliderFloat("F",  &rt.F,  0.0f,   0.10f, "%.4f");
                changed |= ImGui::SliderFloat("k",  &rt.k,  0.030f, 0.080f, "%.4f");
                changed |= ImGui::SliderFloat("Du", &rt.Du, 0.0f,   1.0f,   "%.3f");
                changed |= ImGui::SliderFloat("Dv", &rt.Dv, 0.0f,   1.0f,   "%.3f");
                if (changed) rt.isCustom = true;
            }

            if (ImGui::CollapsingHeader("Integration")) {
                ImGui::SliderInt("substeps", &rt.substeps, 1, 32);
                int seed_int = int(rt.initSeed);
                if (ImGui::InputInt("initSeed", &seed_int, 1, 1000)) {
                    rt.initSeed = uint32_t(std::max(0, seed_int));
                }
                int blk = int(rt.seedBlockSize);
                if (ImGui::SliderInt("seedBlockSize", &blk, 4, 64)) {
                    rt.seedBlockSize = uint32_t(blk);
                }
                ImGui::SliderFloat("noiseAmp", &rt.noiseAmp, 0.0f, 0.3f, "%.3f");
                if (ImGui::Button("Reseed")) reseed();
                ImGui::Text("iteration: %llu", static_cast<unsigned long long>(rt.iteration));
            }

            if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::SliderInt  ("raymarchSteps",   &rt.raymarchSteps,   16, 256);
                ImGui::SliderFloat("densityThreshold",&rt.densityThreshold, 0.0f, 1.0f, "%.3f");
                ImGui::SliderFloat("densityIntensity",&rt.densityIntensity, 0.0f, 4.0f, "%.3f");
                ImGui::SliderFloat("exposure",        &rt.exposure,         0.1f, 4.0f, "%.3f");
            }

            if (ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen)) {
                static const char* COLORMAP_NAMES[] = {"magma", "inferno", "viridis", "hsv"};
                ImGui::Combo("Colormap", &rt.colormap,
                             COLORMAP_NAMES, IM_ARRAYSIZE(COLORMAP_NAMES));
            }

            if (ImGui::CollapsingHeader("Camera")) {
                ImGui::Checkbox  ("Auto-orbit",          &rt.autoOrbit);
                ImGui::SliderFloat("orbitSpeedDegPerSec",&rt.orbitSpeedDegPerSec, 0.0f, 60.0f, "%.1f");
                ImGui::SliderFloat("orbitRadius",        &rt.orbitRadius,         0.5f, 5.0f, "%.2f");
            }

            if (ImGui::CollapsingHeader("State", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::Button("Save (F5)")) save_capture();
                ImGui::SameLine();
                if (ImGui::Button("Load most recent (F9)")) {
                    auto latest = StateReader::findLatest("captures");
                    if (latest) load_capture(*latest);
                }
            }
        }
        ImGui::End();
        profiler.drawImGui();
        gpusims::ui::drawToasts();

        renderer.beginRendering(*frame, VkClearColorValue{{0.0f, 0.0f, 0.0f, 1.0f}});
        {
            auto _scope = profiler.scope(cmd, "raymarch");
            rm_pipeline.bind(cmd, gs[g_set_idx][slot]);
            vkCmdDraw(cmd, 3, 1, 0, 0);
        }
        {
            auto _scope = profiler.scope(cmd, "imgui");
            gpusims::ui::renderImGui(cmd);
        }
        renderer.endRendering(*frame);

        profiler.endFrame(cmd);
        renderer.endFrame(*frame);
    }

    // ---- Shutdown ----------------------------------------------------------
    renderer.waitIdle();
    gpusims::ui::shutdownImGui();
    if (sampler_nearest) vkDestroySampler(ctx.device(), sampler_nearest, nullptr);
    if (sampler_linear)  vkDestroySampler(ctx.device(), sampler_linear,  nullptr);
    if (sampler_lut)     vkDestroySampler(ctx.device(), sampler_lut,     nullptr);
    logInfo("reaction-diffusion-3d: clean shutdown");
    return 0;
}
