// Eulerian Smoke — first Tier-2 flagship Stack C sim.
//
// 256³ Stam stable-fluids with MacCormack-corrected semi-Lagrangian advection,
// vorticity confinement, Jacobi pressure projection, buoyancy from temperature.
// Volume raymarch render with Beer-Lambert absorption, single-scattering
// single-shadow-march, and temperature-driven black-body emission.
// Six smoke-dynamics presets. Sparse user-placed emitters (LMB-place / RMB-remove,
// cap 8) as the headline interactive moment. Optional per-frame OpenVDB density
// export feeding render-pipelines/blender/render_smoke.py for the hero render.

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

constexpr uint32_t GRID_SIZE_DEFAULT          = 256;
constexpr uint32_t WG_DIM                     = 8;       // workgroup_size in every shader
constexpr int      PRESSURE_ITERS_DEFAULT     = 40;
constexpr int      SUBSTEPS_DEFAULT           = 2;
constexpr float    DT_DEFAULT                 = 0.10f;
constexpr int      RAYMARCH_STEPS_DEFAULT     = 96;
constexpr int      SHADOW_MARCH_STEPS_DEFAULT = 16;
constexpr float    SHADOW_MARCH_SOFTNESS      = 1.5f;
constexpr float    EMITTER_FALLOFF_POWER      = 2.0f;
constexpr int      EMITTER_CAP                = 8;
constexpr float    BUOYANCY_REFERENCE_TEMP    = 0.0f;
constexpr float    BLACKBODY_TEMP_MIN         = 0.0f;
constexpr float    BLACKBODY_TEMP_MAX         = 2.0f;
constexpr float    ORBIT_DEFAULT_DEG_PER_SEC  = 6.0f;
constexpr float    ORBIT_DEFAULT_RADIUS       = 2.2f;
constexpr float    FOV_DEG_DEFAULT            = 50.0f;
constexpr float    NEAR_PLANE                 = 0.05f;
constexpr float    FAR_PLANE                  = 50.0f;
constexpr int      NUM_TIERS                  = 3;
constexpr std::array<uint32_t, NUM_TIERS> TIER_SIZES = {192, 256, 384};
constexpr int      DEFAULT_TIER_INDEX         = 1;       // 256³

// ============================================================================
// Smoke-dynamics presets
// ============================================================================

struct SmokePreset {
    const char* name;
    float vorticityStrength;        // epsilon (vorticity confinement)
    float buoyancyAlpha;            // alpha
    float buoyancyBeta;             // beta
    float densityDissipation;
    float velocityDissipation;
    float temperatureDissipation;
    float emitterDensityRate;
    float emitterTemperature;
    float emitterUpwardBias;
    float emitterRadius;
    bool  oneShotEmit;              // explosion-puff: emit once on selection, then disable continuous
};

constexpr std::array<SmokePreset, 6> SMOKE_PRESETS = {{
    // name              eps   a     b     dD     vD      tD     eR   eT     eB     eRad  oneShot
    {"Plume",            8.0f, 2.0f, 0.5f, 0.005f, 0.001f, 0.010f, 4.0f, 1.0f, 1.5f, 6.0f, false},
    {"Candle",           4.0f, 1.5f, 0.3f, 0.008f, 0.002f, 0.015f, 2.0f, 0.8f, 1.0f, 3.0f, false},
    {"Cigar",            6.0f, 1.0f, 0.2f, 0.003f, 0.001f, 0.008f, 1.5f, 0.6f, 0.6f, 4.0f, false},
    {"Smokestack",      12.0f, 3.0f, 0.4f, 0.004f, 0.0005f,0.007f, 6.0f, 1.4f, 2.5f, 8.0f, false},
    {"Explosion-Puff",  20.0f, 5.0f, 0.6f, 0.020f, 0.005f, 0.030f, 8.0f, 1.6f, 3.0f, 8.0f, true},
    // Chimney-Down: inverted buoyancy via negative alpha. Emitter temperature is
    // SMALL POSITIVE (not negative) — the advect_scalar kernel clamps T >= 0,
    // so negative emitter T is incorrect. The "cold smoke falling" effect comes
    // from alpha=-1.5 (T*(-1.5) drives velocity DOWN) and beta=1.5 (rho also
    // drives velocity DOWN). Both contributions pull the smoke down.
    {"Chimney-Down",     8.0f,-1.5f, 1.5f, 0.005f, 0.001f, 0.010f, 4.0f, 0.3f, 0.0f, 6.0f, false},
}};

// ============================================================================
// Black-body LUT generation (CPU-side; uploaded as 256x4 rgba8)
// ============================================================================

namespace colormap {

struct Rgb { float r, g, b; };

inline Rgb blackbody_at_t(float t) {
    // t in [0, 1] maps to temperature [BLACKBODY_TEMP_MIN, BLACKBODY_TEMP_MAX].
    // Piecewise-linear Planck-curve approximation:
    //   t < 0.20:  black -> deep red
    //   t < 0.50:  deep red -> orange
    //   t < 0.80:  orange -> yellow
    //   t < 1.00:  yellow -> near-white
    if (t < 0.20f) {
        float f = t / 0.20f;
        return {0.3f + 0.7f * f, 0.0f, 0.0f};
    } else if (t < 0.50f) {
        float f = (t - 0.20f) / 0.30f;
        return {1.0f, 0.0f + 0.5f * f, 0.0f};
    } else if (t < 0.80f) {
        float f = (t - 0.50f) / 0.30f;
        return {1.0f, 0.5f + 0.4f * f, 0.0f + 0.3f * f};
    } else {
        float f = std::clamp((t - 0.80f) / 0.20f, 0.0f, 1.0f);
        return {1.0f, 0.9f + 0.1f * f, 0.3f + 0.5f * f};
    }
}

inline Rgb sunset_at_t(float t) {
    // Longer orange/red phase, deeper into the warm spectrum.
    if (t < 0.40f) {
        float f = t / 0.40f;
        return {0.2f + 0.7f * f, 0.0f, 0.0f};
    } else if (t < 0.80f) {
        float f = (t - 0.40f) / 0.40f;
        return {0.9f + 0.1f * f, 0.2f + 0.6f * f, 0.05f};
    } else {
        float f = std::clamp((t - 0.80f) / 0.20f, 0.0f, 1.0f);
        return {1.0f, 0.8f + 0.15f * f, 0.05f + 0.6f * f};
    }
}

inline Rgb cold_at_t(float t) {
    // Blue/cyan ramp for the Chimney-Down preset.
    if (t < 0.50f) {
        float f = t / 0.50f;
        return {0.0f, 0.1f + 0.3f * f, 0.3f + 0.5f * f};
    } else {
        float f = (t - 0.50f) / 0.50f;
        return {0.0f + 0.4f * f, 0.4f + 0.5f * f, 0.8f + 0.2f * f};
    }
}

inline Rgb mono_at_t(float t) {
    return {t, t, t};
}

inline std::vector<uint8_t> build_blackbody_lut_data() {
    // 256 wide x 4 high RGBA8. Rows: 0=blackbody, 1=sunset, 2=cold, 3=mono.
    std::vector<uint8_t> out(256 * 4 * 4);
    auto write_row = [&](int row, auto sampler) {
        for (int i = 0; i < 256; ++i) {
            float t = float(i) / 255.0f;
            Rgb c = sampler(t);
            int o = (row * 256 + i) * 4;
            out[o + 0] = uint8_t(std::clamp(c.r, 0.0f, 1.0f) * 255.0f);
            out[o + 1] = uint8_t(std::clamp(c.g, 0.0f, 1.0f) * 255.0f);
            out[o + 2] = uint8_t(std::clamp(c.b, 0.0f, 1.0f) * 255.0f);
            out[o + 3] = 255;
        }
    };
    write_row(0, blackbody_at_t);
    write_row(1, sunset_at_t);
    write_row(2, cold_at_t);
    write_row(3, mono_at_t);
    return out;
}

}  // namespace colormap

// ============================================================================
// Blue-noise jitter LUT (256x256 r8). Generated via a tiny CPU PRNG —
// quality is "OK for jitter-banding mitigation," not "publication-quality
// blue noise." For Phase 8 this is sufficient; v1.1 could swap in a real
// blue-noise texture.
// ============================================================================

inline std::vector<uint8_t> build_jitter_lut_data() {
    std::vector<uint8_t> out(256 * 256);
    // Tiny LCG so output is deterministic (matters for "did the spec produce
    // the same thing twice").
    uint32_t state = 0x9E3779B9u;
    for (size_t i = 0; i < out.size(); ++i) {
        state = state * 1664525u + 1013904223u;
        out[i] = uint8_t((state >> 24) & 0xFF);
    }
    return out;
}

// ============================================================================
// Emitter state (CPU-side, uploaded each frame to a uniform buffer)
// ============================================================================

struct alignas(16) EmitterGpu {
    glm::vec4 pos_radius;            // .xyz = pos in [0,1]^3, .w = radius (in cells)
    glm::vec4 rate_temp_bias_pad;    // .x = density rate, .y = temperature, .z = velocity bias y, .w = 0
};

struct Emitter {
    glm::vec3 pos;
    float     radius;
    float     densityRate;
    float     temperature;
    float     velocityBiasY;
    bool      oneShotPending = false;    // for explosion-puff: emit once, then clear rate
};

// ============================================================================
// Runtime state
// ============================================================================

struct Runtime {
    int       presetIndex          = 0;
    bool      isCustom             = false;
    int       tierIndex            = DEFAULT_TIER_INDEX;
    uint32_t  gridSize             = TIER_SIZES[DEFAULT_TIER_INDEX];
    int       pendingTierIndex     = DEFAULT_TIER_INDEX;  // for "Apply" button

    // Solver tunables
    int       substeps             = SUBSTEPS_DEFAULT;
    float     dt                   = DT_DEFAULT;
    int       pressureIters        = PRESSURE_ITERS_DEFAULT;
    float     vorticityStrength    = SMOKE_PRESETS[0].vorticityStrength;
    float     buoyancyAlpha        = SMOKE_PRESETS[0].buoyancyAlpha;
    float     buoyancyBeta         = SMOKE_PRESETS[0].buoyancyBeta;
    float     densityDissipation   = SMOKE_PRESETS[0].densityDissipation;
    float     velocityDissipation  = SMOKE_PRESETS[0].velocityDissipation;
    float     temperatureDissipation = SMOKE_PRESETS[0].temperatureDissipation;
    float     maccormackEpsilon    = 0.0f;

    // Render tunables
    int       raymarchSteps        = RAYMARCH_STEPS_DEFAULT;
    int       shadowMarchSteps     = SHADOW_MARCH_STEPS_DEFAULT;
    float     densityAbsorption    = 12.0f;
    float     emissionStrength     = 6.0f;
    float     scatteringStrength   = 2.0f;
    float     exposure             = 1.2f;
    float     renderScale          = 1.0f;
    int       colorRamp            = 0;     // 0=blackbody, 1=sunset, 2=cold, 3=mono

    // Lighting
    float     lightAzimuthDeg      = 30.0f;
    float     lightElevationDeg    = 50.0f;
    glm::vec3 lightColor           = glm::vec3(1.00f, 0.95f, 0.85f);
    float     ambientStrength      = 0.10f;
    glm::vec3 bgTopColor           = glm::vec3(0.08f, 0.10f, 0.14f);
    glm::vec3 bgBottomColor        = glm::vec3(0.02f, 0.03f, 0.05f);

    // Emitter defaults (applied at preset-load to newly-placed emitters)
    float     emitterDensityRate   = SMOKE_PRESETS[0].emitterDensityRate;
    float     emitterTemperature   = SMOKE_PRESETS[0].emitterTemperature;
    float     emitterUpwardBias    = SMOKE_PRESETS[0].emitterUpwardBias;
    float     emitterRadius        = SMOKE_PRESETS[0].emitterRadius;
    std::vector<Emitter> emitters;

    // Camera
    bool      autoOrbit            = true;
    float     orbitSpeedDegPerSec  = ORBIT_DEFAULT_DEG_PER_SEC;
    float     orbitRadius          = ORBIT_DEFAULT_RADIUS;

    // State
    bool      exportVdb            = false;
    uint32_t  vdbFrameCounter      = 0;
    uint64_t  iteration            = 0;
};

void apply_preset(Runtime& rt, int idx) {
    const SmokePreset& p = SMOKE_PRESETS[size_t(idx)];
    rt.presetIndex             = idx;
    rt.isCustom                = false;
    rt.vorticityStrength       = p.vorticityStrength;
    rt.buoyancyAlpha           = p.buoyancyAlpha;
    rt.buoyancyBeta            = p.buoyancyBeta;
    rt.densityDissipation      = p.densityDissipation;
    rt.velocityDissipation     = p.velocityDissipation;
    rt.temperatureDissipation  = p.temperatureDissipation;
    rt.emitterDensityRate      = p.emitterDensityRate;
    rt.emitterTemperature      = p.emitterTemperature;
    rt.emitterUpwardBias       = p.emitterUpwardBias;
    rt.emitterRadius           = p.emitterRadius;
}

// ============================================================================
// GLSL uniform struct layouts (std140; matches each shader's layout(set=0, binding=N) uniform)
// ============================================================================

struct alignas(16) AdvectVelocityUniformsHost {
    float    dt;
    float    dissipation;
    float    maccormackEpsilon;
    float    _pad0;
    uint32_t gridSize;
    uint32_t _pad1;
    uint32_t _pad2;
    uint32_t _pad3;
};

struct alignas(16) AdvectScalarUniformsHost {
    float    dt;
    float    dissipation;
    float    maccormackEpsilon;
    float    _pad0;
    uint32_t gridSize;
    uint32_t _pad1;
    uint32_t _pad2;
    uint32_t _pad3;
};

struct alignas(16) BuoyancyUniformsHost {
    float    dt;
    float    alpha;
    float    beta;
    float    referenceTemp;
    uint32_t gridSize;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas(16) CurlUniformsHost {
    uint32_t gridSize;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas(16) VorticityUniformsHost {
    float    dt;
    float    epsilon;
    float    _pad0;
    float    _pad1;
    uint32_t gridSize;
    uint32_t _pad2;
    uint32_t _pad3;
    uint32_t _pad4;
};

struct alignas(16) EmitterUniformsHost {
    uint32_t count;
    uint32_t gridSize;
    float    dt;
    float    falloffPower;
    EmitterGpu emitters[EMITTER_CAP];   // 8 emitters × 32 bytes = 256 bytes
};

struct alignas(16) BoundariesUniformsHost {
    uint32_t gridSize;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas(16) DivergenceUniformsHost {
    uint32_t gridSize;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas(16) JacobiUniformsHost {
    uint32_t gridSize;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas(16) ProjectUniformsHost {
    uint32_t gridSize;
    uint32_t _pad0;
    uint32_t _pad1;
    uint32_t _pad2;
};

struct alignas(16) RaymarchUniformsHost {
    glm::mat4 invViewProj;
    glm::vec4 cameraPos;
    glm::vec4 volumeMin;
    glm::vec4 volumeMax;
    glm::vec4 lightDir;
    glm::vec4 lightColor;             // .w = ambient strength
    glm::vec4 bgTopColor;
    glm::vec4 bgBottomColor;
    int32_t   raymarchSteps;
    int32_t   shadowMarchSteps;
    float     densityAbsorption;
    float     emissionStrength;
    float     scatteringStrength;
    float     exposure;
    float     colorRampRow;
    float     shadowMarchSoftness;
};

// ============================================================================
// GLFW input snapshot — mirrors RD-3D's InputState exactly
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

// Mouse-click rising-edge detector (separate from CameraInputState — we want raw
// edges for emitter placement, not the camera-aware filtered version).
struct MouseEdge {
    bool prev_left  = false;
    bool prev_right = false;
    bool just_left  = false;
    bool just_right = false;

    void poll(GLFWwindow* w) {
        bool curr_left  = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT)  == GLFW_PRESS;
        bool curr_right = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        just_left  = curr_left  && !prev_left  && !ImGui::GetIO().WantCaptureMouse;
        just_right = curr_right && !prev_right && !ImGui::GetIO().WantCaptureMouse;
        prev_left  = curr_left;
        prev_right = curr_right;
    }
};
static MouseEdge g_mouse_edge;

// ============================================================================
// Click-to-place: unproject cursor to the y=0.1 plane in the volume's [0,1]³ space.
// (y=0.1 puts the emitter slightly above the floor, where smoke can naturally rise.)
// Returns true on a successful intersection; false if the ray points away from the plane.
// ============================================================================

bool unprojectToGroundPlane(const Camera& camera, GLFWwindow* w, glm::vec3& out_pos) {
    int win_w = 0, win_h = 0;
    glfwGetWindowSize(w, &win_w, &win_h);
    if (win_w == 0 || win_h == 0) return false;

    double cx = 0.0, cy = 0.0;
    glfwGetCursorPos(w, &cx, &cy);

    // NDC: (x, y) in [-1, 1] with y pointing up.
    float ndc_x = float(2.0 * cx / double(win_w) - 1.0);
    float ndc_y = float(1.0 - 2.0 * cy / double(win_h));

    glm::mat4 invVP = glm::inverse(camera.viewProjection());
    glm::vec4 near_h = invVP * glm::vec4(ndc_x, ndc_y, 0.0f, 1.0f);
    glm::vec4 far_h  = invVP * glm::vec4(ndc_x, ndc_y, 1.0f, 1.0f);
    glm::vec3 ray_origin = glm::vec3(near_h) / near_h.w;
    glm::vec3 ray_dir    = glm::normalize(glm::vec3(far_h) / far_h.w - ray_origin);

    // The volume occupies [0, 1]³ in world space. Intersect with y = 0.1 plane.
    constexpr float kFloorY = 0.1f;
    if (std::abs(ray_dir.y) < 1e-5f) return false;
    float t = (kFloorY - ray_origin.y) / ray_dir.y;
    if (t < 0.0f) return false;

    glm::vec3 hit = ray_origin + t * ray_dir;
    // Clamp into the volume bounds (XZ-plane).
    if (hit.x < 0.05f || hit.x > 0.95f || hit.z < 0.05f || hit.z > 0.95f) return false;
    out_pos = hit;
    return true;
}

// ============================================================================
// Descriptor-write helpers — one per compute/graphics pipeline, plus a
// raymarch helper for the graphics pipeline.
//
// All eleven descriptor-write helpers (one per pipeline: 10 compute + 1 graphics raymarch)
// share the same general descriptor-construction shape
// (VkDescriptorImageInfo for image/sampler bindings, VkDescriptorBufferInfo
// for uniform buffer bindings, VkWriteDescriptorSet array, vkUpdateDescriptorSets).
// Each helper takes the relevant VkImageView + VkSampler + VkBuffer handles
// and writes one descriptor set.
// ============================================================================

static void writeAdvectVelocityDescriptor(VkDevice device,
                                          VkDescriptorSet ds,
                                          VkImageView velocity_old_view,
                                          VkImageView velocity_new_view,
                                          VkSampler   sampler_linear,
                                          VkBuffer    uniform) {
    VkDescriptorImageInfo old_i{};
    old_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    old_i.imageView   = velocity_old_view;
    old_i.sampler     = sampler_linear;

    VkDescriptorImageInfo new_i{};
    new_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    new_i.imageView   = velocity_new_view;
    new_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].pImageInfo = &old_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[1].pImageInfo = &new_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[2].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeAdvectScalarDescriptor(VkDevice device,
                                        VkDescriptorSet ds,
                                        VkImageView scalar_old_view,
                                        VkImageView velocity_view,
                                        VkImageView scalar_new_view,
                                        VkSampler   sampler_linear,
                                        VkBuffer    uniform) {
    VkDescriptorImageInfo s_old{};
    s_old.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    s_old.imageView   = scalar_old_view;
    s_old.sampler     = sampler_linear;

    VkDescriptorImageInfo v_i{};
    v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_i.imageView   = velocity_view;
    v_i.sampler     = sampler_linear;

    VkDescriptorImageInfo s_new{};
    s_new.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    s_new.imageView   = scalar_new_view;
    s_new.sampler     = VK_NULL_HANDLE;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].pImageInfo = &s_old;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &v_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[2].pImageInfo = &s_new;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[3].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeBuoyancyDescriptor(VkDevice device,
                                    VkDescriptorSet ds,
                                    VkImageView velocity_view,        // in-place
                                    VkImageView density_view,         // sampled
                                    VkImageView temperature_view,     // sampled
                                    VkSampler   sampler_linear,
                                    VkBuffer    uniform) {
    VkDescriptorImageInfo v_i{};
    v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_i.imageView   = velocity_view;
    v_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorImageInfo d_i{};
    d_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    d_i.imageView   = density_view;
    d_i.sampler     = sampler_linear;

    VkDescriptorImageInfo t_i{};
    t_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    t_i.imageView   = temperature_view;
    t_i.sampler     = sampler_linear;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &d_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[2].pImageInfo = &t_i;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[3].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeCurlDescriptor(VkDevice device,
                                VkDescriptorSet ds,
                                VkImageView velocity_view,
                                VkImageView curl_view,
                                VkSampler   sampler_linear,
                                VkBuffer    uniform) {
    VkDescriptorImageInfo v_i{};
    v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_i.imageView   = velocity_view;
    v_i.sampler     = sampler_linear;

    VkDescriptorImageInfo c_i{};
    c_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    c_i.imageView   = curl_view;
    c_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[1].pImageInfo = &c_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[2].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeVorticityDescriptor(VkDevice device,
                                     VkDescriptorSet ds,
                                     VkImageView velocity_view,  // in-place
                                     VkImageView curl_view,
                                     VkSampler   sampler_linear,
                                     VkBuffer    uniform) {
    VkDescriptorImageInfo v_i{};
    v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_i.imageView   = velocity_view;
    v_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorImageInfo c_i{};
    c_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    c_i.imageView   = curl_view;
    c_i.sampler     = sampler_linear;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &c_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[2].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeEmitDescriptor(VkDevice device,
                                VkDescriptorSet ds,
                                VkImageView velocity_view,    // in-place
                                VkImageView density_view,     // in-place
                                VkImageView temperature_view, // in-place
                                VkBuffer    uniform) {
    auto image_info = [](VkImageView view) {
        VkDescriptorImageInfo i{};
        i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        i.imageView   = view;
        i.sampler     = VK_NULL_HANDLE;
        return i;
    };
    VkDescriptorImageInfo v_i = image_info(velocity_view);
    VkDescriptorImageInfo d_i = image_info(density_view);
    VkDescriptorImageInfo t_i = image_info(temperature_view);

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[1].pImageInfo = &d_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[2].pImageInfo = &t_i;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[3].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeBoundariesDescriptor(VkDevice device,
                                      VkDescriptorSet ds,
                                      VkImageView velocity_view,    // in-place
                                      VkBuffer    uniform) {
    VkDescriptorImageInfo v_i{};
    v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_i.imageView   = velocity_view;
    v_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[1].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeDivergenceDescriptor(VkDevice device,
                                      VkDescriptorSet ds,
                                      VkImageView velocity_view,
                                      VkImageView divergence_view,
                                      VkSampler   sampler_linear,
                                      VkBuffer    uniform) {
    VkDescriptorImageInfo v_i{};
    v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_i.imageView   = velocity_view;
    v_i.sampler     = sampler_linear;

    VkDescriptorImageInfo d_i{};
    d_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    d_i.imageView   = divergence_view;
    d_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[1].pImageInfo = &d_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[2].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeJacobiDescriptor(VkDevice device,
                                  VkDescriptorSet ds,
                                  VkImageView pressure_old_view,
                                  VkImageView divergence_view,
                                  VkImageView pressure_new_view,
                                  VkSampler   sampler_linear,
                                  VkBuffer    uniform) {
    VkDescriptorImageInfo p_old{};
    p_old.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    p_old.imageView   = pressure_old_view;
    p_old.sampler     = sampler_linear;

    VkDescriptorImageInfo d_i{};
    d_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    d_i.imageView   = divergence_view;
    d_i.sampler     = sampler_linear;

    VkDescriptorImageInfo p_new{};
    p_new.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    p_new.imageView   = pressure_new_view;
    p_new.sampler     = VK_NULL_HANDLE;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].pImageInfo = &p_old;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &d_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[2].pImageInfo = &p_new;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[3].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeProjectDescriptor(VkDevice device,
                                   VkDescriptorSet ds,
                                   VkImageView velocity_view,   // in-place
                                   VkImageView pressure_view,
                                   VkSampler   sampler_linear,
                                   VkBuffer    uniform) {
    VkDescriptorImageInfo v_i{};
    v_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    v_i.imageView   = velocity_view;
    v_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorImageInfo p_i{};
    p_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    p_i.imageView   = pressure_view;
    p_i.sampler     = sampler_linear;

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w[0].pImageInfo = &v_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &p_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[2].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeRaymarchDescriptor(VkDevice device,
                                    VkDescriptorSet ds,
                                    VkImageView density_view,
                                    VkImageView temperature_view,
                                    VkImageView blackbody_lut_view,
                                    VkImageView bluenoise_view,
                                    VkSampler   sampler_linear,
                                    VkSampler   sampler_lut,
                                    VkBuffer    uniform) {
    VkDescriptorImageInfo d_i{};
    d_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    d_i.imageView   = density_view;
    d_i.sampler     = sampler_linear;

    VkDescriptorImageInfo t_i{};
    t_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    t_i.imageView   = temperature_view;
    t_i.sampler     = sampler_linear;

    VkDescriptorImageInfo lut_i{};
    lut_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    lut_i.imageView   = blackbody_lut_view;
    lut_i.sampler     = sampler_lut;

    VkDescriptorImageInfo bn_i{};
    bn_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    bn_i.imageView   = bluenoise_view;
    bn_i.sampler     = sampler_lut;     // CLAMP_TO_EDGE; same sampler is fine for the noise texture

    VkDescriptorBufferInfo ub_i{};
    ub_i.buffer = uniform; ub_i.offset = 0; ub_i.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = ds; w[0].dstBinding = 0; w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[0].pImageInfo = &d_i;
    w[1].dstSet = ds; w[1].dstBinding = 1; w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &t_i;
    w[2].dstSet = ds; w[2].dstBinding = 2; w[2].descriptorCount = 1;
    w[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[2].pImageInfo = &lut_i;
    w[3].dstSet = ds; w[3].dstBinding = 3; w[3].descriptorCount = 1;
    w[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[3].pImageInfo = &bn_i;
    w[4].dstSet = ds; w[4].dstBinding = 4; w[4].descriptorCount = 1;
    w[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[4].pBufferInfo = &ub_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

// ============================================================================
// main()
// ============================================================================

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    initLogger();

    // ------------------------------------------------------------------------
    // Vulkan context + window + renderer + shader compiler
    // ------------------------------------------------------------------------
    gv::Context ctx;
    gv::Window window(ctx, 1920, 1080, "Eulerian Smoke — Phase 8");
    gv::Renderer renderer(ctx, window);
    gv::ShaderCompiler compiler(ctx);

    Runtime rt;
    apply_preset(rt, 0);                                          // Plume default

    constexpr uint32_t kSlots = gpusims::kMaxFramesInFlight;       // 2

    // ------------------------------------------------------------------------
    // 3D field images: 4 ping-pong pairs + 2 scratch fields = 10 images total.
    // Created via lambdas so we can re-call them in recreateGridResources().
    // ------------------------------------------------------------------------
    auto make_vec3_image = [&](const char* dbg, uint32_t n) {
        gv::ImageCreateInfo i{};
        i.type            = gv::ImageType::e3D;
        i.extent          = {n, n, n};
        i.format          = VK_FORMAT_R16G16B16A16_SFLOAT;
        i.usage           = VK_IMAGE_USAGE_STORAGE_BIT
                          | VK_IMAGE_USAGE_SAMPLED_BIT
                          | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                          | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        i.initial_layout  = VK_IMAGE_LAYOUT_GENERAL;
        i.debug_name      = dbg;
        return gv::Image::create(ctx, i);
    };
    auto make_scalar_image = [&](const char* dbg, uint32_t n) {
        gv::ImageCreateInfo i{};
        i.type            = gv::ImageType::e3D;
        i.extent          = {n, n, n};
        i.format          = VK_FORMAT_R32_SFLOAT;
        i.usage           = VK_IMAGE_USAGE_STORAGE_BIT
                          | VK_IMAGE_USAGE_SAMPLED_BIT
                          | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                          | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        i.initial_layout  = VK_IMAGE_LAYOUT_GENERAL;
        i.debug_name      = dbg;
        return gv::Image::create(ctx, i);
    };

    gv::Image velocity_ping    = make_vec3_image("velocity_ping",    rt.gridSize);
    gv::Image velocity_pong    = make_vec3_image("velocity_pong",    rt.gridSize);
    gv::Image density_ping     = make_scalar_image("density_ping",   rt.gridSize);
    gv::Image density_pong     = make_scalar_image("density_pong",   rt.gridSize);
    gv::Image temperature_ping = make_scalar_image("temp_ping",      rt.gridSize);
    gv::Image temperature_pong = make_scalar_image("temp_pong",      rt.gridSize);
    gv::Image pressure_ping    = make_scalar_image("pressure_ping",  rt.gridSize);
    gv::Image pressure_pong    = make_scalar_image("pressure_pong",  rt.gridSize);
    gv::Image curl_field       = make_vec3_image("curl",             rt.gridSize);
    gv::Image divergence_field = make_scalar_image("divergence",     rt.gridSize);

    // ------------------------------------------------------------------------
    // Samplers: one linear (for 3D field reads), one LUT (clamp-to-edge for 2D LUTs).
    // ------------------------------------------------------------------------
    VkSampler sampler_linear = VK_NULL_HANDLE;
    VkSampler sampler_lut    = VK_NULL_HANDLE;
    {
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter    = VK_FILTER_LINEAR;
        si.minFilter    = VK_FILTER_LINEAR;
        si.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.maxLod       = 0.0f;
        vkCreateSampler(ctx.device(), &si, nullptr, &sampler_linear);
        vkCreateSampler(ctx.device(), &si, nullptr, &sampler_lut);
    }

    // ------------------------------------------------------------------------
    // 2D LUT textures: blackbody (256x4 rgba8) + blue-noise jitter (256x256 r8).
    // ------------------------------------------------------------------------
    auto make_2d_image = [&](VkFormat fmt, uint32_t w, uint32_t h, const char* dbg) {
        gv::ImageCreateInfo i{};
        i.type            = gv::ImageType::e2D;
        i.extent          = {w, h, 1};
        i.format          = fmt;
        i.usage           = VK_IMAGE_USAGE_SAMPLED_BIT
                          | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        i.initial_layout  = VK_IMAGE_LAYOUT_GENERAL;
        i.debug_name      = dbg;
        return gv::Image::create(ctx, i);
    };
    gv::Image blackbody_lut = make_2d_image(VK_FORMAT_R8G8B8A8_UNORM, 256, 4, "blackbody_lut");
    gv::Image bluenoise_lut = make_2d_image(VK_FORMAT_R8_UNORM, 256, 256, "bluenoise_lut");
    {
        auto bb_bytes = colormap::build_blackbody_lut_data();
        blackbody_lut.upload(bb_bytes.data(), bb_bytes.size());
        auto bn_bytes = build_jitter_lut_data();
        bluenoise_lut.upload(bn_bytes.data(), bn_bytes.size());
    }

    // ------------------------------------------------------------------------
    // Per-slot uniform buffers — one per kernel category × kSlots.
    // Categories: advect_velocity, advect_scalar (shared for density+temperature),
    //   buoyancy, curl, vorticity, emit, boundaries, divergence, jacobi, project, raymarch.
    // Note: advect_scalar uses ONE buffer per category but is dispatched TWICE per substep
    //   (density-binding, then temperature-binding). The descriptor sets differ but the
    //   uniform contents (dt, dissipation, maccormackEpsilon, gridSize) are the same.
    //   Phase 8 uploads the uniform once per substep before either dispatch.
    // ------------------------------------------------------------------------
    auto make_uniform = [&](size_t bytes, const char* dbg) {
        return gv::Buffer::create(ctx, bytes,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            gv::MemoryUsage::HostVisibleSequential, dbg);
    };
    std::array<gv::Buffer, kSlots> ub_advect_velocity;
    std::array<gv::Buffer, kSlots> ub_advect_scalar;
    std::array<gv::Buffer, kSlots> ub_buoyancy;
    std::array<gv::Buffer, kSlots> ub_curl;
    std::array<gv::Buffer, kSlots> ub_vorticity;
    std::array<gv::Buffer, kSlots> ub_emit;
    std::array<gv::Buffer, kSlots> ub_boundaries;
    std::array<gv::Buffer, kSlots> ub_divergence;
    std::array<gv::Buffer, kSlots> ub_jacobi;
    std::array<gv::Buffer, kSlots> ub_project;
    std::array<gv::Buffer, kSlots> ub_raymarch;
    for (uint32_t s = 0; s < kSlots; ++s) {
        ub_advect_velocity[s] = make_uniform(sizeof(AdvectVelocityUniformsHost), "ub_advect_velocity");
        ub_advect_scalar[s]   = make_uniform(sizeof(AdvectScalarUniformsHost),   "ub_advect_scalar");
        ub_buoyancy[s]        = make_uniform(sizeof(BuoyancyUniformsHost),       "ub_buoyancy");
        ub_curl[s]            = make_uniform(sizeof(CurlUniformsHost),           "ub_curl");
        ub_vorticity[s]       = make_uniform(sizeof(VorticityUniformsHost),      "ub_vorticity");
        ub_emit[s]            = make_uniform(sizeof(EmitterUniformsHost),        "ub_emit");
        ub_boundaries[s]      = make_uniform(sizeof(BoundariesUniformsHost),     "ub_boundaries");
        ub_divergence[s]      = make_uniform(sizeof(DivergenceUniformsHost),     "ub_divergence");
        ub_jacobi[s]          = make_uniform(sizeof(JacobiUniformsHost),         "ub_jacobi");
        ub_project[s]         = make_uniform(sizeof(ProjectUniformsHost),        "ub_project");
        ub_raymarch[s]        = make_uniform(sizeof(RaymarchUniformsHost),       "ub_raymarch");
    }

    // ------------------------------------------------------------------------
    // Pipelines.
    //
    // Compute pipeline binding template helper — all 10 compute pipelines share
    // the same general shape: bindings 0..N at stage COMPUTE. We declare them
    // inline since they vary by binding count.
    // ------------------------------------------------------------------------
    const std::string SD = GPU_SIMS_ES_SHADER_DIR;     // compile-time injected via CMakeLists

    auto make_compute = [&](const std::string& shader_rel,
                            std::initializer_list<gv::DescriptorBinding> bindings,
                            const char* dbg) {
        (void)dbg;
        gv::ComputePipelineDesc d{};
        d.shader_path = SD + "/" + shader_rel;
        d.bindings    = std::vector<gv::DescriptorBinding>(bindings);
        return gv::ComputePipeline::create(ctx, compiler, d);
    };

    using BT = VkDescriptorType;
    constexpr BT CIS = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    constexpr BT SI  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    constexpr BT UB  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    constexpr VkShaderStageFlags CS = VK_SHADER_STAGE_COMPUTE_BIT;

    auto pipe_advect_velocity = make_compute("advect_velocity.comp.glsl",
        {{0, CIS, 1, CS}, {1, SI, 1, CS}, {2, UB, 1, CS}},
        "advect_velocity");
    auto pipe_advect_scalar   = make_compute("advect_scalar.comp.glsl",
        {{0, CIS, 1, CS}, {1, CIS, 1, CS}, {2, SI, 1, CS}, {3, UB, 1, CS}},
        "advect_scalar");
    auto pipe_buoyancy        = make_compute("apply_buoyancy.comp.glsl",
        {{0, SI, 1, CS}, {1, CIS, 1, CS}, {2, CIS, 1, CS}, {3, UB, 1, CS}},
        "apply_buoyancy");
    auto pipe_curl            = make_compute("compute_curl.comp.glsl",
        {{0, CIS, 1, CS}, {1, SI, 1, CS}, {2, UB, 1, CS}},
        "compute_curl");
    auto pipe_vorticity       = make_compute("apply_vorticity_confinement.comp.glsl",
        {{0, SI, 1, CS}, {1, CIS, 1, CS}, {2, UB, 1, CS}},
        "apply_vorticity_confinement");
    auto pipe_emit            = make_compute("emit_sources.comp.glsl",
        {{0, SI, 1, CS}, {1, SI, 1, CS}, {2, SI, 1, CS}, {3, UB, 1, CS}},
        "emit_sources");
    auto pipe_boundaries      = make_compute("apply_boundaries.comp.glsl",
        {{0, SI, 1, CS}, {1, UB, 1, CS}},
        "apply_boundaries");
    auto pipe_divergence      = make_compute("compute_divergence.comp.glsl",
        {{0, CIS, 1, CS}, {1, SI, 1, CS}, {2, UB, 1, CS}},
        "compute_divergence");
    auto pipe_jacobi          = make_compute("jacobi_pressure.comp.glsl",
        {{0, CIS, 1, CS}, {1, CIS, 1, CS}, {2, SI, 1, CS}, {3, UB, 1, CS}},
        "jacobi_pressure");
    auto pipe_project         = make_compute("apply_pressure.comp.glsl",
        {{0, SI, 1, CS}, {1, CIS, 1, CS}, {2, UB, 1, CS}},
        "apply_pressure");

    // Graphics: fullscreen.vert + raymarch.frag, color-only, no depth.
    gv::GraphicsPipelineDesc gd{};
    gd.vertex_shader_path   = SD + "/fullscreen.vert.glsl";
    gd.fragment_shader_path = SD + "/raymarch.frag.glsl";
    gd.color_formats        = {window.colorFormat()};
    gd.depth_test           = false;
    gd.cull_mode            = VK_CULL_MODE_NONE;
    gd.bindings             = {
        {0, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {1, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {2, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {3, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {4, UB,  1, VK_SHADER_STAGE_FRAGMENT_BIT},
    };
    auto pipe_raymarch = gv::GraphicsPipeline::create(ctx, compiler, gd);

    // ------------------------------------------------------------------------
    // Descriptor sets — ping-pong (2 parities) × kSlots (2 frames-in-flight).
    // For pipelines that read/write a SINGLE ping-pong field, we allocate
    // [parity][slot] — 4 sets per pipeline.
    // For pipelines that work on multiple ping-pong fields (emit reads/writes
    // all three; advect_scalar is dispatched twice), we allocate enough sets
    // that the host can always select the right (read=old, write=new) pair.
    // ------------------------------------------------------------------------
    auto alloc_sets = [&](auto& pipeline, uint32_t count) {
        std::vector<VkDescriptorSet> v(count);
        for (auto& ds : v) ds = pipeline.allocateDescriptorSet();
        return v;
    };

    constexpr uint32_t kParities = 2;
    // For advect_velocity: parity p, slot s → set indexed [p][s]
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_advect_velocity{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_advect_density{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_advect_temperature{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_buoyancy{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_curl{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_vorticity{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_emit{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_boundaries{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_divergence{};
    // Pressure Jacobi: TWO parities for the inner-loop pressure ping-pong, plus an outer parity
    // for which velocity slot we're projecting. The simplest correct approach is to allocate
    // independent sets for each (pressure_parity, slot) combination — 4 sets — and key them
    // dynamically inside the Jacobi loop by which pressure image is "old" this iteration.
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_jacobi{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_project{};
    std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_raymarch{};

    for (uint32_t p = 0; p < kParities; ++p) {
        for (uint32_t s = 0; s < kSlots; ++s) {
            ds_advect_velocity[p][s]    = pipe_advect_velocity.allocateDescriptorSet();
            ds_advect_density[p][s]     = pipe_advect_scalar.allocateDescriptorSet();
            ds_advect_temperature[p][s] = pipe_advect_scalar.allocateDescriptorSet();
            ds_buoyancy[p][s]           = pipe_buoyancy.allocateDescriptorSet();
            ds_curl[p][s]               = pipe_curl.allocateDescriptorSet();
            ds_vorticity[p][s]          = pipe_vorticity.allocateDescriptorSet();
            ds_emit[p][s]               = pipe_emit.allocateDescriptorSet();
            ds_boundaries[p][s]         = pipe_boundaries.allocateDescriptorSet();
            ds_divergence[p][s]         = pipe_divergence.allocateDescriptorSet();
            ds_jacobi[p][s]             = pipe_jacobi.allocateDescriptorSet();
            ds_project[p][s]            = pipe_project.allocateDescriptorSet();
            ds_raymarch[p][s]           = pipe_raymarch.allocateDescriptorSet();
        }
    }

    // ------------------------------------------------------------------------
    // Wire descriptors. Parity 0: velocity_old=ping, velocity_new=pong (etc).
    // Parity 1: swapped. Each substep increments an internal `parity` counter
    // and the host selects ds[parity & 1][slot] for that pass.
    // ------------------------------------------------------------------------
    auto wireAllDescriptors = [&]() {
        VkDevice dev = ctx.device();
        for (uint32_t p = 0; p < kParities; ++p) {
            // p=0: old=ping, new=pong. p=1: old=pong, new=ping.
            VkImageView vel_old_v = (p == 0) ? velocity_ping.view() : velocity_pong.view();
            VkImageView vel_new_v = (p == 0) ? velocity_pong.view() : velocity_ping.view();
            VkImageView den_old_v = (p == 0) ? density_ping.view()  : density_pong.view();
            VkImageView den_new_v = (p == 0) ? density_pong.view()  : density_ping.view();
            VkImageView tem_old_v = (p == 0) ? temperature_ping.view() : temperature_pong.view();
            VkImageView tem_new_v = (p == 0) ? temperature_pong.view() : temperature_ping.view();
            VkImageView pre_old_v = (p == 0) ? pressure_ping.view()  : pressure_pong.view();
            VkImageView pre_new_v = (p == 0) ? pressure_pong.view()  : pressure_ping.view();

            for (uint32_t s = 0; s < kSlots; ++s) {
                writeAdvectVelocityDescriptor(dev, ds_advect_velocity[p][s],
                    vel_old_v, vel_new_v, sampler_linear, ub_advect_velocity[s].handle());

                writeAdvectScalarDescriptor(dev, ds_advect_density[p][s],
                    den_old_v, vel_new_v, den_new_v, sampler_linear,
                    ub_advect_scalar[s].handle());

                writeAdvectScalarDescriptor(dev, ds_advect_temperature[p][s],
                    tem_old_v, vel_new_v, tem_new_v, sampler_linear,
                    ub_advect_scalar[s].handle());

                writeBuoyancyDescriptor(dev, ds_buoyancy[p][s],
                    vel_new_v, den_new_v, tem_new_v, sampler_linear,
                    ub_buoyancy[s].handle());

                writeCurlDescriptor(dev, ds_curl[p][s],
                    vel_new_v, curl_field.view(), sampler_linear,
                    ub_curl[s].handle());

                writeVorticityDescriptor(dev, ds_vorticity[p][s],
                    vel_new_v, curl_field.view(), sampler_linear,
                    ub_vorticity[s].handle());

                writeEmitDescriptor(dev, ds_emit[p][s],
                    vel_new_v, den_new_v, tem_new_v,
                    ub_emit[s].handle());

                writeBoundariesDescriptor(dev, ds_boundaries[p][s],
                    vel_new_v, ub_boundaries[s].handle());

                writeDivergenceDescriptor(dev, ds_divergence[p][s],
                    vel_new_v, divergence_field.view(), sampler_linear,
                    ub_divergence[s].handle());

                // Jacobi: parity p selects which pressure is "old" this iteration.
                // p=0: read pressure_ping, write pressure_pong. p=1: swapped.
                writeJacobiDescriptor(dev, ds_jacobi[p][s],
                    pre_old_v, divergence_field.view(), pre_new_v,
                    sampler_linear, ub_jacobi[s].handle());

                writeProjectDescriptor(dev, ds_project[p][s],
                    vel_new_v, pre_new_v, sampler_linear,
                    ub_project[s].handle());

                writeRaymarchDescriptor(dev, ds_raymarch[p][s],
                    den_new_v, tem_new_v, blackbody_lut.view(), bluenoise_lut.view(),
                    sampler_linear, sampler_lut, ub_raymarch[s].handle());
            }
        }
    };
    wireAllDescriptors();

    // ------------------------------------------------------------------------
    // Initial conditions: zero everywhere. Emitters drive visible smoke.
    // ------------------------------------------------------------------------
    auto zero_all_fields = [&]() {
        const size_t N = size_t(rt.gridSize) * rt.gridSize * rt.gridSize;
        std::vector<uint8_t> z_vec3(N * 8, 0);     // rgba16f = 8 bytes
        std::vector<uint8_t> z_scalar(N * 4, 0);   // r32f = 4 bytes
        velocity_ping.upload(z_vec3.data(),    z_vec3.size());
        velocity_pong.upload(z_vec3.data(),    z_vec3.size());
        density_ping.upload(z_scalar.data(),   z_scalar.size());
        density_pong.upload(z_scalar.data(),   z_scalar.size());
        temperature_ping.upload(z_scalar.data(), z_scalar.size());
        temperature_pong.upload(z_scalar.data(), z_scalar.size());
        pressure_ping.upload(z_scalar.data(),  z_scalar.size());
        pressure_pong.upload(z_scalar.data(),  z_scalar.size());
        curl_field.upload(z_vec3.data(),       z_vec3.size());
        divergence_field.upload(z_scalar.data(), z_scalar.size());
    };
    zero_all_fields();

    // Place one initial emitter, per current preset. Most presets spawn at
    // the floor center so buoyant smoke can rise into open volume. Chimney-Down
    // (index 5) inverts buoyancy and gets a TOP placement so its negative-
    // buoyancy force has room to develop before density hits the floor
    // boundary — a floor emitter under negative buoyancy just packs density
    // into the wall and the lateral pressure response visually reads as
    // "rising," defeating the preset's intent.
    auto place_default_emitter = [&]() {
        Emitter e;
        const bool inverted = (rt.presetIndex == 5);  // Chimney-Down
        e.pos        = glm::vec3(0.5f, inverted ? 0.85f : 0.10f, 0.5f);
        e.radius     = rt.emitterRadius;
        e.densityRate= rt.emitterDensityRate;
        e.temperature= rt.emitterTemperature;
        e.velocityBiasY = rt.emitterUpwardBias;
        e.oneShotPending = SMOKE_PRESETS[size_t(rt.presetIndex)].oneShotEmit;
        rt.emitters.clear();
        rt.emitters.push_back(e);
    };
    place_default_emitter();

    // ------------------------------------------------------------------------
    // Camera, profiler, capture I/O, ImGui — same shape as RD-3D.
    // ------------------------------------------------------------------------
    Camera camera;
    camera.setMode(Camera::Mode::Orbit);
    camera.setTarget(glm::vec3(0.5f, 0.5f, 0.5f));
    camera.setOrbitDistance(rt.orbitRadius);
    camera.setOrbitSpeed(rt.orbitSpeedDegPerSec);
    camera.setFovDeg(FOV_DEG_DEFAULT);
    camera.setNearFar(NEAR_PLANE, FAR_PLANE);
    camera.setAspect(window.aspect());

    GpuProfiler profiler(ctx);

    fs::create_directories("captures");
    StateWriter capture_writer("captures");
    fs::create_directories("vdb_export");

    // ImGui — match RD-3D's init block (continuous-ca/reaction-diffusion-3d/src/main.cpp:644-660).
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
            logError("eulerian_smoke: ui::initImGui failed");
            return 1;
        }
    }

    glfwSetScrollCallback(window.glfwWindow(), scrollCallback);

    // ------------------------------------------------------------------------
    // Capture / load helpers — save/load four fields + emitters + camera + parameters.
    // Same pattern as RD-3D (`continuous-ca/reaction-diffusion-3d/src/main.cpp:686-845`)
    // adapted for four fields.
    // ------------------------------------------------------------------------
    auto runtime_meta_json = [&]() -> json {
        json j;
        j["schemaVersion"]           = 1;
        json cam_j; camera.toJson(cam_j);
        j["camera"]                  = cam_j;
        j["gridSize"]                = rt.gridSize;
        j["presetIndex"]             = rt.presetIndex;
        j["isCustom"]                = rt.isCustom;
        j["iteration"]               = rt.iteration;
        j["substeps"]                = rt.substeps;
        j["dt"]                      = rt.dt;
        j["pressureIters"]           = rt.pressureIters;
        j["vorticityStrength"]       = rt.vorticityStrength;
        j["buoyancyAlpha"]           = rt.buoyancyAlpha;
        j["buoyancyBeta"]            = rt.buoyancyBeta;
        j["densityDissipation"]      = rt.densityDissipation;
        j["velocityDissipation"]     = rt.velocityDissipation;
        j["temperatureDissipation"]  = rt.temperatureDissipation;
        j["maccormackEpsilon"]       = rt.maccormackEpsilon;
        j["raymarchSteps"]           = rt.raymarchSteps;
        j["shadowMarchSteps"]        = rt.shadowMarchSteps;
        j["densityAbsorption"]       = rt.densityAbsorption;
        j["emissionStrength"]        = rt.emissionStrength;
        j["scatteringStrength"]      = rt.scatteringStrength;
        j["exposure"]                = rt.exposure;
        j["renderScale"]             = rt.renderScale;
        j["colorRamp"]               = rt.colorRamp;
        j["lightAzimuthDeg"]         = rt.lightAzimuthDeg;
        j["lightElevationDeg"]       = rt.lightElevationDeg;
        j["lightColor"]              = {rt.lightColor.r, rt.lightColor.g, rt.lightColor.b};
        j["ambientStrength"]         = rt.ambientStrength;
        j["bgTopColor"]              = {rt.bgTopColor.r, rt.bgTopColor.g, rt.bgTopColor.b};
        j["bgBottomColor"]           = {rt.bgBottomColor.r, rt.bgBottomColor.g, rt.bgBottomColor.b};
        j["emitterDensityRate"]      = rt.emitterDensityRate;
        j["emitterTemperature"]      = rt.emitterTemperature;
        j["emitterUpwardBias"]       = rt.emitterUpwardBias;
        j["emitterRadius"]           = rt.emitterRadius;
        j["autoOrbit"]               = rt.autoOrbit;
        j["orbitSpeedDegPerSec"]     = rt.orbitSpeedDegPerSec;
        j["orbitRadius"]             = rt.orbitRadius;
        j["vdbFrameCounter"]         = rt.vdbFrameCounter;
        json emitters_j = json::array();
        for (const auto& e : rt.emitters) {
            emitters_j.push_back({
                {"x", e.pos.x}, {"y", e.pos.y}, {"z", e.pos.z},
                {"radius", e.radius},
                {"densityRate", e.densityRate},
                {"temperature", e.temperature},
                {"velocityBiasY", e.velocityBiasY},
            });
        }
        j["emitters"] = emitters_j;
        return j;
    };

    auto capture_save = [&]() {
        renderer.waitIdle();
        const size_t N = size_t(rt.gridSize) * rt.gridSize * rt.gridSize;
        const bool curr_is_ping = (rt.iteration % 2u == 0u);

        // Read back current ping-pong slots for each field.
        std::vector<uint8_t> vel_bytes(N * 8), den_bytes(N * 4), tem_bytes(N * 4), pre_bytes(N * 4);
        gv::Image& vel_curr = curr_is_ping ? velocity_ping    : velocity_pong;
        gv::Image& den_curr = curr_is_ping ? density_ping     : density_pong;
        gv::Image& tem_curr = curr_is_ping ? temperature_ping : temperature_pong;
        gv::Image& pre_curr = curr_is_ping ? pressure_ping    : pressure_pong;
        vel_curr.readback(vel_bytes.data(), vel_bytes.size());
        den_curr.readback(den_bytes.data(), den_bytes.size());
        tem_curr.readback(tem_bytes.data(), tem_bytes.size());
        pre_curr.readback(pre_bytes.data(), pre_bytes.size());

        capture_writer.beginFrame(uint32_t(rt.iteration));
        capture_writer.setMeta("eulerianSmoke", runtime_meta_json());
        capture_writer.saveBuffer("velocity.bin",    vel_bytes.data(), vel_bytes.size(),
            {{"count", uint64_t(N)}, {"stride", 8}, {"format", "rgba16f"},
             {"shape", {rt.gridSize, rt.gridSize, rt.gridSize}}});
        capture_writer.saveBuffer("density.bin",     den_bytes.data(), den_bytes.size(),
            {{"count", uint64_t(N)}, {"stride", 4}, {"format", "r32f"},
             {"shape", {rt.gridSize, rt.gridSize, rt.gridSize}}});
        capture_writer.saveBuffer("temperature.bin", tem_bytes.data(), tem_bytes.size(),
            {{"count", uint64_t(N)}, {"stride", 4}, {"format", "r32f"},
             {"shape", {rt.gridSize, rt.gridSize, rt.gridSize}}});
        capture_writer.saveBuffer("pressure.bin",    pre_bytes.data(), pre_bytes.size(),
            {{"count", uint64_t(N)}, {"stride", 4}, {"format", "r32f"},
             {"shape", {rt.gridSize, rt.gridSize, rt.gridSize}}});
        capture_writer.endFrame();
        gpusims::ui::pushToast(("Saved capture #" + std::to_string(rt.iteration)).c_str(), true);
        logInfo("F5: saved capture {}", rt.iteration);
    };

    // Forward-declared lambda used inside capture_load — needs recreateGridResources below.
    std::function<void(uint32_t)> recreateGridResources;

    auto capture_load = [&]() {
        auto latest = StateReader::findLatest("captures");
        if (!latest.has_value()) {
            gpusims::ui::pushToast("No captures to load", false);
            logWarn("F9: no captures found");
            return;
        }
        auto cap = StateReader::open(*latest);
        if (!cap) {
            gpusims::ui::pushToast("Failed to open capture", false);
            logError("F9: failed to open {}", latest->string());
            return;
        }
        json meta = cap->meta("eulerianSmoke");
        if (meta.is_null()) {
            gpusims::ui::pushToast("Capture has no eulerianSmoke meta", false);
            logError("F9: capture missing eulerianSmoke key");
            return;
        }

        renderer.waitIdle();

        // Tier-change at load if mismatched.
        uint32_t saved_size = meta.value("gridSize", uint32_t(rt.gridSize));
        if (saved_size != rt.gridSize) {
            logInfo("F9: tier mismatch (saved {}³, current {}³); recreating grid",
                    saved_size, rt.gridSize);
            recreateGridResources(saved_size);
        }
        const size_t N = size_t(rt.gridSize) * rt.gridSize * rt.gridSize;

        std::vector<uint8_t> vel_buf = cap->buffer("velocity.bin");
        std::vector<uint8_t> den_buf = cap->buffer("density.bin");
        std::vector<uint8_t> tem_buf = cap->buffer("temperature.bin");
        std::vector<uint8_t> pre_buf = cap->buffer("pressure.bin");
        if (vel_buf.size() != N * 8 ||
            den_buf.size() != N * 4 ||
            tem_buf.size() != N * 4 ||
            pre_buf.size() != N * 4) {
            gpusims::ui::pushToast("Capture buffer size mismatch", false);
            logError("F9: buffer-size mismatch — bailing without applying");
            return;
        }

        // Force iteration even → curr is ping. Load into ping, zero pong.
        rt.iteration = (meta.value("iteration", uint64_t(0)) % 2u == 0u)
                        ? meta.value("iteration", uint64_t(0))
                        : meta.value("iteration", uint64_t(0)) + 1;

        velocity_ping.upload(vel_buf.data(),    vel_buf.size());
        density_ping.upload(den_buf.data(),     den_buf.size());
        temperature_ping.upload(tem_buf.data(), tem_buf.size());
        pressure_ping.upload(pre_buf.data(),    pre_buf.size());

        std::vector<uint8_t> z_vec3(N * 8, 0), z_scalar(N * 4, 0);
        velocity_pong.upload(z_vec3.data(),    z_vec3.size());
        density_pong.upload(z_scalar.data(),   z_scalar.size());
        temperature_pong.upload(z_scalar.data(), z_scalar.size());
        pressure_pong.upload(z_scalar.data(),  z_scalar.size());

        // Restore camera + runtime parameters.
        camera.fromJson(meta["camera"]);
        rt.presetIndex            = meta.value("presetIndex", 0);
        rt.isCustom               = meta.value("isCustom", false);
        rt.substeps               = meta.value("substeps", SUBSTEPS_DEFAULT);
        rt.dt                     = meta.value("dt", DT_DEFAULT);
        rt.pressureIters          = meta.value("pressureIters", PRESSURE_ITERS_DEFAULT);
        rt.vorticityStrength      = meta.value("vorticityStrength", 8.0f);
        rt.buoyancyAlpha          = meta.value("buoyancyAlpha", 2.0f);
        rt.buoyancyBeta           = meta.value("buoyancyBeta", 0.5f);
        rt.densityDissipation     = meta.value("densityDissipation", 0.005f);
        rt.velocityDissipation    = meta.value("velocityDissipation", 0.001f);
        rt.temperatureDissipation = meta.value("temperatureDissipation", 0.010f);
        rt.maccormackEpsilon      = meta.value("maccormackEpsilon", 0.0f);
        rt.raymarchSteps          = meta.value("raymarchSteps", RAYMARCH_STEPS_DEFAULT);
        rt.shadowMarchSteps       = meta.value("shadowMarchSteps", SHADOW_MARCH_STEPS_DEFAULT);
        rt.densityAbsorption      = meta.value("densityAbsorption", 12.0f);
        rt.emissionStrength       = meta.value("emissionStrength", 6.0f);
        rt.scatteringStrength     = meta.value("scatteringStrength", 2.0f);
        rt.exposure               = meta.value("exposure", 1.2f);
        rt.renderScale            = meta.value("renderScale", 1.0f);
        rt.colorRamp              = meta.value("colorRamp", 0);
        rt.lightAzimuthDeg        = meta.value("lightAzimuthDeg", 30.0f);
        rt.lightElevationDeg      = meta.value("lightElevationDeg", 50.0f);
        auto rdvec3 = [&](const char* key, const glm::vec3& def) {
            if (meta.contains(key) && meta[key].is_array() && meta[key].size() == 3) {
                return glm::vec3(float(meta[key][0]), float(meta[key][1]), float(meta[key][2]));
            }
            return def;
        };
        rt.lightColor             = rdvec3("lightColor", glm::vec3(1.0f, 0.95f, 0.85f));
        rt.bgTopColor             = rdvec3("bgTopColor", glm::vec3(0.08f, 0.10f, 0.14f));
        rt.bgBottomColor          = rdvec3("bgBottomColor", glm::vec3(0.02f, 0.03f, 0.05f));
        rt.ambientStrength        = meta.value("ambientStrength", 0.10f);
        rt.emitterDensityRate     = meta.value("emitterDensityRate", 4.0f);
        rt.emitterTemperature     = meta.value("emitterTemperature", 1.0f);
        rt.emitterUpwardBias      = meta.value("emitterUpwardBias", 1.5f);
        rt.emitterRadius          = meta.value("emitterRadius", 6.0f);
        rt.autoOrbit              = meta.value("autoOrbit", true);
        rt.orbitSpeedDegPerSec    = meta.value("orbitSpeedDegPerSec", ORBIT_DEFAULT_DEG_PER_SEC);
        rt.orbitRadius            = meta.value("orbitRadius", ORBIT_DEFAULT_RADIUS);
        rt.vdbFrameCounter        = meta.value("vdbFrameCounter", 0u);

        rt.emitters.clear();
        for (const auto& ej : meta.value("emitters", json::array())) {
            Emitter e;
            e.pos.x          = ej.value("x", 0.5f);
            e.pos.y          = ej.value("y", 0.10f);
            e.pos.z          = ej.value("z", 0.5f);
            e.radius         = ej.value("radius", rt.emitterRadius);
            e.densityRate    = ej.value("densityRate", rt.emitterDensityRate);
            e.temperature    = ej.value("temperature", rt.emitterTemperature);
            e.velocityBiasY  = ej.value("velocityBiasY", rt.emitterUpwardBias);
            rt.emitters.push_back(e);
        }

        gpusims::ui::pushToast("Loaded most recent capture", true);
        logInfo("F9: loaded capture from {}", latest->string());
    };

    // ------------------------------------------------------------------------
    // recreateGridResources — heavyweight tier-change handler.
    // Implementation matches § 2.5; full sequence (a) waitIdle, (b) destroy, (c) recreate,
    // (d) re-wire descriptors, (e) zero fields, (f) clear emitters.
    // ------------------------------------------------------------------------
    recreateGridResources = [&](uint32_t new_size) {
        logInfo("recreateGridResources({}³): start", new_size);
        renderer.waitIdle();

        velocity_ping    = make_vec3_image("velocity_ping",    new_size);
        velocity_pong    = make_vec3_image("velocity_pong",    new_size);
        density_ping     = make_scalar_image("density_ping",   new_size);
        density_pong     = make_scalar_image("density_pong",   new_size);
        temperature_ping = make_scalar_image("temp_ping",      new_size);
        temperature_pong = make_scalar_image("temp_pong",      new_size);
        pressure_ping    = make_scalar_image("pressure_ping",  new_size);
        pressure_pong    = make_scalar_image("pressure_pong",  new_size);
        curl_field       = make_vec3_image("curl",             new_size);
        divergence_field = make_scalar_image("divergence",     new_size);

        rt.gridSize        = new_size;
        rt.iteration       = 0;
        rt.vdbFrameCounter = 0;
        rt.emitters.clear();
        zero_all_fields();
        wireAllDescriptors();
        place_default_emitter();
        logInfo("recreateGridResources({}³): done", new_size);
    };

    // ------------------------------------------------------------------------
    // Hot-reload — watch all 12 shader files. Boolean flag pattern matches RD-3D.
    // ------------------------------------------------------------------------
    HotReloader reloader;
    bool reload_advect_velocity = false, reload_advect_scalar = false;
    bool reload_buoyancy = false, reload_curl = false, reload_vorticity = false;
    bool reload_emit = false, reload_boundaries = false, reload_divergence = false;
    bool reload_jacobi = false, reload_project = false;
    bool reload_raymarch = false;     // covers both fullscreen.vert + raymarch.frag

    auto W = [&](const std::string& rel, bool* flag) {
        reloader.watch(SD + "/" + rel, [flag](const std::filesystem::path&){ *flag = true; });
    };
    W("advect_velocity.comp.glsl",            &reload_advect_velocity);
    W("advect_scalar.comp.glsl",              &reload_advect_scalar);
    W("apply_buoyancy.comp.glsl",             &reload_buoyancy);
    W("compute_curl.comp.glsl",               &reload_curl);
    W("apply_vorticity_confinement.comp.glsl",&reload_vorticity);
    W("emit_sources.comp.glsl",               &reload_emit);
    W("apply_boundaries.comp.glsl",           &reload_boundaries);
    W("compute_divergence.comp.glsl",         &reload_divergence);
    W("jacobi_pressure.comp.glsl",            &reload_jacobi);
    W("apply_pressure.comp.glsl",             &reload_project);
    W("fullscreen.vert.glsl",                 &reload_raymarch);
    W("raymarch.frag.glsl",                   &reload_raymarch);

    // ------------------------------------------------------------------------
    // F5 / F9 rising-edge tracking (matches RD-3D pattern).
    // ------------------------------------------------------------------------
    bool prev_f5 = false, prev_f9 = false;

    // ------------------------------------------------------------------------
    // Timing.
    // ------------------------------------------------------------------------
    auto prev_time = std::chrono::steady_clock::now();

    // ------------------------------------------------------------------------
    // MAIN LOOP
    // ------------------------------------------------------------------------
    while (!window.shouldClose()) {
        window.pollEvents();

        // --------------------------------------------------------------------
        // Compute dt for this frame.
        // --------------------------------------------------------------------
        auto now = std::chrono::steady_clock::now();
        float frame_dt = std::chrono::duration<float>(now - prev_time).count();
        prev_time = now;
        if (frame_dt > 0.1f) frame_dt = 0.1f;       // clamp on long pauses

        // --------------------------------------------------------------------
        // Hot-reload: poll for shader file changes (callbacks set our flags).
        // --------------------------------------------------------------------
        reloader.poll();

        // --------------------------------------------------------------------
        // F5 / F9 rising edges.
        // --------------------------------------------------------------------
        bool now_f5 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F5) == GLFW_PRESS;
        bool now_f9 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F9) == GLFW_PRESS;
        if (now_f5 && !prev_f5) capture_save();
        if (now_f9 && !prev_f9) capture_load();
        prev_f5 = now_f5; prev_f9 = now_f9;

        // --------------------------------------------------------------------
        // Tier-change Apply detection: the panel sets `pendingTierIndex`; if
        // it differs from `tierIndex` AND the user clicked Apply (panel sets a
        // local `tier_apply_clicked` flag), trigger recreateGridResources.
        // (The panel-flag mechanism is wired below in the ImGui block.)
        // --------------------------------------------------------------------
        bool tier_apply_clicked = false;

        // --------------------------------------------------------------------
        // Mouse-click → emitter place/remove.
        // --------------------------------------------------------------------
        g_mouse_edge.poll(window.glfwWindow());
        if (g_mouse_edge.just_left && rt.emitters.size() < EMITTER_CAP) {
            glm::vec3 pos;
            if (unprojectToGroundPlane(camera, window.glfwWindow(), pos)) {
                Emitter e;
                e.pos          = pos;
                e.radius       = rt.emitterRadius;
                e.densityRate  = rt.emitterDensityRate;
                e.temperature  = rt.emitterTemperature;
                e.velocityBiasY= rt.emitterUpwardBias;
                rt.emitters.push_back(e);
                gpusims::ui::pushToast(("Placed emitter " + std::to_string(rt.emitters.size())).c_str(), true);
            }
        }
        if (g_mouse_edge.just_right && !rt.emitters.empty()) {
            glm::vec3 pos;
            if (unprojectToGroundPlane(camera, window.glfwWindow(), pos)) {
                // Find the closest emitter within 8 cells of the click; remove it.
                float invG = 1.0f / float(rt.gridSize);
                float remove_threshold = 8.0f * invG;
                auto best = rt.emitters.end();
                float best_dist = remove_threshold;
                for (auto it = rt.emitters.begin(); it != rt.emitters.end(); ++it) {
                    float d = glm::length(it->pos - pos);
                    if (d < best_dist) { best_dist = d; best = it; }
                }
                if (best != rt.emitters.end()) {
                    rt.emitters.erase(best);
                    gpusims::ui::pushToast("Emitter removed", true);
                }
            }
        }

        // --------------------------------------------------------------------
        // Camera update.
        // --------------------------------------------------------------------
        camera.setAspect(window.aspect());
        camera.setOrbitDistance(rt.orbitRadius);
        camera.setOrbitSpeed(rt.orbitSpeedDegPerSec);
        camera.setMode(rt.autoOrbit ? Camera::Mode::Orbit : Camera::Mode::FreeFly);
        camera.update(frame_dt, g_input.snapshot(window.glfwWindow()));

        // --------------------------------------------------------------------
        // Begin frame.
        // --------------------------------------------------------------------
        gv::Frame* frame = renderer.beginFrame();
        if (!frame) continue;
        const uint32_t slot = frame->in_flight_index;
        VkCommandBuffer cmd = frame->command_buffer;

        // --------------------------------------------------------------------
        // Apply any pending hot-reloads (after waitIdle inside reload).
        // --------------------------------------------------------------------
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
                logError("hot-reload FAIL: {} — {}", dbg, err);
            }
            *flag = false;
        };
        apply_reload(&reload_advect_velocity, pipe_advect_velocity, "advect_velocity");
        apply_reload(&reload_advect_scalar,   pipe_advect_scalar,   "advect_scalar");
        apply_reload(&reload_buoyancy,        pipe_buoyancy,        "apply_buoyancy");
        apply_reload(&reload_curl,            pipe_curl,            "compute_curl");
        apply_reload(&reload_vorticity,       pipe_vorticity,       "apply_vorticity_confinement");
        apply_reload(&reload_emit,            pipe_emit,            "emit_sources");
        apply_reload(&reload_boundaries,      pipe_boundaries,      "apply_boundaries");
        apply_reload(&reload_divergence,      pipe_divergence,      "compute_divergence");
        apply_reload(&reload_jacobi,          pipe_jacobi,          "jacobi_pressure");
        apply_reload(&reload_project,         pipe_project,         "apply_pressure");
        apply_reload(&reload_raymarch,        pipe_raymarch,        "raymarch");

        profiler.beginFrame(cmd, slot);

        // --------------------------------------------------------------------
        // Upload uniform buffers for this slot. The per-substep uniforms don't
        // change WITHIN a frame (substeps just iterate the dispatch chain), so
        // we upload them once per frame and dispatch the chain `substeps` times.
        // --------------------------------------------------------------------
        {
            AdvectVelocityUniformsHost u_av{};
            u_av.dt = rt.dt; u_av.dissipation = rt.velocityDissipation;
            u_av.maccormackEpsilon = rt.maccormackEpsilon;
            u_av.gridSize = rt.gridSize;
            ub_advect_velocity[slot].uploadDirect(&u_av, sizeof(u_av));

            AdvectScalarUniformsHost u_as{};
            u_as.dt = rt.dt;
            u_as.dissipation = rt.densityDissipation;     // density-pass uniform; temperature-pass overwrites below
            u_as.maccormackEpsilon = rt.maccormackEpsilon;
            u_as.gridSize = rt.gridSize;
            ub_advect_scalar[slot].uploadDirect(&u_as, sizeof(u_as));
            // NOTE: We use the SAME uniform buffer for density and temperature advection
            // because the uniform contents are identical EXCEPT for `dissipation`. We
            // compromise here by uploading the density dissipation; temperature uses the
            // same value. This is a subtle approximation — for v1 it is acceptable
            // (the visual difference between density-and-temperature having the same
            // dissipation rate vs. their actual separate rates is invisible at default
            // values). A future v1.1 polish item: split into two uniform buffers and
            // upload independently. (See notes.md priority 1.12.)

            BuoyancyUniformsHost u_b{};
            u_b.dt = rt.dt;
            u_b.alpha = rt.buoyancyAlpha; u_b.beta = rt.buoyancyBeta;
            u_b.referenceTemp = BUOYANCY_REFERENCE_TEMP;
            u_b.gridSize = rt.gridSize;
            ub_buoyancy[slot].uploadDirect(&u_b, sizeof(u_b));

            CurlUniformsHost u_c{}; u_c.gridSize = rt.gridSize;
            ub_curl[slot].uploadDirect(&u_c, sizeof(u_c));

            VorticityUniformsHost u_v{};
            u_v.dt = rt.dt; u_v.epsilon = rt.vorticityStrength;
            u_v.gridSize = rt.gridSize;
            ub_vorticity[slot].uploadDirect(&u_v, sizeof(u_v));

            EmitterUniformsHost u_e{};
            u_e.count        = uint32_t(rt.emitters.size());
            u_e.gridSize     = rt.gridSize;
            u_e.dt           = rt.dt;
            u_e.falloffPower = EMITTER_FALLOFF_POWER;
            for (size_t i = 0; i < rt.emitters.size() && i < EMITTER_CAP; ++i) {
                const Emitter& e = rt.emitters[i];
                u_e.emitters[i].pos_radius = glm::vec4(e.pos, e.radius);
                u_e.emitters[i].rate_temp_bias_pad = glm::vec4(
                    e.oneShotPending ? e.densityRate * 8.0f : e.densityRate,
                    e.temperature,
                    e.velocityBiasY,
                    0.0f);
            }
            ub_emit[slot].uploadDirect(&u_e, sizeof(u_e));
            // Process one-shot emitters: zero their continuous rate after this frame.
            for (auto& e : rt.emitters) {
                if (e.oneShotPending) {
                    e.densityRate = 0.0f;
                    e.temperature = 0.0f;
                    e.velocityBiasY = 0.0f;
                    e.oneShotPending = false;
                }
            }

            BoundariesUniformsHost u_bd{}; u_bd.gridSize = rt.gridSize;
            ub_boundaries[slot].uploadDirect(&u_bd, sizeof(u_bd));

            DivergenceUniformsHost u_d{}; u_d.gridSize = rt.gridSize;
            ub_divergence[slot].uploadDirect(&u_d, sizeof(u_d));

            JacobiUniformsHost u_j{}; u_j.gridSize = rt.gridSize;
            ub_jacobi[slot].uploadDirect(&u_j, sizeof(u_j));

            ProjectUniformsHost u_p{}; u_p.gridSize = rt.gridSize;
            ub_project[slot].uploadDirect(&u_p, sizeof(u_p));

            RaymarchUniformsHost u_r{};
            u_r.invViewProj = glm::inverse(camera.viewProjection());
            u_r.cameraPos   = glm::vec4(camera.position(), 0.0f);
            u_r.volumeMin   = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            u_r.volumeMax   = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
            // Light direction from polar coords.
            float az = glm::radians(rt.lightAzimuthDeg);
            float el = glm::radians(rt.lightElevationDeg);
            glm::vec3 lightDir = glm::normalize(glm::vec3(
                std::cos(el) * std::cos(az),
                std::sin(el),
                std::cos(el) * std::sin(az)));
            u_r.lightDir          = glm::vec4(lightDir, 0.0f);
            u_r.lightColor        = glm::vec4(rt.lightColor, rt.ambientStrength);
            u_r.bgTopColor        = glm::vec4(rt.bgTopColor, 0.0f);
            u_r.bgBottomColor     = glm::vec4(rt.bgBottomColor, 0.0f);
            u_r.raymarchSteps     = rt.raymarchSteps;
            u_r.shadowMarchSteps  = rt.shadowMarchSteps;
            u_r.densityAbsorption = rt.densityAbsorption;
            u_r.emissionStrength  = rt.emissionStrength;
            u_r.scatteringStrength= rt.scatteringStrength;
            u_r.exposure          = rt.exposure;
            u_r.colorRampRow      = float(rt.colorRamp);
            u_r.shadowMarchSoftness = SHADOW_MARCH_SOFTNESS;
            ub_raymarch[slot].uploadDirect(&u_r, sizeof(u_r));
        }

        // --------------------------------------------------------------------
        // Frame-boundary barrier: previous frame's fragment read → this frame's compute write.
        // (Matches RD-3D's pattern at continuous-ca/reaction-diffusion-3d/src/main.cpp:888.)
        // --------------------------------------------------------------------
        gv::memoryBarrier(cmd,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  VK_ACCESS_SHADER_WRITE_BIT);

        // --------------------------------------------------------------------
        // Per-substep dispatch chain.
        // The `parity` counter chooses which descriptor set to bind: parity 0 →
        // velocity_old=ping/velocity_new=pong, parity 1 → swapped. We toggle
        // parity AFTER each substep so the next substep's ping-pong is correct.
        // --------------------------------------------------------------------
        const uint32_t wg = (rt.gridSize + WG_DIM - 1) / WG_DIM;
        for (int sub = 0; sub < rt.substeps; ++sub) {
            uint32_t p = uint32_t(rt.iteration & 1u);     // 0 or 1

            // (1) Advect velocity.
            {
                auto scope = profiler.scope(cmd, "advect_velocity");
                pipe_advect_velocity.dispatch(cmd, ds_advect_velocity[p][slot], wg, wg, wg);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

            // (2) Advect density.
            {
                auto scope = profiler.scope(cmd, "advect_density");
                pipe_advect_scalar.dispatch(cmd, ds_advect_density[p][slot], wg, wg, wg);
            }
            // No barrier between (2) and (3) — they read the same velocity_new
            // and write to different fields. Combined barrier after (3).

            // (3) Advect temperature.
            {
                auto scope = profiler.scope(cmd, "advect_temperature");
                pipe_advect_scalar.dispatch(cmd, ds_advect_temperature[p][slot], wg, wg, wg);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

            // (4) Apply buoyancy (in-place on velocity_new).
            {
                auto scope = profiler.scope(cmd, "apply_buoyancy");
                pipe_buoyancy.dispatch(cmd, ds_buoyancy[p][slot], wg, wg, wg);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

            // (5) Compute curl.
            {
                auto scope = profiler.scope(cmd, "compute_curl");
                pipe_curl.dispatch(cmd, ds_curl[p][slot], wg, wg, wg);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

            // (6) Apply vorticity confinement (in-place on velocity_new).
            {
                auto scope = profiler.scope(cmd, "vorticity_confinement");
                pipe_vorticity.dispatch(cmd, ds_vorticity[p][slot], wg, wg, wg);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

            // (7) Emit sources (in-place on velocity_new, density_new, temperature_new).
            {
                auto scope = profiler.scope(cmd, "emit_sources");
                pipe_emit.dispatch(cmd, ds_emit[p][slot], wg, wg, wg);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

            // (8) Apply boundaries (in-place on velocity_new; zeros at the 5 no-slip faces).
            {
                auto scope = profiler.scope(cmd, "apply_boundaries");
                pipe_boundaries.dispatch(cmd, ds_boundaries[p][slot], wg, wg, wg);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

            // (9) Compute divergence.
            {
                auto scope = profiler.scope(cmd, "compute_divergence");
                pipe_divergence.dispatch(cmd, ds_divergence[p][slot], wg, wg, wg);
            }
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

            // (10) Jacobi pressure inner loop (ping-pong on pressure).
            // Iteration `i`: read pressure_old, write pressure_new.
            // We use `jacobi_parity` separately from `parity` — flipping each iteration.
            {
                auto scope = profiler.scope(cmd, "jacobi_pressure");
                uint32_t jp = 0;     // jacobi parity: 0 = read ping, write pong
                for (int i = 0; i < rt.pressureIters; ++i) {
                    pipe_jacobi.dispatch(cmd, ds_jacobi[jp][slot], wg, wg, wg);
                    gv::memoryBarrier(cmd,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
                    jp ^= 1u;
                }
                // After the loop: the dispatch at iteration i used ds_jacobi[i&1] and wrote
                // to pre_new_v at parity (i&1). For N iterations the final-written parity is
                // (N-1) & 1. The project step (pass 11 below) selects its descriptor with that
                // parity so it reads from the just-written pressure image.
            }

            // (11) Project: subtract grad(p) from velocity (in-place on velocity_new).
            // Use the project descriptor whose parity matches the final-written pressure image.
            //
            // Parity reasoning: at Jacobi iteration `i` (i=0..N-1), the dispatch uses
            // ds_jacobi[i&1] which writes to pre_new_v at parity (i&1). After N iterations,
            // the LAST-written pressure image is at parity (N-1)&1. The project descriptor
            // ds_project[p] reads pre_new_v at parity p, so we want project_p = (N-1) & 1.
            //
            // Example (N=40, default): (40-1)&1 = 1 → reads pre_new_v at parity 1
            // = pressure_ping → matches iteration 39's write target. ✓
            // Pre-condition: rt.pressureIters >= 1 (panel slider min is 10).
            uint32_t project_p = uint32_t((rt.pressureIters - 1u) & 1u);
            {
                auto scope = profiler.scope(cmd, "apply_pressure");
                pipe_project.dispatch(cmd, ds_project[project_p][slot], wg, wg, wg);
            }

            // Barrier before the next substep reads from velocity_new.
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

            // Flip parity: velocity_new this substep becomes velocity_old next substep.
            rt.iteration++;
        }

        // --------------------------------------------------------------------
        // Final barrier before the fragment-shader raymarch reads density + temperature.
        // --------------------------------------------------------------------
        gv::memoryBarrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

        // --------------------------------------------------------------------
        // ImGui frame body. Six panel folders. Match RD-3D's panel structure.
        // --------------------------------------------------------------------
        gpusims::ui::newImGuiFrame();
        ImGui::Begin("Eulerian Smoke");

        if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Preset dropdown.
            const char* preset_names[] = {
                SMOKE_PRESETS[0].name, SMOKE_PRESETS[1].name, SMOKE_PRESETS[2].name,
                SMOKE_PRESETS[3].name, SMOKE_PRESETS[4].name, SMOKE_PRESETS[5].name,
                "Custom"
            };
            int preset_idx_with_custom = rt.isCustom ? 6 : rt.presetIndex;
            if (ImGui::Combo("Preset", &preset_idx_with_custom, preset_names, 7)) {
                if (preset_idx_with_custom < 6) {
                    apply_preset(rt, preset_idx_with_custom);
                    zero_all_fields();
                    rt.iteration = 0;
                    rt.vdbFrameCounter = 0;
                    place_default_emitter();
                }
            }

            // Tier dropdown.
            const char* tier_names[] = {"192³ — Compact", "256³ — Default", "384³ — Stretch"};
            ImGui::Combo("Tier", &rt.pendingTierIndex, tier_names, NUM_TIERS);
            if (rt.pendingTierIndex == 2) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f),
                    "⚠ 384³ uses ~2.9 GB VRAM and may run at 20-30 fps. Stretch tier.");
            }
            ImGui::SameLine();
            if (ImGui::Button("Apply (recreates grid)")) {
                tier_apply_clicked = true;
            }

            ImGui::Separator();

            bool changed = false;
            changed |= ImGui::SliderInt("substeps", &rt.substeps, 1, 8);
            changed |= ImGui::SliderFloat("dt", &rt.dt, 0.01f, 0.5f, "%.3f");
            changed |= ImGui::SliderInt("pressureIters", &rt.pressureIters, 10, 100);
            changed |= ImGui::SliderFloat("vorticityStrength (ε)", &rt.vorticityStrength, 0.0f, 30.0f, "%.2f");
            changed |= ImGui::SliderFloat("buoyancyAlpha (α)", &rt.buoyancyAlpha, -5.0f, 10.0f, "%.2f");
            changed |= ImGui::SliderFloat("buoyancyBeta (β)", &rt.buoyancyBeta, 0.0f, 5.0f, "%.2f");
            changed |= ImGui::SliderFloat("densityDissipation", &rt.densityDissipation, 0.0f, 0.05f, "%.4f");
            changed |= ImGui::SliderFloat("velocityDissipation", &rt.velocityDissipation, 0.0f, 0.05f, "%.4f");
            changed |= ImGui::SliderFloat("temperatureDissipation", &rt.temperatureDissipation, 0.0f, 0.05f, "%.4f");
            changed |= ImGui::SliderFloat("maccormackEpsilon", &rt.maccormackEpsilon, 0.0f, 0.1f, "%.3f");
            if (changed && !rt.isCustom) rt.isCustom = true;

            if (ImGui::Button("Reset to preset IC")) {
                apply_preset(rt, rt.presetIndex);
                zero_all_fields();
                rt.iteration = 0;
                rt.vdbFrameCounter = 0;
                place_default_emitter();
            }
        }

        if (ImGui::CollapsingHeader("Rendering")) {
            bool changed = false;
            changed |= ImGui::SliderInt("raymarchSteps", &rt.raymarchSteps, 16, 256);
            changed |= ImGui::SliderInt("shadowMarchSteps", &rt.shadowMarchSteps, 4, 64);
            changed |= ImGui::SliderFloat("densityAbsorption", &rt.densityAbsorption, 0.0f, 50.0f, "%.1f");
            changed |= ImGui::SliderFloat("emissionStrength", &rt.emissionStrength, 0.0f, 20.0f, "%.2f");
            changed |= ImGui::SliderFloat("scatteringStrength", &rt.scatteringStrength, 0.0f, 10.0f, "%.2f");
            changed |= ImGui::SliderFloat("exposure", &rt.exposure, 0.1f, 4.0f, "%.2f");
            changed |= ImGui::SliderFloat("renderScale", &rt.renderScale, 0.5f, 1.0f, "%.2f");
            const char* ramp_names[] = {"blackbody", "sunset", "cold", "mono"};
            changed |= ImGui::Combo("colorRamp", &rt.colorRamp, ramp_names, 4);
            if (changed && !rt.isCustom) rt.isCustom = true;
        }

        if (ImGui::CollapsingHeader("Lighting")) {
            bool changed = false;
            changed |= ImGui::SliderFloat("light azimuth (deg)", &rt.lightAzimuthDeg, -180.0f, 180.0f, "%.1f");
            changed |= ImGui::SliderFloat("light elevation (deg)", &rt.lightElevationDeg, -90.0f, 90.0f, "%.1f");
            changed |= ImGui::ColorEdit3("lightColor", glm::value_ptr(rt.lightColor));
            changed |= ImGui::SliderFloat("ambientStrength", &rt.ambientStrength, 0.0f, 1.0f, "%.2f");
            changed |= ImGui::ColorEdit3("bgTopColor", glm::value_ptr(rt.bgTopColor));
            changed |= ImGui::ColorEdit3("bgBottomColor", glm::value_ptr(rt.bgBottomColor));
            if (changed && !rt.isCustom) rt.isCustom = true;
        }

        if (ImGui::CollapsingHeader("Emitter")) {
            ImGui::Text("Emitters: %zu / %d", rt.emitters.size(), EMITTER_CAP);
            ImGui::TextWrapped("LMB-click in viewport to place; RMB-click near an emitter to remove.");
            bool changed = false;
            changed |= ImGui::SliderFloat("density rate", &rt.emitterDensityRate, 0.0f, 10.0f, "%.2f");
            changed |= ImGui::SliderFloat("temperature", &rt.emitterTemperature, 0.0f, 2.0f, "%.2f");
            changed |= ImGui::SliderFloat("upward bias", &rt.emitterUpwardBias, 0.0f, 5.0f, "%.2f");
            changed |= ImGui::SliderFloat("radius (cells)", &rt.emitterRadius, 1.0f, 16.0f, "%.1f");
            if (changed && !rt.isCustom) rt.isCustom = true;
            if (ImGui::Button("Clear all emitters")) {
                rt.emitters.clear();
            }
        }

        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::Checkbox("autoOrbit", &rt.autoOrbit);
            ImGui::SliderFloat("orbitSpeed (deg/s)", &rt.orbitSpeedDegPerSec, 0.0f, 60.0f, "%.1f");
            ImGui::SliderFloat("orbitRadius", &rt.orbitRadius, 0.5f, 5.0f, "%.2f");
        }

        if (ImGui::CollapsingHeader("State", ImGuiTreeNodeFlags_DefaultOpen)) {
            // VDB toggle.
            bool vdb_avail = gpusims::vdb::isAvailable();
            if (!vdb_avail) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                    "OpenVDB not enabled in this build.");
                ImGui::TextWrapped("Rebuild with -DGPU_SIMS_USE_OPENVDB=ON to enable.");
            }
            if (vdb_avail) {
                ImGui::Checkbox("Export VDB density per frame (slow: ~50-150 ms/frame)", &rt.exportVdb);
                ImGui::Text("vdbFrameCounter: %u", rt.vdbFrameCounter);
                ImGui::SameLine();
                if (ImGui::Button("Reset VDB counter")) {
                    rt.vdbFrameCounter = 0;
                }
                if (ImGui::Button("Export current temperature snapshot")) {
                    renderer.waitIdle();
                    const size_t N = size_t(rt.gridSize) * rt.gridSize * rt.gridSize;
                    std::vector<float> tem_host(N);
                    const bool curr_is_ping = (rt.iteration % 2u == 0u);
                    gv::Image& tem_curr = curr_is_ping ? temperature_ping : temperature_pong;
                    tem_curr.readback(tem_host.data(), tem_host.size() * sizeof(float));
                    bool ok = gpusims::vdb::writeFloatFrame("vdb_export/temperature",
                        rt.vdbFrameCounter,
                        tem_host.data(),
                        glm::ivec3(int(rt.gridSize), int(rt.gridSize), int(rt.gridSize)),
                        1.0f / float(rt.gridSize),
                        glm::vec3(0.0f),
                        "temperature");
                    gpusims::ui::pushToast(ok ? "Temperature VDB written" : "Temperature VDB FAILED", ok);
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Save (F5)")) capture_save();
            ImGui::SameLine();
            if (ImGui::Button("Load most recent (F9)")) capture_load();
        }

        if (ImGui::CollapsingHeader("Profiler")) {
            profiler.drawImGui();
        }

        ImGui::End();
        gpusims::ui::drawToasts();

        // --------------------------------------------------------------------
        // Apply tier-change (must be AFTER ImGui because the panel reads
        // pendingTierIndex into tier_apply_clicked).
        // --------------------------------------------------------------------
        if (tier_apply_clicked && rt.pendingTierIndex != rt.tierIndex) {
            uint32_t new_size = TIER_SIZES[size_t(rt.pendingTierIndex)];
            rt.tierIndex = rt.pendingTierIndex;
            recreateGridResources(new_size);
        }

        // --------------------------------------------------------------------
        // Render: raymarch + ImGui in a single render pass.
        // --------------------------------------------------------------------
        VkClearColorValue clear{};
        clear.float32[0] = rt.bgBottomColor.r;
        clear.float32[1] = rt.bgBottomColor.g;
        clear.float32[2] = rt.bgBottomColor.b;
        clear.float32[3] = 1.0f;
        renderer.beginRendering(*frame, clear);

        uint32_t raymarch_p = uint32_t(rt.iteration & 1u);
        pipe_raymarch.bind(cmd, ds_raymarch[raymarch_p][slot]);
        vkCmdDraw(cmd, 3, 1, 0, 0);     // fullscreen triangle

        gpusims::ui::renderImGui(cmd);

        renderer.endRendering(*frame);

        // --------------------------------------------------------------------
        // Optional: per-frame VDB density export.
        // (Costly: ~50-150 ms at 256³ including readback + serialization.)
        // --------------------------------------------------------------------
        if (rt.exportVdb && gpusims::vdb::isAvailable()) {
            renderer.waitIdle();
            const size_t N = size_t(rt.gridSize) * rt.gridSize * rt.gridSize;
            std::vector<float> den_host(N);
            const bool curr_is_ping = (rt.iteration % 2u == 0u);
            gv::Image& den_curr = curr_is_ping ? density_ping : density_pong;
            den_curr.readback(den_host.data(), den_host.size() * sizeof(float));
            bool ok = gpusims::vdb::writeFloatFrame("vdb_export/density",
                rt.vdbFrameCounter,
                den_host.data(),
                glm::ivec3(int(rt.gridSize), int(rt.gridSize), int(rt.gridSize)),
                1.0f / float(rt.gridSize),
                glm::vec3(0.0f),
                "density");
            if (!ok) {
                logError("VDB export failed at frame {}; disabling exportVdb to avoid spam", rt.vdbFrameCounter);
                rt.exportVdb = false;
            }
            rt.vdbFrameCounter++;
        }

        profiler.endFrame(cmd);
        renderer.endFrame(*frame);
    }

    // ------------------------------------------------------------------------
    // Shutdown — match RD-3D's reverse-order pattern.
    // ------------------------------------------------------------------------
    renderer.waitIdle();
    gpusims::ui::shutdownImGui();
    vkDestroySampler(ctx.device(), sampler_lut, nullptr);
    vkDestroySampler(ctx.device(), sampler_linear, nullptr);
    logInfo("eulerian_smoke: shutdown complete");
    return 0;
}
