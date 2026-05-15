// Lattice Boltzmann (D3Q19 BGK) around a NACA airfoil.
// Stack C / Vulkan 1.3 / common-cpp consumer.
//
// Phase 12. See docs/phase12_lattice_boltzmann.md for the design spec.
// References:
//   - tools/integrity/docs/algebraic/d3q19.md  (lattice constants)
//   - references/lbm-principles-practice/chapter13/cpu/LBM.cpp:97  (BGK pattern)
//   - references/lbm-principles-practice/chapter5/poiseuille_BB.m:123  (halfway BB pattern)
//
// This binary is the second Stack C consumer of OpenVDB writeVec3* (first
// real-data consumer of writeVec3Grid). VDB export is gated at compile time by
// -DGPU_SIMS_USE_OPENVDB=ON; in stub mode the toggle is a no-op.

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <gpusims/camera.hpp>
#include <gpusims/gpu_profiler.hpp>
#include <gpusims/hot_reload.hpp>
#include <gpusims/imgui_setup.hpp>
#include <gpusims/log.hpp>
#include <gpusims/state_reader.hpp>
#include <gpusims/state_writer.hpp>
#include <gpusims/vdb_writer.hpp>
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
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace fs  = std::filesystem;
namespace gv  = gpusims::vk;
namespace vdb = gpusims::vdb;
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

constexpr uint32_t NUM_DIRS         = 19;
constexpr uint32_t NUM_NONREST      = 18;
constexpr uint32_t WG_DIM_X         = 8;
constexpr uint32_t WG_DIM_Y         = 8;
constexpr uint32_t WG_DIM_Z         = 4;
constexpr uint32_t WG_DIM_STREAMLINE = 64;

constexpr uint32_t STREAMLINE_COUNT_DEFAULT = 10000;
constexpr uint32_t STREAMLINE_HISTORY       = 64;
constexpr int      STREAMLINE_RESEED_AGE    = 256;

constexpr float TAU_DEFAULT = 0.6f;
constexpr float U_INF_MAX   = 0.1f;
constexpr float RHO_0       = 1.0f;

constexpr int   SUBSTEPS_DEFAULT = 1;
constexpr int   SUBSTEPS_MIN     = 1;
constexpr int   SUBSTEPS_MAX     = 16;

constexpr float FOV_DEG_DEFAULT = 55.0f;
constexpr float NEAR_PLANE      = 0.05f;
constexpr float FAR_PLANE       = 4000.0f;

constexpr bool RENDER_VELMAG_DEFAULT      = true;
constexpr bool RENDER_STREAMLINES_DEFAULT = true;

constexpr float AIRFOIL_CHORD_CENTER_X_FRAC = 0.25f;
constexpr float AIRFOIL_CHORD_LENGTH_FRAC   = 0.25f;

constexpr int VDB_EVERY_N_FRAMES_DEFAULT = 4;

constexpr int NUM_TIERS = 3;
struct Tier {
    const char* label;
    uint32_t    Nx;
    uint32_t    Ny;
    uint32_t    Nz;
    const char* note;
};
constexpr std::array<Tier, NUM_TIERS> TIERS = {{
    {"128^3 (Laptop)",        128, 128, 128, "Laptop / iGPU"},
    {"256x128x128 (Desktop)", 256, 128, 128, "RX 6800 XT default"},
    {"512x256x256 (Capture)", 512, 256, 256, "Hero render; ~5 FPS"},
}};
constexpr int DEFAULT_TIER_INDEX = 1;

// ============================================================================
// Presets
// ============================================================================

struct Preset {
    const char* label;
    const char* naca_designation;
    float       angle_of_attack_deg;
    float       u_inf;
    float       tau;
};
constexpr int NUM_PRESETS = 3;
constexpr std::array<Preset, NUM_PRESETS> PRESETS = {{
    {"NACA0012 - Low-Re", "0012", 4.0f, 0.04f, 0.60f},
    {"NACA0012 - Med-Re", "0012", 8.0f, 0.06f, 0.55f},
    {"NACA4412 - Med-Re", "4412", 6.0f, 0.06f, 0.55f},
}};
constexpr int DEFAULT_PRESET_INDEX = 0;

// ============================================================================
// Reserved-for-v1.1 emitter type. Kept alive for capture-format compatibility
// with future versions; never populated in v1.
// ============================================================================
struct Emitter {
    glm::vec3 pos;
    glm::vec3 velocity;
    float     emissionRate;
    float     radius;
    float     ageSec;
};

// ============================================================================
// Runtime state
// ============================================================================

struct Runtime {
    int      tierIndex          = DEFAULT_TIER_INDEX;
    int      pendingTierIndex   = DEFAULT_TIER_INDEX;
    uint32_t Nx = TIERS[DEFAULT_TIER_INDEX].Nx;
    uint32_t Ny = TIERS[DEFAULT_TIER_INDEX].Ny;
    uint32_t Nz = TIERS[DEFAULT_TIER_INDEX].Nz;
    uint64_t totalCells = uint64_t(Nx) * Ny * Nz;

    int      presetIndex        = DEFAULT_PRESET_INDEX;
    uint64_t iteration          = 0;

    int       substeps          = SUBSTEPS_DEFAULT;
    float     tau               = TAU_DEFAULT;
    float     uInfMagnitude     = 0.04f;
    float     angleOfAttackDeg  = 4.0f;
    glm::vec3 uInf              = glm::vec3(0.04f, 0.0f, 0.0f);

    bool      renderVelmag      = RENDER_VELMAG_DEFAULT;
    bool      renderStreamlines = RENDER_STREAMLINES_DEFAULT;

    uint32_t  streamlineCount       = STREAMLINE_COUNT_DEFAULT;
    uint32_t  streamlineHistory     = STREAMLINE_HISTORY;
    uint32_t  streamlineFrameIndex  = 0;

    bool      exportVdb         = false;
    int       vdbEveryNFrames   = VDB_EVERY_N_FRAMES_DEFAULT;

    float     lastFrameMs       = 16.0f;
    float     currentRe         = 0.0f;

    int       raymarchSteps     = 128;
    float     velmagMin         = 0.0f;
    float     velmagMax         = 0.1f;
    float     velmagAbsorption  = 4.0f;
    float     exposure          = 1.5f;

    bool      pendingLoadFromF9 = false;
    bool      streamlineRealloc = false;

    std::vector<Emitter> emitters;
};

static Runtime rt;

// ============================================================================
// NACA 4-digit airfoil geometry. Reference: NACA Report 460.
// ============================================================================
namespace naca {

struct AirfoilParams {
    float M;
    float P;
    float T;
};

inline AirfoilParams parse(const char* designation) {
    AirfoilParams p{};
    int m  = (designation[0] - '0');
    int pp = (designation[1] - '0');
    int t  = (designation[2] - '0') * 10 + (designation[3] - '0');
    p.M = float(m)  / 100.0f;
    p.P = float(pp) / 10.0f;
    p.T = float(t)  / 100.0f;
    return p;
}

inline float thickness(float x, float T) {
    return 5.0f * T * (
        0.2969f * std::sqrt(x)
      - 0.1260f * x
      - 0.3516f * x * x
      + 0.2843f * x * x * x
      - 0.1015f * x * x * x * x);
}

inline glm::vec2 camber_and_slope(float x, float M, float P) {
    if (M <= 0.0f) return glm::vec2(0.0f, 0.0f);
    if (x <= P) {
        float yc  = (M / (P * P)) * (2.0f * P * x - x * x);
        float dyc = (M / (P * P)) * (2.0f * P - 2.0f * x);
        return glm::vec2(yc, dyc);
    } else {
        float inv = 1.0f / ((1.0f - P) * (1.0f - P));
        float yc  = M * inv * ((1.0f - 2.0f * P) + 2.0f * P * x - x * x);
        float dyc = M * inv * (2.0f * P - 2.0f * x);
        return glm::vec2(yc, dyc);
    }
}

inline std::pair<glm::vec2, glm::vec2> surface_points(float x, const AirfoilParams& p) {
    float yt = thickness(x, p.T);
    glm::vec2 cs = camber_and_slope(x, p.M, p.P);
    float theta = std::atan(cs.y);
    glm::vec2 upper = glm::vec2(x - yt * std::sin(theta), cs.x + yt * std::cos(theta));
    glm::vec2 lower = glm::vec2(x + yt * std::sin(theta), cs.x - yt * std::cos(theta));
    return {upper, lower};
}

// Voxelize the airfoil into the supplied mask. `leading_edge_xy` parameter name
// reflects what the math expects: the leading edge of the airfoil in pixel
// coordinates. mask is expected to be sized Nx*Ny*Nz, x-fastest.
void voxelize_into(std::vector<uint8_t>& mask,
                   uint32_t Nx, uint32_t Ny, uint32_t Nz,
                   const AirfoilParams& p,
                   float chord_pixels,
                   float alpha_rad,
                   glm::vec2 leading_edge_xy)
{
    constexpr int N_SAMPLES = 256;
    std::vector<glm::vec2> perimeter;
    perimeter.reserve(2 * size_t(N_SAMPLES));
    for (int i = 0; i < N_SAMPLES; ++i) {
        float x = float(i) / float(N_SAMPLES - 1);
        auto pr = surface_points(x, p);
        perimeter.push_back(pr.first);
    }
    for (int i = N_SAMPLES - 1; i >= 0; --i) {
        float x = float(i) / float(N_SAMPLES - 1);
        auto pr = surface_points(x, p);
        perimeter.push_back(pr.second);
    }

    float c = std::cos(alpha_rad);
    float s = std::sin(alpha_rad);
    for (auto& pt : perimeter) {
        float x_pix = pt.x * chord_pixels;
        float y_pix = pt.y * chord_pixels;
        float xr = c * x_pix - s * y_pix;
        float yr = s * x_pix + c * y_pix;
        pt = glm::vec2(xr + leading_edge_xy.x, yr + leading_edge_xy.y);
    }

    auto inside = [&](float px, float py) -> bool {
        int wn = 0;
        for (size_t i = 0; i < perimeter.size(); ++i) {
            const auto& a = perimeter[i];
            const auto& b = perimeter[(i + 1) % perimeter.size()];
            if (a.y <= py) {
                if (b.y > py && (b.x - a.x) * (py - a.y) - (px - a.x) * (b.y - a.y) > 0.0f)
                    ++wn;
            } else {
                if (b.y <= py && (b.x - a.x) * (py - a.y) - (px - a.x) * (b.y - a.y) < 0.0f)
                    --wn;
            }
        }
        return wn != 0;
    };

    std::fill(mask.begin(), mask.end(), uint8_t(0));
    for (uint32_t y = 0; y < Ny; ++y) {
        for (uint32_t x = 0; x < Nx; ++x) {
            bool in = inside(float(x) + 0.5f, float(y) + 0.5f);
            if (in) {
                for (uint32_t z = 0; z < Nz; ++z) {
                    mask[x + size_t(Nx) * (y + size_t(Ny) * z)] = 1;
                }
            }
        }
    }
}

}  // namespace naca

// ============================================================================
// Half-precision -> float (IEEE 754 binary16 -> binary32). Used to convert
// rgba16f velocity readback into the float layout writeVec3Grid expects.
// ============================================================================
static inline float half_to_float(uint16_t h) {
    uint32_t s = (uint32_t(h) & 0x8000u) << 16;
    uint32_t e = (uint32_t(h) >> 10) & 0x1Fu;
    uint32_t m = uint32_t(h) & 0x3FFu;
    if (e == 0) {
        if (m == 0) return std::bit_cast<float>(s);
        while (!(m & 0x400u)) {
            m <<= 1;
            e = uint32_t(int(e) - 1);
        }
        e += 1;
        m &= 0x3FFu;
    } else if (e == 31) {
        return std::bit_cast<float>(s | 0x7F800000u | (m << 13));
    }
    uint32_t f = s | ((e + 112u) << 23) | (m << 13);
    return std::bit_cast<float>(f);
}

// ============================================================================
// GLSL uniform layouts. Match the shaders byte-for-byte.
// ============================================================================

struct alignas(16) InitUniforms {
    glm::ivec4 dims;
    float      tau_inv;
    float      omtau_inv;
    float      _pad0;
    float      _pad1;
};

struct alignas(16) CollideUniforms {
    glm::ivec4 dims;
    float      tau_inv;
    float      omtau_inv;
    float      _pad0;
    float      _pad1;
};

struct alignas(16) StreamUniforms {
    glm::ivec4 dims;
};

struct alignas(16) BoundariesUniforms {
    glm::ivec4 dims;
    glm::vec4  u_inf;
    glm::ivec4 inlet_axis;
};

struct alignas(16) MomentsUniforms {
    glm::ivec4 dims;
};

struct alignas(16) StreamlineAdvectUniforms {
    glm::ivec4 dims;
    glm::vec4  domain_min;
    glm::vec4  domain_max;
    uint32_t   streamline_count;
    uint32_t   history;
    uint32_t   head_index;
    uint32_t   frame_count;
    float      dt_render;
    uint32_t   reseed_age_threshold;
    uint32_t   _pad0;
    uint32_t   _pad1;
};

struct alignas(16) RaymarchUniforms {
    glm::mat4 invViewProj;
    glm::vec4 cameraPos;
    glm::vec4 volumeMin;
    glm::vec4 volumeMax;
    glm::vec4 volumeAspect;
    int32_t   raymarchSteps;
    int32_t   _pad0;
    float     velmagAbsorption;
    float     velmagMin;
    float     velmagMax;
    float     exposure;
    float     _pad1;
    float     _pad2;
};

struct alignas(16) StreamlineRenderUniforms {
    glm::mat4 viewProj;
    glm::vec4 lineColor;
    uint32_t  history;
    uint32_t  head_index;
    float     ageFalloff;
    float     _pad0;
};

// ============================================================================
// Input snapshot (mirrors ES InputState).
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
// Image creation helpers.
// ============================================================================

static gv::Image make_3d_r32f(gv::Context& ctx, uint32_t Nx, uint32_t Ny, uint32_t Nz,
                              const char* dbg) {
    gv::ImageCreateInfo i{};
    i.type           = gv::ImageType::e3D;
    i.extent         = {Nx, Ny, Nz};
    i.format         = VK_FORMAT_R32_SFLOAT;
    i.usage          = VK_IMAGE_USAGE_STORAGE_BIT
                     | VK_IMAGE_USAGE_SAMPLED_BIT
                     | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                     | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    i.initial_layout = VK_IMAGE_LAYOUT_GENERAL;
    i.debug_name     = dbg;
    return gv::Image::create(ctx, i);
}
static gv::Image make_3d_rgba16f(gv::Context& ctx, uint32_t Nx, uint32_t Ny, uint32_t Nz,
                                 const char* dbg) {
    gv::ImageCreateInfo i{};
    i.type           = gv::ImageType::e3D;
    i.extent         = {Nx, Ny, Nz};
    i.format         = VK_FORMAT_R16G16B16A16_SFLOAT;
    i.usage          = VK_IMAGE_USAGE_STORAGE_BIT
                     | VK_IMAGE_USAGE_SAMPLED_BIT
                     | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                     | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    i.initial_layout = VK_IMAGE_LAYOUT_GENERAL;
    i.debug_name     = dbg;
    return gv::Image::create(ctx, i);
}
static gv::Image make_3d_r8uint(gv::Context& ctx, uint32_t Nx, uint32_t Ny, uint32_t Nz,
                                const char* dbg) {
    gv::ImageCreateInfo i{};
    i.type           = gv::ImageType::e3D;
    i.extent         = {Nx, Ny, Nz};
    i.format         = VK_FORMAT_R8_UINT;
    i.usage          = VK_IMAGE_USAGE_STORAGE_BIT
                     | VK_IMAGE_USAGE_SAMPLED_BIT
                     | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                     | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    i.initial_layout = VK_IMAGE_LAYOUT_GENERAL;
    i.debug_name     = dbg;
    return gv::Image::create(ctx, i);
}
static gv::Image make_2d_r8(gv::Context& ctx, uint32_t w, uint32_t h, const char* dbg) {
    gv::ImageCreateInfo i{};
    i.type           = gv::ImageType::e2D;
    i.extent         = {w, h, 1};
    i.format         = VK_FORMAT_R8_UNORM;
    i.usage          = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    i.initial_layout = VK_IMAGE_LAYOUT_GENERAL;
    i.debug_name     = dbg;
    return gv::Image::create(ctx, i);
}
static gv::Image make_2d_rgba8(gv::Context& ctx, uint32_t w, uint32_t h, const char* dbg) {
    gv::ImageCreateInfo i{};
    i.type           = gv::ImageType::e2D;
    i.extent         = {w, h, 1};
    i.format         = VK_FORMAT_R8G8B8A8_UNORM;
    i.usage          = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    i.initial_layout = VK_IMAGE_LAYOUT_GENERAL;
    i.debug_name     = dbg;
    return gv::Image::create(ctx, i);
}

// ============================================================================
// Colormap LUT (256x4 rgba8). Row 0 = viridis-ish ramp for velocity magnitude.
// ============================================================================

static std::vector<uint8_t> build_colormap_lut() {
    std::vector<uint8_t> out(256u * 4u * 4u);
    auto sample = [](float t) -> std::array<uint8_t, 3> {
        // Simple cool-to-warm ramp: dark blue -> cyan -> green -> yellow -> red.
        float r, g, b;
        if (t < 0.25f) {
            float f = t / 0.25f;
            r = 0.05f;            g = 0.05f + 0.6f * f;  b = 0.4f + 0.6f * f;
        } else if (t < 0.5f) {
            float f = (t - 0.25f) / 0.25f;
            r = 0.05f;            g = 0.65f + 0.3f * f;  b = 1.0f - 0.7f * f;
        } else if (t < 0.75f) {
            float f = (t - 0.5f) / 0.25f;
            r = 0.05f + 0.95f * f; g = 0.95f - 0.05f * f; b = 0.3f - 0.25f * f;
        } else {
            float f = (t - 0.75f) / 0.25f;
            r = 1.0f;             g = 0.9f - 0.7f * f;  b = 0.05f;
        }
        return {uint8_t(std::clamp(r, 0.0f, 1.0f) * 255.0f),
                uint8_t(std::clamp(g, 0.0f, 1.0f) * 255.0f),
                uint8_t(std::clamp(b, 0.0f, 1.0f) * 255.0f)};
    };
    for (int row = 0; row < 4; ++row) {
        for (int i = 0; i < 256; ++i) {
            float t = float(i) / 255.0f;
            auto rgb = sample(t);
            int o = (row * 256 + i) * 4;
            out[o + 0] = rgb[0];
            out[o + 1] = rgb[1];
            out[o + 2] = rgb[2];
            out[o + 3] = 255;
        }
    }
    return out;
}

static std::vector<uint8_t> build_jitter_lut() {
    std::vector<uint8_t> out(256u * 256u);
    uint32_t state = 0x9E3779B9u;
    for (size_t i = 0; i < out.size(); ++i) {
        state = state * 1664525u + 1013904223u;
        out[i] = uint8_t((state >> 24) & 0xFFu);
    }
    return out;
}

// ============================================================================
// Descriptor write helpers.
// Each helper packs a vkUpdateDescriptorSets call for one pipeline.
// ============================================================================

static void writeInitDescriptor(VkDevice dev, VkDescriptorSet ds,
                                const std::array<VkImageView, NUM_NONREST>& f_nonrest_views,
                                VkImageView f_rest_view,
                                VkImageView rho_view,
                                VkImageView vel_view,
                                VkBuffer ub) {
    std::array<VkDescriptorImageInfo, NUM_NONREST> nr_info{};
    for (uint32_t i = 0; i < NUM_NONREST; ++i) {
        nr_info[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        nr_info[i].imageView   = f_nonrest_views[i];
        nr_info[i].sampler     = VK_NULL_HANDLE;
    }
    VkDescriptorImageInfo r_i{}; r_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; r_i.imageView = f_rest_view;
    VkDescriptorImageInfo rho_i{}; rho_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; rho_i.imageView = rho_view;
    VkDescriptorImageInfo vel_i{}; vel_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; vel_i.imageView = vel_view;
    VkDescriptorBufferInfo ub_i{}; ub_i.buffer = ub; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = NUM_NONREST;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[0].pImageInfo = nr_info.data();
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[1].pImageInfo = &r_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[2].pImageInfo = &rho_i;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[3].pImageInfo = &vel_i;
    w[4].dstSet = ds; w[4].dstBinding = 4; w[4].descriptorCount = 1;
    w[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(dev, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeCollideDescriptor(VkDevice dev, VkDescriptorSet ds,
                                   const std::array<VkImageView, NUM_NONREST>& f_nonrest_views,
                                   VkImageView f_rest_view,
                                   VkImageView rho_view,
                                   VkImageView vel_view,
                                   VkSampler sampler_lin,
                                   VkBuffer ub) {
    std::array<VkDescriptorImageInfo, NUM_NONREST> nr_info{};
    for (uint32_t i = 0; i < NUM_NONREST; ++i) {
        nr_info[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        nr_info[i].imageView   = f_nonrest_views[i];
        nr_info[i].sampler     = VK_NULL_HANDLE;
    }
    VkDescriptorImageInfo r_i{}; r_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; r_i.imageView = f_rest_view;
    VkDescriptorImageInfo rho_i{}; rho_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; rho_i.imageView = rho_view; rho_i.sampler = sampler_lin;
    VkDescriptorImageInfo vel_i{}; vel_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; vel_i.imageView = vel_view; vel_i.sampler = sampler_lin;
    VkDescriptorBufferInfo ub_i{}; ub_i.buffer = ub; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = NUM_NONREST;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[0].pImageInfo = nr_info.data();
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[1].pImageInfo = &r_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[2].pImageInfo = &rho_i;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[3].pImageInfo = &vel_i;
    w[4].dstSet = ds; w[4].dstBinding = 4; w[4].descriptorCount = 1;
    w[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(dev, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeStreamDescriptor(VkDevice dev, VkDescriptorSet ds,
                                  const std::array<VkImageView, NUM_NONREST>& f_in_nonrest_views,
                                  VkImageView f_in_rest_view,
                                  const std::array<VkImageView, NUM_NONREST>& f_out_nonrest_views,
                                  VkImageView f_out_rest_view,
                                  VkSampler sampler_pt,
                                  VkBuffer ub) {
    std::array<VkDescriptorImageInfo, NUM_NONREST> in_info{};
    std::array<VkDescriptorImageInfo, NUM_NONREST> out_info{};
    for (uint32_t i = 0; i < NUM_NONREST; ++i) {
        in_info[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        in_info[i].imageView   = f_in_nonrest_views[i];
        in_info[i].sampler     = sampler_pt;
        out_info[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        out_info[i].imageView   = f_out_nonrest_views[i];
        out_info[i].sampler     = VK_NULL_HANDLE;
    }
    VkDescriptorImageInfo ir_i{}; ir_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; ir_i.imageView = f_in_rest_view; ir_i.sampler = sampler_pt;
    VkDescriptorImageInfo or_i{}; or_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; or_i.imageView = f_out_rest_view;
    VkDescriptorBufferInfo ub_i{}; ub_i.buffer = ub; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = NUM_NONREST;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[0].pImageInfo = in_info.data();
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[1].pImageInfo = &ir_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = NUM_NONREST;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[2].pImageInfo = out_info.data();
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[3].pImageInfo = &or_i;
    w[4].dstSet = ds; w[4].dstBinding = 4; w[4].descriptorCount = 1;
    w[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(dev, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeBoundariesDescriptor(VkDevice dev, VkDescriptorSet ds,
                                      const std::array<VkImageView, NUM_NONREST>& f_nonrest_views,
                                      VkImageView f_rest_view,
                                      VkImageView mask_view,
                                      VkSampler sampler_pt,
                                      VkBuffer ub) {
    std::array<VkDescriptorImageInfo, NUM_NONREST> nr_info{};
    for (uint32_t i = 0; i < NUM_NONREST; ++i) {
        nr_info[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        nr_info[i].imageView   = f_nonrest_views[i];
        nr_info[i].sampler     = VK_NULL_HANDLE;
    }
    VkDescriptorImageInfo r_i{}; r_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; r_i.imageView = f_rest_view;
    VkDescriptorImageInfo m_i{}; m_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; m_i.imageView = mask_view; m_i.sampler = sampler_pt;
    VkDescriptorBufferInfo ub_i{}; ub_i.buffer = ub; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = NUM_NONREST;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[0].pImageInfo = nr_info.data();
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[1].pImageInfo = &r_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[2].pImageInfo = &m_i;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[3].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(dev, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeMomentsDescriptor(VkDevice dev, VkDescriptorSet ds,
                                   const std::array<VkImageView, NUM_NONREST>& f_nonrest_views,
                                   VkImageView f_rest_view,
                                   VkImageView rho_view,
                                   VkImageView vel_view,
                                   VkSampler sampler_pt,
                                   VkBuffer ub) {
    std::array<VkDescriptorImageInfo, NUM_NONREST> nr_info{};
    for (uint32_t i = 0; i < NUM_NONREST; ++i) {
        nr_info[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        nr_info[i].imageView   = f_nonrest_views[i];
        nr_info[i].sampler     = sampler_pt;
    }
    VkDescriptorImageInfo r_i{}; r_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; r_i.imageView = f_rest_view; r_i.sampler = sampler_pt;
    VkDescriptorImageInfo rho_i{}; rho_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; rho_i.imageView = rho_view;
    VkDescriptorImageInfo vel_i{}; vel_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; vel_i.imageView = vel_view;
    VkDescriptorBufferInfo ub_i{}; ub_i.buffer = ub; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = NUM_NONREST;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[0].pImageInfo = nr_info.data();
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[1].pImageInfo = &r_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[2].pImageInfo = &rho_i;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[3].pImageInfo = &vel_i;
    w[4].dstSet = ds; w[4].dstBinding = 4; w[4].descriptorCount = 1;
    w[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(dev, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeStreamlineAdvectDescriptor(VkDevice dev, VkDescriptorSet ds,
                                            VkImageView vel_view,
                                            VkSampler sampler_lin,
                                            VkBuffer pos_buf,
                                            VkBuffer ub) {
    VkDescriptorImageInfo v_i{}; v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; v_i.imageView = vel_view; v_i.sampler = sampler_lin;
    VkDescriptorBufferInfo p_i{}; p_i.buffer = pos_buf; p_i.offset = 0; p_i.range = VK_WHOLE_SIZE;
    VkDescriptorBufferInfo ub_i{}; ub_i.buffer = ub; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo = &p_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[2].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(dev, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeRaymarchDescriptor(VkDevice dev, VkDescriptorSet ds,
                                    VkImageView vel_view,
                                    VkImageView lut_view,
                                    VkImageView jitter_view,
                                    VkSampler sampler_lin,
                                    VkSampler sampler_lut,
                                    VkBuffer ub) {
    VkDescriptorImageInfo v_i{}; v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; v_i.imageView = vel_view; v_i.sampler = sampler_lin;
    VkDescriptorImageInfo l_i{}; l_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; l_i.imageView = lut_view; l_i.sampler = sampler_lut;
    VkDescriptorImageInfo j_i{}; j_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL; j_i.imageView = jitter_view; j_i.sampler = sampler_lut;
    VkDescriptorBufferInfo ub_i{}; ub_i.buffer = ub; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[1].pImageInfo = &l_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w[2].pImageInfo = &j_i;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[3].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(dev, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeStreamlineRenderDescriptor(VkDevice dev, VkDescriptorSet ds,
                                            VkBuffer pos_buf, VkBuffer ub) {
    VkDescriptorBufferInfo p_i{}; p_i.buffer = pos_buf; p_i.offset = 0; p_i.range = VK_WHOLE_SIZE;
    VkDescriptorBufferInfo ub_i{}; ub_i.buffer = ub; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo = &p_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[1].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(dev, uint32_t(w.size()), w.data(), 0, nullptr);
}

// ============================================================================
// main()
// ============================================================================

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    initLogger();

    gv::ContextCreateInfo ci{};
    ci.application_name             = "lattice_boltzmann";
    ci.enable_swapchain             = true;
    ci.enable_subgroup_size_control = true;
    gv::Context ctx(ci);

    gv::Window   window(ctx, 1920, 1080, "GPU-Sims - Lattice Boltzmann");
    gv::Renderer renderer(ctx, window);
    gv::ShaderCompiler compiler(ctx);
    compiler.addIncludeDir(GPU_SIMS_LBM_SHADER_DIR);

    constexpr uint32_t kSlots    = gpusims::kMaxFramesInFlight;
    constexpr uint32_t kParities = 2;

    // ------------------------------------------------------------------------
    // Tier-resident images. Recreated on tier change.
    // ------------------------------------------------------------------------
    std::array<gv::Image, NUM_NONREST> f_ping_nonrest;
    std::array<gv::Image, NUM_NONREST> f_pong_nonrest;
    gv::Image f_rest_ping;
    gv::Image f_rest_pong;
    gv::Image rho_image;
    gv::Image velocity_image;
    gv::Image obstacle_mask;

    auto allocate_tier_images = [&]() {
        for (uint32_t i = 0; i < NUM_NONREST; ++i) {
            f_ping_nonrest[i] = make_3d_r32f(ctx, rt.Nx, rt.Ny, rt.Nz, "f_ping_nonrest");
            f_pong_nonrest[i] = make_3d_r32f(ctx, rt.Nx, rt.Ny, rt.Nz, "f_pong_nonrest");
        }
        f_rest_ping     = make_3d_r32f   (ctx, rt.Nx, rt.Ny, rt.Nz, "f_rest_ping");
        f_rest_pong     = make_3d_r32f   (ctx, rt.Nx, rt.Ny, rt.Nz, "f_rest_pong");
        rho_image       = make_3d_r32f   (ctx, rt.Nx, rt.Ny, rt.Nz, "rho");
        velocity_image  = make_3d_rgba16f(ctx, rt.Nx, rt.Ny, rt.Nz, "velocity");
        obstacle_mask   = make_3d_r8uint (ctx, rt.Nx, rt.Ny, rt.Nz, "obstacle_mask");
    };
    allocate_tier_images();

    // ------------------------------------------------------------------------
    // Streamline buffers.
    // ------------------------------------------------------------------------
    gv::Buffer streamline_positions;
    gv::Buffer streamline_head_index;

    auto allocate_streamline_buffers = [&]() {
        size_t pos_bytes = size_t(rt.streamlineCount) * size_t(rt.streamlineHistory)
                         * sizeof(float) * 4;
        streamline_positions = gv::Buffer::create(
            ctx, pos_bytes,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            gv::MemoryUsage::DeviceLocal, "streamline_positions");
        streamline_head_index = gv::Buffer::create(
            ctx, sizeof(uint32_t),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            gv::MemoryUsage::DeviceLocal, "streamline_head_index");
    };
    allocate_streamline_buffers();

    // ------------------------------------------------------------------------
    // Samplers.
    // ------------------------------------------------------------------------
    VkSampler sampler_linear_clamp = VK_NULL_HANDLE;
    VkSampler sampler_point_clamp  = VK_NULL_HANDLE;
    VkSampler sampler_lut          = VK_NULL_HANDLE;
    {
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter    = VK_FILTER_LINEAR;
        si.minFilter    = VK_FILTER_LINEAR;
        si.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.maxLod       = 0.0f;
        vkCreateSampler(ctx.device(), &si, nullptr, &sampler_linear_clamp);
        vkCreateSampler(ctx.device(), &si, nullptr, &sampler_lut);
        si.magFilter    = VK_FILTER_NEAREST;
        si.minFilter    = VK_FILTER_NEAREST;
        vkCreateSampler(ctx.device(), &si, nullptr, &sampler_point_clamp);
    }

    // ------------------------------------------------------------------------
    // 2D LUTs (colormap + jitter).
    // ------------------------------------------------------------------------
    gv::Image colormap_lut = make_2d_rgba8(ctx, 256, 4,   "colormap_lut");
    gv::Image jitter_lut   = make_2d_r8   (ctx, 256, 256, "jitter_lut");
    {
        auto cmap = build_colormap_lut();
        colormap_lut.upload(cmap.data(), cmap.size());
        auto jit = build_jitter_lut();
        jitter_lut.upload(jit.data(), jit.size());
    }

    // ------------------------------------------------------------------------
    // Per-slot uniform buffers.
    // ------------------------------------------------------------------------
    auto make_uniform = [&](size_t bytes, const char* dbg) {
        return gv::Buffer::create(ctx, bytes,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            gv::MemoryUsage::HostVisibleSequential, dbg);
    };
    std::array<gv::Buffer, kSlots> ub_init;
    std::array<gv::Buffer, kSlots> ub_collide;
    std::array<gv::Buffer, kSlots> ub_stream;
    std::array<gv::Buffer, kSlots> ub_boundaries;
    std::array<gv::Buffer, kSlots> ub_moments;
    std::array<gv::Buffer, kSlots> ub_streamline_advect;
    std::array<gv::Buffer, kSlots> ub_raymarch;
    std::array<gv::Buffer, kSlots> ub_streamline_render;
    for (uint32_t s = 0; s < kSlots; ++s) {
        ub_init[s]               = make_uniform(sizeof(InitUniforms),             "ub_init");
        ub_collide[s]            = make_uniform(sizeof(CollideUniforms),          "ub_collide");
        ub_stream[s]             = make_uniform(sizeof(StreamUniforms),           "ub_stream");
        ub_boundaries[s]         = make_uniform(sizeof(BoundariesUniforms),       "ub_boundaries");
        ub_moments[s]            = make_uniform(sizeof(MomentsUniforms),          "ub_moments");
        ub_streamline_advect[s]  = make_uniform(sizeof(StreamlineAdvectUniforms), "ub_streamline_advect");
        ub_raymarch[s]           = make_uniform(sizeof(RaymarchUniforms),         "ub_raymarch");
        ub_streamline_render[s]  = make_uniform(sizeof(StreamlineRenderUniforms), "ub_streamline_render");
    }

    // ------------------------------------------------------------------------
    // Pipelines.
    // ------------------------------------------------------------------------
    using BT = VkDescriptorType;
    constexpr BT CIS = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    constexpr BT SI  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    constexpr BT SB  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    constexpr BT UB  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    constexpr VkShaderStageFlags CS = VK_SHADER_STAGE_COMPUTE_BIT;

    const std::string SD = GPU_SIMS_LBM_SHADER_DIR;

    gv::ComputePipelineDesc d_init{};
    d_init.shader_path = SD + "/init_equilibrium.comp.glsl";
    d_init.bindings = {
        {0, SI,  NUM_NONREST, CS},
        {1, SI,  1,           CS},
        {2, SI,  1,           CS},
        {3, SI,  1,           CS},
        {4, UB,  1,           CS},
    };
    d_init.push_constant_size = sizeof(glm::vec4);
    auto pipe_init = gv::ComputePipeline::create(ctx, compiler, d_init);

    gv::ComputePipelineDesc d_collide{};
    d_collide.shader_path = SD + "/collide.comp.glsl";
    d_collide.bindings = {
        {0, SI,  NUM_NONREST, CS},
        {1, SI,  1,           CS},
        {2, CIS, 1,           CS},
        {3, CIS, 1,           CS},
        {4, UB,  1,           CS},
    };
    d_collide.required_subgroup_size = 32;
    d_collide.require_full_subgroups = true;
    auto pipe_collide = gv::ComputePipeline::create(ctx, compiler, d_collide);

    gv::ComputePipelineDesc d_stream{};
    d_stream.shader_path = SD + "/stream.comp.glsl";
    d_stream.bindings = {
        {0, CIS, NUM_NONREST, CS},
        {1, CIS, 1,           CS},
        {2, SI,  NUM_NONREST, CS},
        {3, SI,  1,           CS},
        {4, UB,  1,           CS},
    };
    d_stream.required_subgroup_size = 32;
    d_stream.require_full_subgroups = true;
    auto pipe_stream = gv::ComputePipeline::create(ctx, compiler, d_stream);

    gv::ComputePipelineDesc d_boundaries{};
    d_boundaries.shader_path = SD + "/apply_boundaries.comp.glsl";
    d_boundaries.bindings = {
        {0, SI,  NUM_NONREST, CS},
        {1, SI,  1,           CS},
        {2, CIS, 1,           CS},
        {3, UB,  1,           CS},
    };
    d_boundaries.required_subgroup_size = 32;
    d_boundaries.require_full_subgroups = true;
    auto pipe_boundaries = gv::ComputePipeline::create(ctx, compiler, d_boundaries);

    gv::ComputePipelineDesc d_moments{};
    d_moments.shader_path = SD + "/compute_moments.comp.glsl";
    d_moments.bindings = {
        {0, CIS, NUM_NONREST, CS},
        {1, CIS, 1,           CS},
        {2, SI,  1,           CS},
        {3, SI,  1,           CS},
        {4, UB,  1,           CS},
    };
    d_moments.required_subgroup_size = 32;
    d_moments.require_full_subgroups = true;
    auto pipe_moments = gv::ComputePipeline::create(ctx, compiler, d_moments);

    gv::ComputePipelineDesc d_streamline_advect{};
    d_streamline_advect.shader_path = SD + "/streamline_advect.comp.glsl";
    d_streamline_advect.bindings = {
        {0, CIS, 1, CS},
        {1, SB,  1, CS},
        {2, UB,  1, CS},
    };
    auto pipe_streamline_advect = gv::ComputePipeline::create(ctx, compiler, d_streamline_advect);

    gv::GraphicsPipelineDesc gd_raymarch{};
    gd_raymarch.vertex_shader_path   = SD + "/fullscreen.vert.glsl";
    gd_raymarch.fragment_shader_path = SD + "/velmag.frag.glsl";
    gd_raymarch.color_formats        = {window.colorFormat()};
    gd_raymarch.depth_test           = false;
    gd_raymarch.cull_mode            = VK_CULL_MODE_NONE;
    gd_raymarch.blend_enable         = true;  // alpha-over blend with background
    gd_raymarch.bindings = {
        {0, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {1, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {2, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {3, UB,  1, VK_SHADER_STAGE_FRAGMENT_BIT},
    };
    auto pipe_raymarch = gv::GraphicsPipeline::create(ctx, compiler, gd_raymarch);

    gv::GraphicsPipelineDesc gd_streamline{};
    gd_streamline.vertex_shader_path   = SD + "/streamline.vert.glsl";
    gd_streamline.fragment_shader_path = SD + "/streamline.frag.glsl";
    gd_streamline.color_formats        = {window.colorFormat()};
    gd_streamline.depth_test           = false;
    gd_streamline.cull_mode            = VK_CULL_MODE_NONE;
    gd_streamline.topology             = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    gd_streamline.blend_enable         = true;
    gd_streamline.push_constant_size   = sizeof(uint32_t);
    gd_streamline.bindings = {
        {0, SB, 1, VK_SHADER_STAGE_VERTEX_BIT},
        {1, UB, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
    };
    auto pipe_streamline = gv::GraphicsPipeline::create(ctx, compiler, gd_streamline);

    // ------------------------------------------------------------------------
    // Descriptor sets.
    // ------------------------------------------------------------------------
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_init{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_collide{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_stream{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_boundaries{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_moments{};
    std::array<VkDescriptorSet, kSlots>                        ds_streamline_advect{};
    std::array<VkDescriptorSet, kSlots>                        ds_raymarch{};
    std::array<VkDescriptorSet, kSlots>                        ds_streamline{};

    auto allocate_descriptors = [&]() {
        for (uint32_t p = 0; p < kParities; ++p) {
            for (uint32_t s = 0; s < kSlots; ++s) {
                ds_init      [p][s] = pipe_init      .allocateDescriptorSet();
                ds_collide   [p][s] = pipe_collide   .allocateDescriptorSet();
                ds_stream    [p][s] = pipe_stream    .allocateDescriptorSet();
                ds_boundaries[p][s] = pipe_boundaries.allocateDescriptorSet();
                ds_moments   [p][s] = pipe_moments   .allocateDescriptorSet();
            }
        }
        for (uint32_t s = 0; s < kSlots; ++s) {
            ds_streamline_advect[s] = pipe_streamline_advect.allocateDescriptorSet();
            ds_raymarch[s]          = pipe_raymarch.allocateDescriptorSet();
            ds_streamline[s]        = pipe_streamline.allocateDescriptorSet();
        }
    };
    allocate_descriptors();

    auto wire_all_descriptors = [&]() {
        VkDevice dev = ctx.device();
        std::array<VkImageView, NUM_NONREST> ping_views{};
        std::array<VkImageView, NUM_NONREST> pong_views{};
        for (uint32_t i = 0; i < NUM_NONREST; ++i) {
            ping_views[i] = f_ping_nonrest[i].view();
            pong_views[i] = f_pong_nonrest[i].view();
        }
        for (uint32_t p = 0; p < kParities; ++p) {
            const auto& in_views   = (p == 0) ? ping_views : pong_views;
            const auto& out_views  = (p == 0) ? pong_views : ping_views;
            VkImageView in_rest    = (p == 0) ? f_rest_ping.view() : f_rest_pong.view();
            VkImageView out_rest   = (p == 0) ? f_rest_pong.view() : f_rest_ping.view();
            for (uint32_t s = 0; s < kSlots; ++s) {
                writeInitDescriptor(dev, ds_init[p][s],
                    in_views, in_rest, rho_image.view(), velocity_image.view(),
                    ub_init[s].handle());
                writeCollideDescriptor(dev, ds_collide[p][s],
                    in_views, in_rest, rho_image.view(), velocity_image.view(),
                    sampler_linear_clamp, ub_collide[s].handle());
                writeStreamDescriptor(dev, ds_stream[p][s],
                    in_views, in_rest, out_views, out_rest,
                    sampler_point_clamp, ub_stream[s].handle());
                writeBoundariesDescriptor(dev, ds_boundaries[p][s],
                    out_views, out_rest, obstacle_mask.view(), sampler_point_clamp,
                    ub_boundaries[s].handle());
                writeMomentsDescriptor(dev, ds_moments[p][s],
                    out_views, out_rest, rho_image.view(), velocity_image.view(),
                    sampler_point_clamp, ub_moments[s].handle());
            }
        }
        for (uint32_t s = 0; s < kSlots; ++s) {
            writeStreamlineAdvectDescriptor(dev, ds_streamline_advect[s],
                velocity_image.view(), sampler_linear_clamp,
                streamline_positions.handle(), ub_streamline_advect[s].handle());
            writeRaymarchDescriptor(dev, ds_raymarch[s],
                velocity_image.view(), colormap_lut.view(), jitter_lut.view(),
                sampler_linear_clamp, sampler_lut, ub_raymarch[s].handle());
            writeStreamlineRenderDescriptor(dev, ds_streamline[s],
                streamline_positions.handle(), ub_streamline_render[s].handle());
        }
    };
    wire_all_descriptors();

    // ------------------------------------------------------------------------
    // Streamline seed initialization.
    // Initial age is randomized per streamline over [0, STREAMLINE_RESEED_AGE)
    // so reseed events are uniformly distributed across the cycle. Without the
    // randomization, all ~10k streamlines reseed in the same frame every
    // STREAMLINE_RESEED_AGE frames (a visible burst). The position stays at
    // (0,0,0) — the GPU's first advect pass will reseed each streamline to a
    // valid inlet-slab position as its age crosses the threshold.
    // ------------------------------------------------------------------------
    auto seed_streamlines = [&]() {
        size_t total = size_t(rt.streamlineCount) * size_t(rt.streamlineHistory);
        std::vector<glm::vec4> data(total);
        static std::mt19937 rng(12345u);
        std::uniform_real_distribution<float> ud(0.0f, 1.0f);
        const glm::vec3 pos(0.0f);
        for (uint32_t i = 0; i < rt.streamlineCount; ++i) {
            float initial_age = ud(rng) * float(STREAMLINE_RESEED_AGE);
            for (uint32_t h = 0; h < rt.streamlineHistory; ++h) {
                data[size_t(i) * rt.streamlineHistory + h] = glm::vec4(pos, initial_age);
            }
        }
        streamline_positions.stage(ctx, data.data(), data.size() * sizeof(glm::vec4));
        uint32_t zero = 0;
        streamline_head_index.stage(ctx, &zero, sizeof(uint32_t));
        rt.streamlineFrameIndex = 0;
    };
    seed_streamlines();

    // ------------------------------------------------------------------------
    // Update u_inf vector from magnitude + AoA (helper used by panel).
    // ------------------------------------------------------------------------
    auto update_u_inf_vector = [&]() {
        float a = glm::radians(rt.angleOfAttackDeg);
        rt.uInf = glm::vec3(rt.uInfMagnitude * std::cos(a),
                            rt.uInfMagnitude * std::sin(a),
                            0.0f);
    };

    // ------------------------------------------------------------------------
    // Apply preset: voxelize airfoil, run init kernel, reseed streamlines.
    // ------------------------------------------------------------------------
    auto apply_preset = [&](int preset_idx) {
        const auto& P = PRESETS[size_t(preset_idx)];
        rt.presetIndex      = preset_idx;
        rt.tau              = P.tau;
        rt.uInfMagnitude    = P.u_inf;
        rt.angleOfAttackDeg = P.angle_of_attack_deg;
        // Auto-calibrate velmag colormap range to the preset's free-stream
        // magnitude. Default Runtime values (0.0, 0.1) span too wide for
        // u_inf in [0.04, 0.06], squashing wake-structure contrast.
        rt.velmagMin = 0.0f;
        rt.velmagMax = 1.5f * rt.uInfMagnitude;
        update_u_inf_vector();

        // Voxelize airfoil.
        auto airfoil = naca::parse(P.naca_designation);
        std::vector<uint8_t> mask_bytes(size_t(rt.Nx) * rt.Ny * rt.Nz);
        float chord_pixels = AIRFOIL_CHORD_LENGTH_FRAC * float(rt.Nx);
        glm::vec2 leading_edge_xy(
            AIRFOIL_CHORD_CENTER_X_FRAC * float(rt.Nx) - chord_pixels * 0.5f,
            float(rt.Ny) * 0.5f);
        naca::voxelize_into(mask_bytes, rt.Nx, rt.Ny, rt.Nz, airfoil,
                            chord_pixels, glm::radians(P.angle_of_attack_deg),
                            leading_edge_xy);
        obstacle_mask.upload(mask_bytes.data(), mask_bytes.size());

        // Run init kernel via a one-shot command buffer.
        InitUniforms iu{};
        iu.dims      = glm::ivec4(int(rt.Nx), int(rt.Ny), int(rt.Nz), 0);
        iu.tau_inv   = 1.0f / rt.tau;
        iu.omtau_inv = 1.0f - iu.tau_inv;
        ub_init[0].uploadDirect(&iu, sizeof(iu));

        glm::vec4 push_uinf(rt.uInf, 0.0f);
        ctx.runOneShot([&](VkCommandBuffer cmd) {
            uint32_t wgX = (rt.Nx + WG_DIM_X - 1) / WG_DIM_X;
            uint32_t wgY = (rt.Ny + WG_DIM_Y - 1) / WG_DIM_Y;
            uint32_t wgZ = (rt.Nz + WG_DIM_Z - 1) / WG_DIM_Z;
            pipe_init.dispatch(cmd, ds_init[0][0], wgX, wgY, wgZ,
                               &push_uinf, sizeof(push_uinf));
        });

        seed_streamlines();
        rt.iteration = 0;
    };
    apply_preset(DEFAULT_PRESET_INDEX);

    // ------------------------------------------------------------------------
    // Camera.
    // ------------------------------------------------------------------------
    Camera camera;
    camera.setMode(Camera::Mode::FreeFly);
    camera.setFovDeg(FOV_DEG_DEFAULT);
    camera.setNearFar(NEAR_PLANE, FAR_PLANE);
    camera.setAspect(window.aspect());
    camera.setMoveSpeed(float(std::max(rt.Nx, std::max(rt.Ny, rt.Nz))) * 0.5f);

    auto reset_camera_for_tier = [&]() {
        glm::vec3 pos(float(rt.Nx) * 1.5f, float(rt.Ny) * 0.8f, float(rt.Nz) * 1.5f);
        glm::vec3 target(float(rt.Nx) * 0.4f, float(rt.Ny) * 0.5f, float(rt.Nz) * 0.5f);
        camera.setPosition(pos);
        // FreeFly faces -Z by default; compute yaw/pitch toward target.
        glm::vec3 fwd = glm::normalize(target - pos);
        float yaw_deg   = glm::degrees(std::atan2(fwd.z, fwd.x));
        float pitch_deg = glm::degrees(std::asin(std::clamp(fwd.y, -1.0f, 1.0f)));
        camera.setOrientation(yaw_deg, pitch_deg);
        camera.setMoveSpeed(float(std::max(rt.Nx, std::max(rt.Ny, rt.Nz))) * 0.5f);
    };
    reset_camera_for_tier();

    // ------------------------------------------------------------------------
    // ImGui.
    // ------------------------------------------------------------------------
    {
        gpusims::ui::ImGuiInit ic{};
        ic.glfw_window     = window.glfwWindow();
        ic.instance        = ctx.instance();
        ic.physical_device = ctx.physicalDevice();
        ic.device          = ctx.device();
        ic.queue_family    = ctx.graphicsQueueFamily();
        ic.queue           = ctx.graphicsQueue();
        ic.descriptor_pool = VK_NULL_HANDLE;
        ic.color_format    = window.colorFormat();
        ic.min_image_count = 2;
        ic.image_count     = window.imageCount();
        if (!gpusims::ui::initImGui(ic)) {
            logError("lattice_boltzmann: ui::initImGui failed");
            return 1;
        }
    }
    glfwSetScrollCallback(window.glfwWindow(), scrollCallback);

    GpuProfiler profiler(ctx);

    fs::create_directories("captures");
    fs::create_directories("vdb_export");
    StateWriter capture_writer(fs::current_path() / "captures");

    // ------------------------------------------------------------------------
    // Capture meta JSON helper.
    // ------------------------------------------------------------------------
    auto runtime_meta_json = [&]() -> json {
        json j;
        j["schemaVersion"]   = 1;
        j["tierIndex"]       = rt.tierIndex;
        j["presetIndex"]     = rt.presetIndex;
        j["Nx"]              = rt.Nx;
        j["Ny"]              = rt.Ny;
        j["Nz"]              = rt.Nz;
        j["tau"]             = rt.tau;
        j["substeps"]        = rt.substeps;
        j["uInfMagnitude"]   = rt.uInfMagnitude;
        j["angleOfAttackDeg"]= rt.angleOfAttackDeg;
        j["iteration"]       = rt.iteration;
        json cam_j; camera.toJson(cam_j);
        j["camera"]          = cam_j;
        return j;
    };

    // ------------------------------------------------------------------------
    // F5 capture-save.
    // ------------------------------------------------------------------------
    auto capture_save = [&]() {
        renderer.waitIdle();
        size_t N = size_t(rt.Nx) * rt.Ny * rt.Nz;
        std::vector<float>    rho_bytes(N);
        std::vector<uint16_t> vel_halfs(N * 4);
        std::vector<uint8_t>  mask_bytes(N);
        rho_image     .readback(rho_bytes.data(),  rho_bytes.size()  * sizeof(float));
        velocity_image.readback(vel_halfs.data(),  vel_halfs.size()  * sizeof(uint16_t));
        obstacle_mask .readback(mask_bytes.data(), mask_bytes.size());

        uint32_t frame_idx = uint32_t(rt.iteration / uint64_t(std::max(rt.substeps, 1)));
        capture_writer.beginFrame(frame_idx);
        capture_writer.setMeta("latticeBoltzmann", runtime_meta_json());
        capture_writer.saveBuffer("density", rho_bytes.data(), rho_bytes.size() * sizeof(float),
            {{"count", uint64_t(N)}, {"stride", 4}, {"format", "r32f"},
             {"shape", {rt.Nx, rt.Ny, rt.Nz}}});
        capture_writer.saveBuffer("velocity", vel_halfs.data(), vel_halfs.size() * sizeof(uint16_t),
            {{"count", uint64_t(N)}, {"stride", 8}, {"format", "rgba16f"},
             {"shape", {rt.Nx, rt.Ny, rt.Nz}}});
        capture_writer.saveBuffer("obstacle_mask", mask_bytes.data(), mask_bytes.size(),
            {{"count", uint64_t(N)}, {"stride", 1}, {"format", "r8uint"},
             {"shape", {rt.Nx, rt.Ny, rt.Nz}},
             {"frame_invariant", true}});
        capture_writer.endFrame();
        gpusims::ui::pushToast(("Saved capture #" + std::to_string(frame_idx)).c_str(), true);
        logInfo("F5: saved capture {}", frame_idx);
    };

    // ------------------------------------------------------------------------
    // F9 capture-load.
    // ------------------------------------------------------------------------
    auto capture_load = [&]() {
        auto latest = StateReader::findLatest(fs::current_path() / "captures");
        if (!latest) {
            gpusims::ui::pushToast("F9: no captures.", false);
            logWarn("F9: no captures found.");
            return;
        }
        auto opt = StateReader::open(*latest);
        if (!opt) {
            gpusims::ui::pushToast("F9: failed to open capture.", false);
            logError("F9: failed to open capture at {}", latest->string());
            return;
        }
        auto& reader = *opt;
        json lbm_meta = reader.meta("latticeBoltzmann");
        if (lbm_meta.is_null()) {
            gpusims::ui::pushToast("F9: capture missing latticeBoltzmann key.", false);
            logWarn("F9: capture lacks latticeBoltzmann meta; skipping.");
            return;
        }

        int captured_tier   = lbm_meta.value("tierIndex",   DEFAULT_TIER_INDEX);
        int captured_preset = lbm_meta.value("presetIndex", DEFAULT_PRESET_INDEX);

        if (captured_tier != rt.tierIndex) {
            rt.pendingTierIndex = captured_tier;
            rt.pendingLoadFromF9 = true;
            return;
        }
        if (captured_preset != rt.presetIndex) {
            apply_preset(captured_preset);
        }

        rt.tau              = lbm_meta.value("tau", rt.tau);
        rt.substeps         = lbm_meta.value("substeps", rt.substeps);
        rt.uInfMagnitude    = lbm_meta.value("uInfMagnitude", rt.uInfMagnitude);
        rt.angleOfAttackDeg = lbm_meta.value("angleOfAttackDeg", rt.angleOfAttackDeg);
        rt.iteration        = lbm_meta.value("iteration", uint64_t(0));
        update_u_inf_vector();

        auto rho_blob  = reader.buffer("density");
        auto vel_blob  = reader.buffer("velocity");
        if (!rho_blob.empty()) rho_image.upload(rho_blob.data(), rho_blob.size());
        if (!vel_blob.empty()) velocity_image.upload(vel_blob.data(), vel_blob.size());
        json mask_meta = reader.bufferMeta("obstacle_mask");
        if (!mask_meta.value("frame_invariant", false)) {
            auto mask_blob = reader.buffer("obstacle_mask");
            if (!mask_blob.empty()) obstacle_mask.upload(mask_blob.data(), mask_blob.size());
        }

        // Re-derive f from moments by running init kernel again.
        InitUniforms iu{};
        iu.dims      = glm::ivec4(int(rt.Nx), int(rt.Ny), int(rt.Nz), 0);
        iu.tau_inv   = 1.0f / rt.tau;
        iu.omtau_inv = 1.0f - iu.tau_inv;
        ub_init[0].uploadDirect(&iu, sizeof(iu));
        glm::vec4 push_uinf(rt.uInf, 0.0f);
        ctx.runOneShot([&](VkCommandBuffer cmd) {
            uint32_t wgX = (rt.Nx + WG_DIM_X - 1) / WG_DIM_X;
            uint32_t wgY = (rt.Ny + WG_DIM_Y - 1) / WG_DIM_Y;
            uint32_t wgZ = (rt.Nz + WG_DIM_Z - 1) / WG_DIM_Z;
            pipe_init.dispatch(cmd, ds_init[0][0], wgX, wgY, wgZ,
                               &push_uinf, sizeof(push_uinf));
        });

        if (lbm_meta.contains("camera")) {
            camera.fromJson(lbm_meta["camera"]);
        }

        gpusims::ui::pushToast(("Loaded capture from " + latest->filename().string()).c_str(), true);
        logInfo("F9: loaded capture from {}", latest->string());
    };

    // ------------------------------------------------------------------------
    // Tier-deferred reallocation.
    // ------------------------------------------------------------------------
    auto reallocate_tier = [&]() {
        renderer.waitIdle();
        rt.tierIndex  = rt.pendingTierIndex;
        rt.Nx         = TIERS[rt.tierIndex].Nx;
        rt.Ny         = TIERS[rt.tierIndex].Ny;
        rt.Nz         = TIERS[rt.tierIndex].Nz;
        rt.totalCells = uint64_t(rt.Nx) * rt.Ny * rt.Nz;
        // Destroy + recreate.
        for (auto& f : f_ping_nonrest) f = gv::Image{};
        for (auto& f : f_pong_nonrest) f = gv::Image{};
        f_rest_ping    = gv::Image{};
        f_rest_pong    = gv::Image{};
        rho_image      = gv::Image{};
        velocity_image = gv::Image{};
        obstacle_mask  = gv::Image{};
        allocate_tier_images();
        allocate_streamline_buffers();
        wire_all_descriptors();
        seed_streamlines();
        apply_preset(rt.presetIndex);
        reset_camera_for_tier();
        logInfo("Tier change: {}x{}x{}", rt.Nx, rt.Ny, rt.Nz);
    };

    auto reallocate_streamlines = [&]() {
        renderer.waitIdle();
        allocate_streamline_buffers();
        wire_all_descriptors();
        seed_streamlines();
    };

    // ------------------------------------------------------------------------
    // Hot-reload: framework + per-pipeline flags + apply lambda.
    // ------------------------------------------------------------------------
    HotReloader reloader;
    bool reload_init = false, reload_collide = false, reload_stream = false;
    bool reload_boundaries = false, reload_moments = false, reload_advect = false;
    bool reload_velmag = false, reload_streamline_gfx = false;

    auto W = [&](const std::string& rel, bool* flag) {
        reloader.watch(SD + "/" + rel, [flag](const fs::path&){ *flag = true; });
    };
    W("init_equilibrium.comp.glsl",  &reload_init);
    W("collide.comp.glsl",           &reload_collide);
    W("stream.comp.glsl",            &reload_stream);
    W("apply_boundaries.comp.glsl",  &reload_boundaries);
    W("compute_moments.comp.glsl",   &reload_moments);
    W("streamline_advect.comp.glsl", &reload_advect);
    W("lattice_constants.glsl",      &reload_collide);  // any include change re-flags collide
    W("velmag.frag.glsl",            &reload_velmag);
    W("fullscreen.vert.glsl",        &reload_velmag);
    W("streamline.vert.glsl",        &reload_streamline_gfx);
    W("streamline.frag.glsl",        &reload_streamline_gfx);

    bool prev_f5 = false, prev_f9 = false;
    auto prev_time = std::chrono::steady_clock::now();

    // ------------------------------------------------------------------------
    // MAIN LOOP
    // ------------------------------------------------------------------------
    while (!window.shouldClose()) {
        window.pollEvents();

        auto now = std::chrono::steady_clock::now();
        float frame_dt = std::chrono::duration<float>(now - prev_time).count();
        prev_time = now;
        if (frame_dt > 0.1f) frame_dt = 0.1f;
        rt.lastFrameMs = frame_dt * 1000.0f;

        reloader.poll();

        // F5/F9 rising-edge.
        bool now_f5 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F5) == GLFW_PRESS;
        bool now_f9 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F9) == GLFW_PRESS;
        if (now_f5 && !prev_f5) capture_save();
        if (now_f9 && !prev_f9) capture_load();
        prev_f5 = now_f5; prev_f9 = now_f9;

        // Tier change (deferred).
        if (rt.pendingTierIndex != rt.tierIndex) {
            reallocate_tier();
        }
        if (rt.pendingLoadFromF9) {
            rt.pendingLoadFromF9 = false;
            capture_load();
        }
        if (rt.streamlineRealloc) {
            rt.streamlineRealloc = false;
            reallocate_streamlines();
        }

        camera.setAspect(window.aspect());
        camera.update(frame_dt, g_input.snapshot(window.glfwWindow()));

        gv::Frame* frame = renderer.beginFrame();
        if (!frame) continue;
        const uint32_t slot = frame->in_flight_index;
        VkCommandBuffer cmd = frame->command_buffer;

        // Apply hot reloads.
        auto apply_reload = [&](bool* flag, auto& pipeline, const char* dbg) {
            if (!*flag) return;
            std::string err;
            if (pipeline.reload(ctx, compiler, *frame, &err)) {
                reloader.reportSuccess(dbg);
                gpusims::ui::pushToast((std::string("Reloaded: ") + dbg).c_str(), true);
                logInfo("hot-reload OK: {}", dbg);
            } else {
                reloader.reportFailure(dbg, err);
                gpusims::ui::pushToast((std::string("Reload failed: ") + dbg).c_str(), false);
                logError("hot-reload FAIL: {} - {}", dbg, err);
            }
            *flag = false;
        };
        apply_reload(&reload_init,           pipe_init,              "init_equilibrium");
        apply_reload(&reload_collide,        pipe_collide,           "collide");
        apply_reload(&reload_stream,         pipe_stream,            "stream");
        apply_reload(&reload_boundaries,     pipe_boundaries,        "apply_boundaries");
        apply_reload(&reload_moments,        pipe_moments,           "compute_moments");
        apply_reload(&reload_advect,         pipe_streamline_advect, "streamline_advect");
        apply_reload(&reload_velmag,         pipe_raymarch,          "raymarch");
        apply_reload(&reload_streamline_gfx, pipe_streamline,        "streamline_render");

        profiler.beginFrame(cmd, slot);

        // --------------------------------------------------------------------
        // Per-frame uniform uploads (substep uniforms — same across substeps).
        // --------------------------------------------------------------------
        CollideUniforms cu{};
        cu.dims      = glm::ivec4(int(rt.Nx), int(rt.Ny), int(rt.Nz), 0);
        cu.tau_inv   = 1.0f / rt.tau;
        cu.omtau_inv = 1.0f - cu.tau_inv;
        ub_collide[slot].uploadDirect(&cu, sizeof(cu));

        StreamUniforms su{};
        su.dims = cu.dims;
        ub_stream[slot].uploadDirect(&su, sizeof(su));

        BoundariesUniforms bu{};
        bu.dims       = cu.dims;
        bu.u_inf      = glm::vec4(rt.uInf.x, rt.uInf.y, rt.uInf.z, RHO_0);
        bu.inlet_axis = glm::ivec4(0);
        ub_boundaries[slot].uploadDirect(&bu, sizeof(bu));

        MomentsUniforms mu{};
        mu.dims = cu.dims;
        ub_moments[slot].uploadDirect(&mu, sizeof(mu));

        // Frame-boundary barrier: previous frame's fragment read -> compute write.
        gv::memoryBarrier(cmd,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  VK_ACCESS_SHADER_WRITE_BIT);

        // --------------------------------------------------------------------
        // Substep dispatch chain.
        // --------------------------------------------------------------------
        const uint32_t wgX = (rt.Nx + WG_DIM_X - 1) / WG_DIM_X;
        const uint32_t wgY = (rt.Ny + WG_DIM_Y - 1) / WG_DIM_Y;
        const uint32_t wgZ = (rt.Nz + WG_DIM_Z - 1) / WG_DIM_Z;
        for (int sub = 0; sub < rt.substeps; ++sub) {
            uint32_t p = uint32_t(rt.iteration & 1u);

            {
                auto scope = profiler.scope(cmd, "collide");
                pipe_collide.dispatch(cmd, ds_collide[p][slot], wgX, wgY, wgZ);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

            {
                auto scope = profiler.scope(cmd, "stream");
                pipe_stream.dispatch(cmd, ds_stream[p][slot], wgX, wgY, wgZ);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

            {
                auto scope = profiler.scope(cmd, "boundaries");
                pipe_boundaries.dispatch(cmd, ds_boundaries[p][slot], wgX, wgY, wgZ);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

            {
                auto scope = profiler.scope(cmd, "moments");
                pipe_moments.dispatch(cmd, ds_moments[p][slot], wgX, wgY, wgZ);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_SHADER_READ_BIT);

            rt.iteration++;
        }

        // --------------------------------------------------------------------
        // Streamline advection (compute pass before render).
        // --------------------------------------------------------------------
        if (rt.renderStreamlines) {
            StreamlineAdvectUniforms ssu{};
            ssu.dims                 = glm::ivec4(int(rt.Nx), int(rt.Ny), int(rt.Nz), 0);
            ssu.domain_min           = glm::vec4(0.0f);
            ssu.domain_max           = glm::vec4(float(rt.Nx), float(rt.Ny), float(rt.Nz), 0.0f);
            ssu.streamline_count     = rt.streamlineCount;
            ssu.history              = rt.streamlineHistory;
            ssu.head_index           = rt.streamlineFrameIndex;
            ssu.frame_count          = uint32_t(rt.iteration / uint64_t(std::max(rt.substeps, 1)));
            ssu.dt_render            = std::clamp(rt.lastFrameMs * 0.001f, 1.0e-3f, 0.05f);
            ssu.reseed_age_threshold = uint32_t(STREAMLINE_RESEED_AGE);
            ub_streamline_advect[slot].uploadDirect(&ssu, sizeof(ssu));

            uint32_t wg = (rt.streamlineCount + WG_DIM_STREAMLINE - 1) / WG_DIM_STREAMLINE;
            {
                auto scope = profiler.scope(cmd, "streamline_advect");
                pipe_streamline_advect.dispatch(cmd, ds_streamline_advect[slot], wg, 1, 1);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,  VK_ACCESS_SHADER_READ_BIT);

            rt.streamlineFrameIndex = (rt.streamlineFrameIndex + 1u) % rt.streamlineHistory;
        }

        // --------------------------------------------------------------------
        // Build ImGui frame BEFORE beginRendering so panel side-effects
        // (preset apply etc.) are recorded outside the render pass.
        // --------------------------------------------------------------------
        gpusims::ui::newImGuiFrame();
        ImGui::Begin("Lattice Boltzmann (Phase 12)");
        {
            const char* preset_labels[NUM_PRESETS];
            for (int i = 0; i < NUM_PRESETS; ++i) preset_labels[i] = PRESETS[size_t(i)].label;
            int preset_idx = rt.presetIndex;
            if (ImGui::Combo("Preset", &preset_idx, preset_labels, NUM_PRESETS)) {
                apply_preset(preset_idx);
            }
            const char* tier_labels[NUM_TIERS];
            for (int i = 0; i < NUM_TIERS; ++i) tier_labels[i] = TIERS[size_t(i)].label;
            int tier_idx = rt.pendingTierIndex;
            if (ImGui::Combo("Tier", &tier_idx, tier_labels, NUM_TIERS)) {
                rt.pendingTierIndex = tier_idx;
            }
            ImGui::TextDisabled("%s", TIERS[size_t(rt.tierIndex)].note);
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Solver", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("tau", &rt.tau, 0.51f, 2.0f, "%.3f");
            ImGui::SliderInt("substeps", &rt.substeps, SUBSTEPS_MIN, SUBSTEPS_MAX);
            float nu = (rt.tau - 0.5f) / 3.0f;
            float chord = AIRFOIL_CHORD_LENGTH_FRAC * float(rt.Nx);
            float Re = rt.uInfMagnitude * chord / std::max(nu, 1.0e-9f);
            rt.currentRe = Re;
            ImGui::TextDisabled("nu = %.4f, Re = %.1f", double(nu), double(Re));
        }
        if (ImGui::CollapsingHeader("Flow")) {
            if (ImGui::SliderFloat("|u_inf|", &rt.uInfMagnitude, 0.005f, U_INF_MAX, "%.4f")) {
                update_u_inf_vector();
            }
            if (ImGui::SliderFloat("angle of attack (deg)", &rt.angleOfAttackDeg, -15.0f, 15.0f)) {
                update_u_inf_vector();
            }
            ImGui::TextDisabled("u_inf = (%.3f, %.3f, %.3f)",
                double(rt.uInf.x), double(rt.uInf.y), double(rt.uInf.z));
        }
        if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Velocity-magnitude raymarch", &rt.renderVelmag);
            ImGui::Checkbox("Streamlines",                 &rt.renderStreamlines);
            ImGui::SliderInt("Raymarch steps", &rt.raymarchSteps, 32, 256);
            ImGui::SliderFloat("velmag min", &rt.velmagMin, 0.0f, 0.1f, "%.4f");
            ImGui::SliderFloat("velmag max", &rt.velmagMax, 0.0f, 0.2f, "%.4f");
            ImGui::SliderFloat("Exposure",   &rt.exposure,  0.5f, 3.0f, "%.2f");
        }
        if (ImGui::CollapsingHeader("Streamlines")) {
            int count_i   = int(rt.streamlineCount);
            int history_i = int(rt.streamlineHistory);
            bool need_reseed = false;
            if (ImGui::SliderInt("count",   &count_i,   1000, 50000)) {
                rt.streamlineCount = uint32_t(count_i);
                need_reseed = true;
            }
            if (ImGui::SliderInt("history", &history_i, 16, 128)) {
                rt.streamlineHistory = uint32_t(history_i);
                need_reseed = true;
            }
            if (need_reseed) rt.streamlineRealloc = true;
        }
        if (ImGui::CollapsingHeader("Capture")) {
            ImGui::TextDisabled("F5 = save, F9 = load latest");
            ImGui::Checkbox("Export VDB", &rt.exportVdb);
            if (rt.exportVdb) {
                ImGui::SliderInt("VDB every N frames", &rt.vdbEveryNFrames, 1, 60);
                ImGui::TextDisabled("%s", vdb::isAvailable() ? "VDB ready" : "VDB stub mode");
            }
        }
        if (ImGui::CollapsingHeader("Camera")) {
            camera.drawImGui("Camera");
        }
        if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("FPS: %.1f", double(1000.0f / std::max(rt.lastFrameMs, 0.01f)));
            ImGui::Text("Iter: %llu", static_cast<unsigned long long>(rt.iteration));
            ImGui::Text("Cells: %llu", static_cast<unsigned long long>(rt.totalCells));
            ImGui::Separator();
            profiler.drawImGui();
        }
        ImGui::End();
        gpusims::ui::drawToasts();

        // --------------------------------------------------------------------
        // Render pass: raymarch -> streamlines -> imgui.
        // --------------------------------------------------------------------
        VkClearColorValue clear{};
        clear.float32[0] = 0.02f;
        clear.float32[1] = 0.03f;
        clear.float32[2] = 0.05f;
        clear.float32[3] = 1.0f;
        renderer.beginRendering(*frame, clear);

        if (rt.renderVelmag) {
            RaymarchUniforms ru{};
            ru.invViewProj = glm::inverse(camera.viewProjection());
            ru.cameraPos   = glm::vec4(camera.position(), 0.0f);
            ru.volumeMin   = glm::vec4(0.0f);
            ru.volumeMax   = glm::vec4(float(rt.Nx), float(rt.Ny), float(rt.Nz), 0.0f);
            float max_dim  = float(std::max(rt.Nx, std::max(rt.Ny, rt.Nz)));
            ru.volumeAspect= glm::vec4(float(rt.Nx), float(rt.Ny), float(rt.Nz), max_dim);
            ru.raymarchSteps    = rt.raymarchSteps;
            ru.velmagAbsorption = rt.velmagAbsorption;
            ru.velmagMin        = rt.velmagMin;
            ru.velmagMax        = rt.velmagMax;
            ru.exposure         = rt.exposure;
            ub_raymarch[slot].uploadDirect(&ru, sizeof(ru));

            auto scope = profiler.scope(cmd, "raymarch");
            pipe_raymarch.bind(cmd, ds_raymarch[slot]);
            vkCmdDraw(cmd, 3, 1, 0, 0);
        }

        if (rt.renderStreamlines) {
            StreamlineRenderUniforms sru{};
            sru.viewProj   = camera.viewProjection();
            sru.lineColor  = glm::vec4(1.0f, 0.95f, 0.85f, 1.0f);
            sru.history    = rt.streamlineHistory;
            sru.head_index = rt.streamlineFrameIndex;
            sru.ageFalloff = 0.5f;
            ub_streamline_render[slot].uploadDirect(&sru, sizeof(sru));

            auto scope = profiler.scope(cmd, "streamline_render");
            pipe_streamline.bind(cmd, ds_streamline[slot]);
            // Draw one line-strip per streamline; push the streamline ID via
            // push constants between draws (avoids re-binding pipeline+ds).
            for (uint32_t sid = 0; sid < rt.streamlineCount; ++sid) {
                vkCmdPushConstants(cmd, pipe_streamline.pipelineLayout(),
                                   VK_SHADER_STAGE_VERTEX_BIT,
                                   0, sizeof(uint32_t), &sid);
                vkCmdDraw(cmd, rt.streamlineHistory, 1, 0, 0);
            }
        }

        gpusims::ui::renderImGui(cmd);

        renderer.endRendering(*frame);

        // --------------------------------------------------------------------
        // Optional VDB export.
        // --------------------------------------------------------------------
        if (rt.exportVdb && vdb::isAvailable()) {
            uint32_t frame_idx = uint32_t(rt.iteration / uint64_t(std::max(rt.substeps, 1)));
            if (rt.vdbEveryNFrames > 0 && (frame_idx % uint32_t(rt.vdbEveryNFrames)) == 0u) {
                renderer.waitIdle();
                size_t N = size_t(rt.Nx) * rt.Ny * rt.Nz;
                std::vector<uint16_t> vel_halfs(N * 4);
                velocity_image.readback(vel_halfs.data(), vel_halfs.size() * sizeof(uint16_t));
                std::vector<float> vec3_data(N * 3);
                for (size_t i = 0; i < N; ++i) {
                    vec3_data[i * 3 + 0] = half_to_float(vel_halfs[i * 4 + 0]);
                    vec3_data[i * 3 + 1] = half_to_float(vel_halfs[i * 4 + 1]);
                    vec3_data[i * 3 + 2] = half_to_float(vel_halfs[i * 4 + 2]);
                }
                char fname[64];
                std::snprintf(fname, sizeof(fname), "vdb_export/velocity_%06u.vdb", frame_idx);
                bool ok = vdb::writeVec3Grid(fname, vec3_data.data(),
                    glm::ivec3(int(rt.Nx), int(rt.Ny), int(rt.Nz)),
                    1.0f, glm::vec3(0.0f), "velocity");
                if (!ok) {
                    logError("VDB export failed at frame {}; disabling export", frame_idx);
                    rt.exportVdb = false;
                }
            }
        }

        profiler.endFrame(cmd);
        renderer.endFrame(*frame);
    }

    // ------------------------------------------------------------------------
    // Shutdown.
    // ------------------------------------------------------------------------
    renderer.waitIdle();
    gpusims::ui::shutdownImGui();
    vkDestroySampler(ctx.device(), sampler_lut,          nullptr);
    vkDestroySampler(ctx.device(), sampler_point_clamp,  nullptr);
    vkDestroySampler(ctx.device(), sampler_linear_clamp, nullptr);
    logInfo("lattice_boltzmann: shutdown complete");
    return 0;
}
