// SPH Water — first Tier-2 particle-fluids flagship Stack C sim.
//
// DFSPH (Bender-Koschier 2015 + 2017) at 1M-4M particles. Two inner Jacobi-style
// solver loops per substep: divergence-free correction + density-constancy
// correction. Morton-sorted spatial hash for neighbor search. Screen-space
// fluid surface with refraction (Müller-Fetterer 2007). Four scene presets
// (Dam-Break, Central-Fountain, Droplet-Impact, Pour-from-Source). LMB-paint
// emitter brush via reserve-tail allocation. F5/F9 state capture-and-load.
// First Alembic real-impl consumer in the repo.
//
// ============================================================================
// PHASE 11 SCAFFOLD STATUS
// ============================================================================
//
// This file is a structural scaffold per Phase 11 spec § 4.B. Substantive
// content materialized at scaffold time:
//   - All headers + namespace aliases (§ 4.B.0)
//   - Constants block (§ 4.B.1)
//   - Scene preset table (§ 4.B.2)
//   - Emitter + Runtime structs (§ 4.B.3)
//   - Uniform struct host-mirrors (§ 4.B.4)
//   - Click-to-place paint-plane unproject helper (§ 4.B.6)
//   - main()'s init flow: Context + Window + Renderer + Camera + ImGui +
//     StateWriter + Alembic writer (§ 4.B.8; adapted to the synced common-cpp
//     API — see completion report for the assumed-vs-actual API drift list)
//   - Top-level frame loop with camera update + ImGui panel skeleton
//
// DEFERRED to Phase 11 follow-up (surfaced in the completion report):
//   - Descriptor-write helpers (§ 4.B.7; 17 helpers depending on ES line
//     ranges that drift between spec and synced source).
//   - Buffer + image creation per-tier (§ 4.B.9).
//   - Pipeline creation + hot-reload watch (§ 4.B.10; depends on common-cpp
//     ComputePipelineDesc subgroup-size-control surface additions per hard
//     rule 5 — NOT YET ADDED).
//   - Per-substep DFSPH dispatch chain (§ 4.B.11).
//   - Screen-space fluid render passes via direct vkCmdBeginRenderingKHR
//     (§ 4.B.12).
//   - ImGui panel construction (§ 4.B.13; by-reference to ES with a delta
//     table; ES anchors at the drafted-vs-synced range need re-anchoring).
//   - Alembic export call site (§ 4.B.14).
//   - F5/F9 capture/load (§ 4.B.15).
//
// The shaders in particle-fluids/sph-water/shaders/ are FULL implementations
// per spec § 4.C - 4.H. They're ready to consume once the host-side dispatch
// chain is wired.

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <gpusims/alembic_writer.hpp>
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

namespace fs  = std::filesystem;
namespace gv  = gpusims::vk;
namespace abc = gpusims::abc;
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
// Constants (§ 4.B.1)
// ============================================================================

constexpr int NUM_TIERS = 4;
constexpr std::array<uint32_t, NUM_TIERS> TIER_PARTICLE_COUNTS = {
    256u * 1024u,         // 256k
    1024u * 1024u,        // 1M (default)
    2048u * 1024u,        // 2M
    4096u * 1024u,        // 4M (capture-mode)
};
constexpr int DEFAULT_TIER_INDEX = 1;
constexpr float EMITTER_RESERVE_FRAC = 0.20f;

// DFSPH defaults — SPlisHSPlasH 1.8.10 at TimeStepDFSPH.cpp:35-41.
constexpr int   DFSPH_MIN_ITER_DENSITY   = 2;
constexpr int   DFSPH_MAX_ITER_DENSITY   = 100;
constexpr float DFSPH_MAX_ERROR_DENSITY  = 0.01f;     // PERCENT - 0.01 = 0.01% of rho_0
constexpr int   DFSPH_MAX_ITER_DIV       = 100;
constexpr float DFSPH_MAX_ERROR_DIV      = 0.1f;       // PERCENT - 0.1 = 0.1% of rho_0
constexpr bool  DFSPH_DIV_SOLVER_DEFAULT = true;
constexpr float DFSPH_ALPHA_EPS          = 1.0e-5f;
constexpr float DFSPH_JACOBI_RELAX       = 0.5f;       // SPlisHSPlasH TimeStepDFSPH.cpp:606,:692

constexpr float CFL_FACTOR    = 0.5f;
constexpr float DT_MIN        = 1.0e-4f;
constexpr float DT_MAX        = 5.0e-3f;
constexpr float DENSITY_0     = 1000.0f;
constexpr float GRAVITY_Y     = -9.81f;
constexpr float PARTICLE_RADIUS_DEFAULT     = 0.01f;
constexpr float SUPPORT_RADIUS_RATIO         = 4.0f;
constexpr float CELL_SIZE_RATIO_TO_SUPPORT   = 2.0f;

constexpr uint32_t WG_DIM_SORT      = 256;
constexpr uint32_t WG_DIM_DFSPH     = 256;
constexpr uint32_t WG_DIM_BILATERAL = 16;

constexpr int   BILATERAL_ITERATIONS_DEFAULT       = 4;
constexpr float BILATERAL_SIGMA_SPATIAL_DEFAULT_PX = 3.0f;
constexpr float BILATERAL_SIGMA_DEPTH_DEFAULT_NDC  = 0.05f;
constexpr float THICKNESS_PER_PARTICLE_DEFAULT    = 0.005f;

constexpr float DOMAIN_HALF_EXTENT_DEFAULT = 2.0f;
constexpr int   SUBSTEPS_DEFAULT           = 1;
constexpr int   ALEMBIC_EVERY_N_FRAMES_DEFAULT = 2;
constexpr int   EMITTER_CAP                = 8;
constexpr uint32_t MORTON_BITS_PER_AXIS    = 10;
constexpr uint32_t MAX_CELLS_PER_AXIS      = 1u << MORTON_BITS_PER_AXIS;

constexpr float FOV_DEG_DEFAULT = 50.0f;
constexpr float NEAR_PLANE      = 0.05f;
constexpr float FAR_PLANE       = 100.0f;

// ============================================================================
// Scene presets (§ 4.B.2)
// ============================================================================

enum class EmitterShape : int { None = 0, Cylinder = 1, Rectangle = 2 };

struct SphPreset {
    const char*  name;
    glm::vec3    initial_brick_min;
    glm::vec3    initial_brick_max;
    bool         add_droplet;
    glm::vec3    droplet_center;
    float        droplet_radius;
    glm::vec3    droplet_velocity_init;
    EmitterShape source_shape;
    glm::vec3    source_pos;
    glm::vec3    source_size;
    glm::vec3    source_velocity;
    float        source_emission_rate;
    glm::vec3    domain_min;
    glm::vec3    domain_max;
    float        viscosity;
    float        cohesion;
    float        vorticity_strength;
    float        roughness;
};

constexpr std::array<SphPreset, 4> SPH_PRESETS = {{
    {
        "Dam-Break",
        glm::vec3(+0.5f, -1.0f, -1.0f), glm::vec3(+2.0f,  0.5f, +1.0f),
        false, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f),
        EmitterShape::None, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), 0.0f,
        glm::vec3(-2.0f, -1.0f, -1.0f), glm::vec3(+2.0f, +2.0f, +1.0f),
        0.005f, 0.0f, 0.5f, 0.0f,
    },
    {
        "Central-Fountain",
        glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(+1.0f, -0.95f, +1.0f),
        false, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f),
        EmitterShape::Cylinder, glm::vec3(0.0f, -0.9f, 0.0f),
        glm::vec3(0.1f, 0.05f, 0.0f), glm::vec3(0.0f, +5.0f, 0.0f), 5000.0f,
        glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(+1.0f, +2.0f, +1.0f),
        0.01f, 0.05f, 0.8f, 0.0f,
    },
    {
        "Droplet-Impact",
        glm::vec3(-1.5f, -1.0f, -1.5f), glm::vec3(+1.5f, -0.97f, +1.5f),
        true,  glm::vec3(0.0f, +1.0f, 0.0f), 0.15f, glm::vec3(0.0f, -5.0f, 0.0f),
        EmitterShape::None, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), 0.0f,
        glm::vec3(-1.5f, -1.0f, -1.5f), glm::vec3(+1.5f, +2.0f, +1.5f),
        0.02f, 0.1f, 1.2f, 0.02f,
    },
    {
        "Pour-from-Source",
        glm::vec3(0.0f), glm::vec3(0.0f),
        false, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f),
        EmitterShape::Rectangle, glm::vec3(0.0f, +1.95f, 0.0f),
        glm::vec3(0.3f, 0.0f, 0.3f), glm::vec3(0.0f, -2.0f, 0.0f), 8000.0f,
        glm::vec3(-1.5f, -1.0f, -1.5f), glm::vec3(+1.5f, +2.0f, +1.5f),
        0.01f, 0.05f, 0.6f, 0.0f,
    },
}};

// ============================================================================
// Emitter + Runtime state (§ 4.B.3)
// ============================================================================

struct Emitter {
    glm::vec3 pos;
    float     radius;
    glm::vec3 velocity;
    float     emissionRate;
    float     ageSec = 0.0f;
};

struct Runtime {
    int       presetIndex         = 0;
    bool      isCustom            = false;
    int       tierIndex           = DEFAULT_TIER_INDEX;
    uint32_t  particleCapacity    = TIER_PARTICLE_COUNTS[DEFAULT_TIER_INDEX];
    uint32_t  particleCount       = 0;
    uint32_t  presetParticleCount = 0;
    int       pendingTierIndex    = DEFAULT_TIER_INDEX;

    int       substeps               = SUBSTEPS_DEFAULT;
    float     dt                     = DT_MAX;
    int       maxIterDensity         = DFSPH_MAX_ITER_DENSITY;
    int       maxIterDivergence      = DFSPH_MAX_ITER_DIV;
    int       minIterDensity         = DFSPH_MIN_ITER_DENSITY;
    int       minIterDivergence      = 1;
    float     maxErrorDensityPercent = DFSPH_MAX_ERROR_DENSITY;
    float     maxErrorDivPercent     = DFSPH_MAX_ERROR_DIV;
    bool      divSolverEnabled       = DFSPH_DIV_SOLVER_DEFAULT;
    float     viscosity              = 0.01f;
    float     cohesion               = 0.0f;
    float     vorticityStrength      = 0.5f;

    float     particleRadius     = PARTICLE_RADIUS_DEFAULT;
    float     supportRadius      = PARTICLE_RADIUS_DEFAULT * SUPPORT_RADIUS_RATIO;
    float     cellSize           = PARTICLE_RADIUS_DEFAULT * SUPPORT_RADIUS_RATIO * CELL_SIZE_RATIO_TO_SUPPORT;
    glm::vec3 domainMin          = glm::vec3(-DOMAIN_HALF_EXTENT_DEFAULT);
    glm::vec3 domainMax          = glm::vec3(+DOMAIN_HALF_EXTENT_DEFAULT);

    int       bilateralIterations    = BILATERAL_ITERATIONS_DEFAULT;
    float     bilateralSigmaSpatial  = BILATERAL_SIGMA_SPATIAL_DEFAULT_PX;
    float     bilateralSigmaDepth    = BILATERAL_SIGMA_DEPTH_DEFAULT_NDC;
    float     thicknessPerParticle   = THICKNESS_PER_PARTICLE_DEFAULT;
    float     surfaceRoughness       = 0.0f;
    glm::vec3 waterTint              = glm::vec3(0.10f, 0.30f, 0.50f);
    glm::vec3 skyZenith              = glm::vec3(0.40f, 0.55f, 0.85f);
    glm::vec3 skyHorizon             = glm::vec3(0.85f, 0.90f, 0.95f);
    float     exposure               = 1.0f;

    std::vector<Emitter> emitters;
    bool      lmbHeld          = false;
    glm::vec3 paintPlanePoint  = glm::vec3(0.0f);

    bool      exportAlembic       = false;
    int       alembicEveryNFrames = ALEMBIC_EVERY_N_FRAMES_DEFAULT;
    uint32_t  alembicFrameCounter = 0;
    uint64_t  iteration           = 0;
    bool      paused              = false;
};

static void apply_preset(Runtime& rt, int idx) {
    const SphPreset& p = SPH_PRESETS[size_t(idx)];
    rt.presetIndex       = idx;
    rt.isCustom          = false;
    rt.viscosity         = p.viscosity;
    rt.cohesion          = p.cohesion;
    rt.vorticityStrength = p.vorticity_strength;
    rt.surfaceRoughness  = p.roughness;
    rt.domainMin         = p.domain_min;
    rt.domainMax         = p.domain_max;
    rt.particleCount     = 0;
    rt.presetParticleCount = 0;
    rt.emitters.clear();
}

// ============================================================================
// Click-to-place paint-plane unproject (§ 4.B.6)
// ============================================================================

static bool unprojectToPaintPlane(const Camera& cam,
                                  GLFWwindow* window,
                                  const glm::vec3& planeAnchor,
                                  glm::vec3& out_world) {
    double cx, cy; glfwGetCursorPos(window, &cx, &cy);
    int w, h;      glfwGetFramebufferSize(window, &w, &h);
    if (w <= 0 || h <= 0) return false;

    float ndc_x = (2.0f * float(cx) / float(w)) - 1.0f;
    float ndc_y = 1.0f - (2.0f * float(cy) / float(h));

    glm::mat4 invViewProj = glm::inverse(cam.projection() * cam.view());

    glm::vec4 near_clip(ndc_x, ndc_y, 0.0f, 1.0f);
    glm::vec4 far_clip (ndc_x, ndc_y, 1.0f, 1.0f);
    glm::vec4 near_world_h = invViewProj * near_clip;
    glm::vec4 far_world_h  = invViewProj * far_clip;
    glm::vec3 near_world = glm::vec3(near_world_h) / near_world_h.w;
    glm::vec3 far_world  = glm::vec3(far_world_h)  / far_world_h.w;

    glm::vec3 ray_origin = near_world;
    glm::vec3 ray_dir    = glm::normalize(far_world - near_world);

    glm::vec3 plane_normal = glm::normalize(planeAnchor - cam.position());
    float denom = glm::dot(ray_dir, plane_normal);
    if (std::abs(denom) < 1e-6f) return false;
    float t = glm::dot(planeAnchor - ray_origin, plane_normal) / denom;
    if (t < 0.0f) return false;

    out_world = ray_origin + t * ray_dir;
    return true;
}

// ============================================================================
// Descriptor-write helpers (§ 4.B per phase11_main_cpp_wiring.md)
// ----------------------------------------------------------------------------
// One helper per pipeline that needs a descriptor set. Pattern transferred
// from ES (eulerian-smoke/src/main.cpp:542-580 + 617-657 + 893-943): build
// VkDescriptorBufferInfo / VkDescriptorImageInfo arrays per binding, build
// matching VkWriteDescriptorSet array, vkUpdateDescriptorSets in one call.
// Bindings match the per-shader `layout(set=0, binding=N)` declarations
// in particle-fluids/sph-water/shaders/.
// ============================================================================

static void writeApplyEmitterDescriptor(VkDevice device,
                                        VkDescriptorSet ds,
                                        VkBuffer particles,
                                        VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo p_i{};  p_i.buffer = particles;      p_i.range = VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};  u_i.buffer = uniform_buffer; u_i.range = VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 2> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&p_i;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[1].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeInitialFillDescriptor(VkDevice device,
                                       VkDescriptorSet ds,
                                       VkBuffer particles,
                                       VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo p_i{};  p_i.buffer = particles;      p_i.range = VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};  u_i.buffer = uniform_buffer; u_i.range = VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 2> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&p_i;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[1].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeMortonCodeDescriptor(VkDevice device,
                                      VkDescriptorSet ds,
                                      VkBuffer particles,
                                      VkBuffer morton_codes,
                                      VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=particles;      b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=morton_codes;   b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer;u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[2].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeCellCountDescriptor(VkDevice device,
                                     VkDescriptorSet ds,
                                     VkBuffer morton_codes,
                                     VkBuffer cell_counts,
                                     VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=morton_codes;   b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=cell_counts;    b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer;u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[2].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writePrefixSumLocalDescriptor(VkDevice device,
                                          VkDescriptorSet ds,
                                          VkBuffer in_counts,
                                          VkBuffer out_per_block,
                                          VkBuffer out_block_sums,
                                          VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=in_counts;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=out_per_block;   b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=out_block_sums;  b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[3].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writePrefixSumBlockDescriptor(VkDevice device,
                                          VkDescriptorSet ds,
                                          VkBuffer in_out_sums,
                                          VkBuffer out_prefixes,
                                          VkBuffer out_l2_sums,
                                          VkBuffer in_l2_prefixes,
                                          VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=in_out_sums;     b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=out_prefixes;    b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=out_l2_sums;     b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=in_l2_prefixes;  b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writePrefixSumBlockL2Descriptor(VkDevice device,
                                            VkDescriptorSet ds,
                                            VkBuffer in_l2_sums,
                                            VkBuffer out_l2_prefixes,
                                            VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=in_l2_sums;      b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=out_l2_prefixes; b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 3> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[2].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writePrefixSumAddbackDescriptor(VkDevice device,
                                            VkDescriptorSet ds,
                                            VkBuffer per_block,
                                            VkBuffer block_prefixes,
                                            VkBuffer cell_starts,
                                            VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=per_block;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=block_prefixes;  b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=cell_starts;     b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[3].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeScatterDescriptor(VkDevice device,
                                   VkDescriptorSet ds,
                                   VkBuffer morton_codes,
                                   VkBuffer cell_starts,
                                   VkBuffer sorted_index,
                                   VkBuffer cell_counts_atomic,
                                   VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=morton_codes;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=cell_starts;        b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=sorted_index;       b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=cell_counts_atomic; b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer;    u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeDensityAlphaDescriptor(VkDevice device,
                                        VkDescriptorSet ds,
                                        VkBuffer particles,
                                        VkBuffer cell_starts,
                                        VkBuffer sorted_index,
                                        VkBuffer density_alpha,
                                        VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=particles;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=cell_starts;     b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=sorted_index;    b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=density_alpha;   b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

// Shared by divergence_solve + density_solve (identical binding layout).
// Caller supplies the inner-iter ping-pong by flipping pressure_read/_write.
static void writeDfsphSolveDescriptor(VkDevice device,
                                      VkDescriptorSet ds,
                                      VkBuffer particles,
                                      VkBuffer density_alpha,
                                      VkBuffer cell_starts,
                                      VkBuffer sorted_index,
                                      VkBuffer pressure_read,
                                      VkBuffer pressure_write,
                                      VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=particles;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=density_alpha;   b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=cell_starts;     b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=sorted_index;    b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b4{}; b4.buffer=pressure_read;   b4.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b5{}; b5.buffer=pressure_write;  b5.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 7> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[4].pBufferInfo=&b4;
    w[5].dstSet=ds; w[5].dstBinding=5; w[5].descriptorCount=1;
    w[5].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[5].pBufferInfo=&b5;
    w[6].dstSet=ds; w[6].dstBinding=6; w[6].descriptorCount=1;
    w[6].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[6].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeIntegrateForcesDescriptor(VkDevice device,
                                           VkDescriptorSet ds,
                                           VkBuffer particles,
                                           VkBuffer density_alpha,
                                           VkBuffer cell_starts,
                                           VkBuffer sorted_index,
                                           VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=particles;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=density_alpha;   b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=cell_starts;     b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=sorted_index;    b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writePressureApplyDescriptor(VkDevice device,
                                         VkDescriptorSet ds,
                                         VkBuffer particles,
                                         VkBuffer density_alpha,
                                         VkBuffer pressure_read,
                                         VkBuffer cell_starts,
                                         VkBuffer sorted_index,
                                         VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=particles;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=density_alpha;   b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=pressure_read;   b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=cell_starts;     b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b4{}; b4.buffer=sorted_index;    b4.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 6> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[4].pBufferInfo=&b4;
    w[5].dstSet=ds; w[5].dstBinding=5; w[5].descriptorCount=1;
    w[5].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[5].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

// Shared by particle_sprite (depth pass) vert+frag pair.
static void writeParticleSpriteDescriptor(VkDevice device,
                                          VkDescriptorSet ds,
                                          VkBuffer particles,
                                          VkBuffer render_view_uniform) {
    VkDescriptorBufferInfo p_i{}; p_i.buffer=particles;            p_i.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{}; u_i.buffer=render_view_uniform;  u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 2> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&p_i;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[1].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeThicknessDescriptor(VkDevice device,
                                     VkDescriptorSet ds,
                                     VkBuffer particles,
                                     VkBuffer render_view_uniform) {
    // Identical binding shape to particle_sprite; defined separately so each
    // graphics pipeline's descriptor-set layout is fed from its own write
    // function and pipelines can diverge without touching unrelated callers.
    writeParticleSpriteDescriptor(device, ds, particles, render_view_uniform);
}

static void writeBilateralSmoothDescriptor(VkDevice device,
                                           VkDescriptorSet ds,
                                           VkImageView input_depth_view,
                                           VkImageView output_depth_view,
                                           VkSampler   sampler_linear,
                                           VkBuffer    uniform_buffer) {
    VkDescriptorImageInfo in_i{};
    in_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    in_i.imageView   = input_depth_view;
    in_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorImageInfo out_i{};
    out_i.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    out_i.imageView   = output_depth_view;
    out_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorImageInfo samp_i{};
    samp_i.sampler     = sampler_linear;
    samp_i.imageView   = VK_NULL_HANDLE;
    samp_i.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkDescriptorBufferInfo u_i{}; u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[0].pImageInfo=&in_i;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; w[1].pImageInfo=&out_i;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLER;       w[2].pImageInfo=&samp_i;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;w[3].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

static void writeCompositeDescriptor(VkDevice device,
                                     VkDescriptorSet ds,
                                     VkImageView smoothed_depth_view,
                                     VkImageView thickness_view,
                                     VkSampler   sampler_linear,
                                     VkBuffer    uniform_buffer) {
    VkDescriptorImageInfo d_i{};
    d_i.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    d_i.imageView   = smoothed_depth_view;
    d_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorImageInfo t_i{};
    t_i.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    t_i.imageView   = thickness_view;
    t_i.sampler     = VK_NULL_HANDLE;

    VkDescriptorImageInfo samp_i{};
    samp_i.sampler     = sampler_linear;
    samp_i.imageView   = VK_NULL_HANDLE;
    samp_i.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkDescriptorBufferInfo u_i{}; u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 4> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[0].pImageInfo=&d_i;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[1].pImageInfo=&t_i;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_SAMPLER;       w[2].pImageInfo=&samp_i;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;w[3].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}

// ============================================================================
// GLFW input snapshot + mouse-edge detector (mirrors ES § 4.B.5)
// ============================================================================

struct InputState {
    CameraInputState snapshot(GLFWwindow* w) const {
        CameraInputState s;
        s.key_w = glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
        s.key_a = glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
        s.key_s = glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
        s.key_d = glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
        s.key_q = glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS;
        s.key_e = glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS;
        s.shift_held  = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
                      || glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        s.mouse_left   = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT)   == GLFW_PRESS;
        s.mouse_right  = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT)  == GLFW_PRESS;
        s.mouse_middle = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

        double cx, cy;
        glfwGetCursorPos(w, &cx, &cy);
        s.mouse_dx = float(cx) - last_cx_;
        s.mouse_dy = float(cy) - last_cy_;
        last_cx_ = float(cx);
        last_cy_ = float(cy);
        s.scroll_dy = scroll_dy_acc_;
        scroll_dy_acc_ = 0.0f;
        return s;
    }
    mutable float last_cx_       = 0.0f;
    mutable float last_cy_       = 0.0f;
    mutable float scroll_dy_acc_ = 0.0f;
};
static InputState g_input;
static void scrollCallback(GLFWwindow*, double, double dy) {
    g_input.scroll_dy_acc_ += float(dy);
}

struct MouseEdge {
    bool lmb_pressed   = false;
    bool lmb_released  = false;
    bool rmb_pressed   = false;
    bool rmb_released  = false;
    bool lmb_was_down  = false;
    bool rmb_was_down  = false;
    void poll(GLFWwindow* w) {
        bool lmb = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT)  == GLFW_PRESS;
        bool rmb = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        lmb_pressed  = lmb && !lmb_was_down;
        lmb_released = !lmb && lmb_was_down;
        rmb_pressed  = rmb && !rmb_was_down;
        rmb_released = !rmb && rmb_was_down;
        lmb_was_down = lmb;
        rmb_was_down = rmb;
    }
};
static MouseEdge g_mouse_edge;

// ============================================================================
// Per-tier resources (§ 4.C)
// ----------------------------------------------------------------------------
// All SoA buffers + off-screen images that must be reallocated when the tier
// dropdown changes. Buffers are sized by `particle_count`; the spatial-hash
// buffers + UBOs + off-screen images are sized off `max_cells` + swapchain
// extent. `Apply` in the tier dropdown swaps a fresh TierResources in
// (Phase 11 spec § 4.B.13).
// ============================================================================

struct TierResources {
    gv::Buffer particles;            // N x 128 bytes (packed struct)
    gv::Buffer density_alpha;        // N x 16 bytes
    gv::Buffer pressure_a;           // N x 4 bytes (Jacobi ping)
    gv::Buffer pressure_b;           // N x 4 bytes (Jacobi pong)
    gv::Buffer morton_codes;         // N x 4 bytes
    gv::Buffer sorted_index;         // N x 4 bytes
    gv::Buffer cell_counts;          // MAX_CELLS x 4 bytes (atomic counters)
    gv::Buffer cell_counts_atomic;   // MAX_CELLS x 4 bytes (scatter slot-claim)
    gv::Buffer cell_starts;          // MAX_CELLS x 4 bytes (prefix-sum output)
    gv::Buffer cell_block_sums;      // ceil(MAX_CELLS / WG_DIM_SORT) x 4 bytes
    gv::Buffer cell_block_prefixes;  // same size as cell_block_sums
    gv::Buffer cell_l2_sums;         // ceil(num_blocks / WG_DIM_SORT) x 4 bytes
    gv::Buffer cell_l2_prefixes;     // same size as cell_l2_sums
    gv::Buffer uniform_apply_emitter;
    gv::Buffer uniform_initial_fill;
    gv::Buffer uniform_sort;         // shared by morton_code, cell_count, prefix_sum_*, scatter
    gv::Buffer uniform_dfsph;        // shared by density_alpha, *_solve, integrate_forces, pressure_apply
    gv::Buffer uniform_render_view;  // shared by particle_sprite + thickness
    gv::Buffer uniform_bilateral;
    gv::Buffer uniform_composite;

    gv::Image  depth_image;
    gv::Image  smoothed_depth_a;
    gv::Image  smoothed_depth_b;
    gv::Image  thickness_image;
    VkSampler  sampler_linear = VK_NULL_HANDLE;

    std::uint32_t active_particle_count = 0;
};

static TierResources createTierResources(gv::Context& ctx,
                                         gv::Window&  window,
                                         std::uint32_t particle_count,
                                         std::uint32_t max_cells) {
    TierResources r{};
    r.active_particle_count = particle_count;

    constexpr VkBufferUsageFlags kSsboUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
      | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
      | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    constexpr VkBufferUsageFlags kUboUsage =
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
      | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    r.particles     = gv::Buffer::create(ctx, std::size_t(particle_count) * 128,
                                         kSsboUsage, gv::MemoryUsage::DeviceLocal, "particles");
    r.density_alpha = gv::Buffer::create(ctx, std::size_t(particle_count) * 16,
                                         kSsboUsage, gv::MemoryUsage::DeviceLocal, "density_alpha");
    r.pressure_a    = gv::Buffer::create(ctx, std::size_t(particle_count) * 4,
                                         kSsboUsage, gv::MemoryUsage::DeviceLocal, "pressure_a");
    r.pressure_b    = gv::Buffer::create(ctx, std::size_t(particle_count) * 4,
                                         kSsboUsage, gv::MemoryUsage::DeviceLocal, "pressure_b");
    r.morton_codes  = gv::Buffer::create(ctx, std::size_t(particle_count) * 4,
                                         kSsboUsage, gv::MemoryUsage::DeviceLocal, "morton_codes");
    r.sorted_index  = gv::Buffer::create(ctx, std::size_t(particle_count) * 4,
                                         kSsboUsage, gv::MemoryUsage::DeviceLocal, "sorted_index");

    const std::uint32_t num_blocks    = (max_cells   + WG_DIM_SORT - 1) / WG_DIM_SORT;
    const std::uint32_t num_l2_blocks = (num_blocks  + WG_DIM_SORT - 1) / WG_DIM_SORT;
    r.cell_counts         = gv::Buffer::create(ctx, std::size_t(max_cells) * 4,
                                               kSsboUsage, gv::MemoryUsage::DeviceLocal, "cell_counts");
    r.cell_counts_atomic  = gv::Buffer::create(ctx, std::size_t(max_cells) * 4,
                                               kSsboUsage, gv::MemoryUsage::DeviceLocal, "cell_counts_atomic");
    r.cell_starts         = gv::Buffer::create(ctx, std::size_t(max_cells) * 4,
                                               kSsboUsage, gv::MemoryUsage::DeviceLocal, "cell_starts");
    r.cell_block_sums     = gv::Buffer::create(ctx, std::size_t(num_blocks) * 4,
                                               kSsboUsage, gv::MemoryUsage::DeviceLocal, "cell_block_sums");
    r.cell_block_prefixes = gv::Buffer::create(ctx, std::size_t(num_blocks) * 4,
                                               kSsboUsage, gv::MemoryUsage::DeviceLocal, "cell_block_prefixes");
    r.cell_l2_sums        = gv::Buffer::create(ctx, std::size_t(std::max(num_l2_blocks, 1u)) * 4,
                                               kSsboUsage, gv::MemoryUsage::DeviceLocal, "cell_l2_sums");
    r.cell_l2_prefixes    = gv::Buffer::create(ctx, std::size_t(std::max(num_l2_blocks, 1u)) * 4,
                                               kSsboUsage, gv::MemoryUsage::DeviceLocal, "cell_l2_prefixes");

    // Uniform buffers — host-visible sequential for per-frame uploadDirect.
    // Sizes are generous upper bounds for each shader's std140 U block; std140
    // forces 16-byte alignment so 256 covers every kernel here comfortably.
    auto makeUbo = [&](std::size_t bytes, const char* name) {
        return gv::Buffer::create(ctx, bytes, kUboUsage,
                                  gv::MemoryUsage::HostVisibleSequential, name);
    };
    r.uniform_apply_emitter = makeUbo(2048, "uniform_apply_emitter"); // 32 B header + 8 emitters x 96 B
    r.uniform_initial_fill  = makeUbo( 256, "uniform_initial_fill");
    r.uniform_sort          = makeUbo( 256, "uniform_sort");
    r.uniform_dfsph         = makeUbo( 256, "uniform_dfsph");
    r.uniform_render_view   = makeUbo( 512, "uniform_render_view");
    r.uniform_bilateral     = makeUbo( 256, "uniform_bilateral");
    r.uniform_composite     = makeUbo( 256, "uniform_composite");

    const std::uint32_t W = window.extent().width;
    const std::uint32_t H = window.extent().height;
    gv::ImageCreateInfo idesc{};
    idesc.type   = gv::ImageType::e2D;
    idesc.extent = VkExtent3D{W, H, 1};
    idesc.format = VK_FORMAT_R32_SFLOAT;
    idesc.usage  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                 | VK_IMAGE_USAGE_SAMPLED_BIT
                 | VK_IMAGE_USAGE_STORAGE_BIT
                 | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    idesc.debug_name = "depth_image";
    r.depth_image      = gv::Image::create(ctx, idesc);
    idesc.debug_name = "smoothed_depth_a";
    r.smoothed_depth_a = gv::Image::create(ctx, idesc);
    idesc.debug_name = "smoothed_depth_b";
    r.smoothed_depth_b = gv::Image::create(ctx, idesc);
    idesc.debug_name = "thickness_image";
    r.thickness_image  = gv::Image::create(ctx, idesc);

    VkSamplerCreateInfo sci{};
    sci.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter     = VK_FILTER_LINEAR;
    sci.minFilter     = VK_FILTER_LINEAR;
    sci.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sci.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.minLod        = 0.0f;
    sci.maxLod        = 0.0f;
    if (vkCreateSampler(ctx.device(), &sci, nullptr, &r.sampler_linear) != VK_SUCCESS) {
        throw std::runtime_error("sph-water: vkCreateSampler failed for sampler_linear");
    }
    return r;
}

static void destroyTierResources(gv::Context& ctx, TierResources& r) {
    if (r.sampler_linear != VK_NULL_HANDLE) {
        vkDestroySampler(ctx.device(), r.sampler_linear, nullptr);
        r.sampler_linear = VK_NULL_HANDLE;
    }
}

// MAX_CELLS — total Morton cells; sized to comfortably hold the 4M-tier domain
// at the spec's default supportRadius / cell-size ratios. 64^3 keeps cell_*
// buffers under 1 MiB each.
constexpr std::uint32_t MAX_CELLS = 1u << 18;  // 262 144

// ============================================================================
// main()
// ============================================================================

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    initLogger();
    logInfo("sph-water - Phase 11 (DFSPH + Morton-sort + screen-space fluid + Alembic first-exercise)");

#ifdef GPU_SIMS_SPH_SHADER_DIR
    const std::string SD = GPU_SIMS_SPH_SHADER_DIR;
#else
    const std::string SD = "./shaders";
#endif
    (void)SD;

    // ----------------------------------------------------------------------
    // Vulkan context + window + renderer + shader compiler.
    //
    // Consumes the subgroup-size-control surface added in Turn 3 (commit
    // 9e0ca2f). Counting-sort + bilateral-smooth pipelines pin subgroup size
    // at creation; pinning requires the feature be enabled at device-create.
    // Pinned-kernel list: phase11_main_cpp_wiring.md § 2.3.
    // ----------------------------------------------------------------------
    gv::ContextCreateInfo cdesc{};
    cdesc.application_name             = "gpu-sims-sph-water";
    cdesc.enable_subgroup_size_control = true;
    gv::Context        ctx(cdesc);
    gv::Window         window(ctx, 1920, 1080, "GPU-Sims - SPH Water");
    gv::Renderer       renderer(ctx, window);
    gv::ShaderCompiler compiler(ctx);

    logInfo("[sph-water] Subgroup size: min={}, max={}, stages=0x{:x}",
            ctx.subgroupSizeMin(),
            ctx.subgroupSizeMax(),
            ctx.requiredSubgroupSizeStages());

    // Pinning value used by all counting-sort + bilateral pipelines. Min keeps
    // both AMD RDNA2 (32 on minSubgroupSize) and NVIDIA Turing+ (32) on the
    // same wavefront-size assumption rather than forcing AMD into wave64.
    const std::uint32_t kPinnedSubgroupSize = ctx.subgroupSizeMin();

    Runtime rt;
    apply_preset(rt, 0);

    Camera camera;
    camera.setMode(Camera::Mode::FreeFly);
    camera.setPosition(glm::vec3(0.0f, 1.0f, 4.5f));
    camera.setFovDeg(FOV_DEG_DEFAULT);
    camera.setNearFar(NEAR_PLANE, FAR_PLANE);
    camera.setAspect(window.aspect());

    // ImGui — match ES init pattern (volumetric-grid/eulerian-smoke/src/main.cpp:1341-1356).
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
            logError("sph-water: ui::initImGui failed");
            return 1;
        }
    }
    glfwSetScrollCallback(window.glfwWindow(), scrollCallback);

    // Alembic export — RealParticleWriter or stub depending on -DGPU_SIMS_USE_ALEMBIC=ON.
    std::unique_ptr<abc::ParticleWriter> alembic_writer;
    fs::path alembic_path = fs::current_path() / "alembic_export" / "sph_water.abc";
    fs::create_directories(alembic_path.parent_path());
    if (abc::isAvailable()) {
        alembic_writer = abc::ParticleWriter::create(alembic_path, /*fps=*/30.0);
        if (alembic_writer) {
            logInfo("Alembic writer ready: {}", alembic_path.string());
        } else {
            logWarn("Alembic writer creation returned null; export disabled.");
        }
    } else {
        logInfo("Alembic not available (built without -DGPU_SIMS_USE_ALEMBIC=ON); export will no-op.");
    }

    // StateWriter — writes capture_<NNNN>/state.json + particles.bin to ./captures.
    fs::create_directories("captures");
    StateWriter capture_writer("captures");

    GpuProfiler profiler(ctx);
    HotReloader reloader;

    // ------------------------------------------------------------------------
    // Pipeline creation (§ 4.D).
    //
    // make_compute / make_compute_pinned transferred verbatim from ES's
    // pattern (volumetric-grid/eulerian-smoke/src/main.cpp:1097-1115); the
    // _pinned variant passes required_subgroup_size + require_full_subgroups
    // through to the Turn 3 common-cpp surface. Pinning targets enumerated
    // in phase11_main_cpp_wiring.md § 2.3 (counting-sort kernels + bilateral
    // filter — subgroup-shuffle / reduction ops want a known subgroup size).
    // ------------------------------------------------------------------------
    auto make_compute = [&](const std::string& shader_rel,
                            std::initializer_list<gv::DescriptorBinding> bindings,
                            std::uint32_t push_const_bytes = 0) {
        gv::ComputePipelineDesc d{};
        d.shader_path        = SD + "/" + shader_rel;
        d.bindings           = std::vector<gv::DescriptorBinding>(bindings);
        d.push_constant_size = push_const_bytes;
        return gv::ComputePipeline::create(ctx, compiler, d);
    };
    auto make_compute_pinned = [&](const std::string& shader_rel,
                                   std::initializer_list<gv::DescriptorBinding> bindings,
                                   std::uint32_t push_const_bytes = 0) {
        gv::ComputePipelineDesc d{};
        d.shader_path             = SD + "/" + shader_rel;
        d.bindings                = std::vector<gv::DescriptorBinding>(bindings);
        d.push_constant_size      = push_const_bytes;
        d.required_subgroup_size  = kPinnedSubgroupSize;
        d.require_full_subgroups  = true;
        return gv::ComputePipeline::create(ctx, compiler, d);
    };

    using BT = VkDescriptorType;
    constexpr BT B = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    constexpr BT U = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    constexpr BT T = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    constexpr BT I = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    constexpr BT S = VK_DESCRIPTOR_TYPE_SAMPLER;
    constexpr VkShaderStageFlags CS = VK_SHADER_STAGE_COMPUTE_BIT;
    constexpr VkShaderStageFlags VS = VK_SHADER_STAGE_VERTEX_BIT;
    constexpr VkShaderStageFlags FS = VK_SHADER_STAGE_FRAGMENT_BIT;

    auto pipe_apply_emitter    = make_compute("apply_emitter.comp.glsl",
                                              {{0,B,1,CS},{1,U,1,CS}});
    auto pipe_initial_fill     = make_compute("initial_fill.comp.glsl",
                                              {{0,B,1,CS},{1,U,1,CS}});
    auto pipe_morton_code      = make_compute("morton_code.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,U,1,CS}});
    auto pipe_density_alpha    = make_compute("density_alpha.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,U,1,CS}});
    auto pipe_divergence_solve = make_compute("divergence_solve.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,B,1,CS},{5,B,1,CS},{6,U,1,CS}});
    auto pipe_density_solve    = make_compute("density_solve.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,B,1,CS},{5,B,1,CS},{6,U,1,CS}});
    auto pipe_integrate_forces = make_compute("integrate_forces.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,U,1,CS}});
    auto pipe_pressure_apply   = make_compute("pressure_apply.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,B,1,CS},{5,U,1,CS}});

    auto pipe_cell_count          = make_compute_pinned("cell_count.comp.glsl",
                                                        {{0,B,1,CS},{1,B,1,CS},{2,U,1,CS}});
    auto pipe_prefix_sum_local    = make_compute_pinned("prefix_sum_local.comp.glsl",
                                                        {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,U,1,CS}});
    auto pipe_prefix_sum_block    = make_compute_pinned("prefix_sum_block.comp.glsl",
                                                        {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,U,1,CS}},
                                                        sizeof(std::uint32_t));  // mode push-const
    auto pipe_prefix_sum_block_l2 = make_compute_pinned("prefix_sum_block_l2.comp.glsl",
                                                        {{0,B,1,CS},{1,B,1,CS},{2,U,1,CS}});
    auto pipe_prefix_sum_addback  = make_compute_pinned("prefix_sum_addback.comp.glsl",
                                                        {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,U,1,CS}});
    auto pipe_scatter             = make_compute_pinned("scatter.comp.glsl",
                                                        {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,U,1,CS}});
    auto pipe_bilateral_smooth    = make_compute("bilateral_smooth.comp.glsl",
                                                 {{0,T,1,CS},{1,I,1,CS},{2,S,1,CS},{3,U,1,CS}});

    // Depth pass — particle_sprite vert+frag → R32_SFLOAT off-screen color
    // attachment. Point-list topology; vert writes gl_PointSize, frag writes
    // linear view-space depth. No alpha blend (depth resolves by gl_FragDepth-
    // style write; closest wins via fragment depth test set in frag shader).
    gv::GraphicsPipelineDesc gp_depth{};
    gp_depth.vertex_shader_path   = SD + "/particle_sprite.vert.glsl";
    gp_depth.fragment_shader_path = SD + "/particle_sprite.frag.glsl";
    gp_depth.color_formats        = {VK_FORMAT_R32_SFLOAT};
    gp_depth.topology             = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    gp_depth.bindings = {{0,B,1,VS|FS},{1,U,1,VS|FS}};
    auto pipe_depth = gv::GraphicsPipeline::create(ctx, compiler, gp_depth);

    // Thickness pass — additive blend (ONE, ONE) ACCUMULATES thickness across
    // overlapping point sprites. Consumes the Turn-4 common-cpp blend-factor
    // surface (graphics_pipeline.hpp `src/dst_color_blend_factor` fields).
    gv::GraphicsPipelineDesc gp_thickness{};
    gp_thickness.vertex_shader_path   = SD + "/thickness.vert.glsl";
    gp_thickness.fragment_shader_path = SD + "/thickness.frag.glsl";
    gp_thickness.color_formats        = {VK_FORMAT_R32_SFLOAT};
    gp_thickness.topology             = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    gp_thickness.blend_enable         = true;
    gp_thickness.src_color_blend_factor = VK_BLEND_FACTOR_ONE;
    gp_thickness.dst_color_blend_factor = VK_BLEND_FACTOR_ONE;
    gp_thickness.color_blend_op         = VK_BLEND_OP_ADD;
    gp_thickness.src_alpha_blend_factor = VK_BLEND_FACTOR_ONE;
    gp_thickness.dst_alpha_blend_factor = VK_BLEND_FACTOR_ONE;
    gp_thickness.alpha_blend_op         = VK_BLEND_OP_ADD;
    gp_thickness.bindings = {{0,B,1,VS|FS},{1,U,1,VS|FS}};
    auto pipe_thickness = gv::GraphicsPipeline::create(ctx, compiler, gp_thickness);

    // Composite — fullscreen triangle into the swapchain image, samples the
    // bilaterally-smoothed depth + thickness, applies Fresnel + Beer-Lambert.
    gv::GraphicsPipelineDesc gp_composite{};
    gp_composite.vertex_shader_path   = SD + "/fullscreen.vert.glsl";
    gp_composite.fragment_shader_path = SD + "/composite.frag.glsl";
    gp_composite.color_formats        = {window.colorFormat()};
    gp_composite.bindings = {{0,T,1,FS},{1,T,1,FS},{2,S,1,FS},{3,U,1,FS}};
    auto pipe_composite = gv::GraphicsPipeline::create(ctx, compiler, gp_composite);

    // ------------------------------------------------------------------------
    // Descriptor-set allocation. One DS per compute pipeline (two for the
    // ping-pong DFSPH inner-loop solvers and the bilateral-smooth filter).
    // ------------------------------------------------------------------------
    VkDescriptorSet ds_apply_emitter    = pipe_apply_emitter.allocateDescriptorSet();
    VkDescriptorSet ds_initial_fill     = pipe_initial_fill.allocateDescriptorSet();
    VkDescriptorSet ds_morton_code      = pipe_morton_code.allocateDescriptorSet();
    VkDescriptorSet ds_cell_count       = pipe_cell_count.allocateDescriptorSet();
    VkDescriptorSet ds_prefix_sum_local = pipe_prefix_sum_local.allocateDescriptorSet();
    VkDescriptorSet ds_prefix_sum_block = pipe_prefix_sum_block.allocateDescriptorSet();
    VkDescriptorSet ds_prefix_sum_block_l2 = pipe_prefix_sum_block_l2.allocateDescriptorSet();
    VkDescriptorSet ds_prefix_sum_addback  = pipe_prefix_sum_addback.allocateDescriptorSet();
    VkDescriptorSet ds_scatter          = pipe_scatter.allocateDescriptorSet();
    VkDescriptorSet ds_density_alpha    = pipe_density_alpha.allocateDescriptorSet();
    VkDescriptorSet ds_divergence_solve[2] = {
        pipe_divergence_solve.allocateDescriptorSet(),
        pipe_divergence_solve.allocateDescriptorSet(),
    };
    VkDescriptorSet ds_density_solve[2] = {
        pipe_density_solve.allocateDescriptorSet(),
        pipe_density_solve.allocateDescriptorSet(),
    };
    VkDescriptorSet ds_integrate_forces = pipe_integrate_forces.allocateDescriptorSet();
    VkDescriptorSet ds_pressure_apply   = pipe_pressure_apply.allocateDescriptorSet();
    // Bilateral DS variants: 0 = depth_image -> smoothed_a (first iter only),
    //                        1 = smoothed_a -> smoothed_b,
    //                        2 = smoothed_b -> smoothed_a.
    VkDescriptorSet ds_bilateral[3] = {
        pipe_bilateral_smooth.allocateDescriptorSet(),
        pipe_bilateral_smooth.allocateDescriptorSet(),
        pipe_bilateral_smooth.allocateDescriptorSet(),
    };
    VkDescriptorSet ds_depth     = pipe_depth.allocateDescriptorSet();
    VkDescriptorSet ds_thickness = pipe_thickness.allocateDescriptorSet();
    // Composite reads the bilateral final-output view, which is one of:
    //   [0] depth_image       (when bilateralIterations == 0)
    //   [1] smoothed_depth_a  (even-iter final: 2, 4, …)
    //   [2] smoothed_depth_b  (odd-iter final: 1, 3, …)
    // Pre-allocate all three and bind per-frame to avoid Vulkan VUID-03047
    // (descriptor cannot be updated while in flight).
    VkDescriptorSet ds_composite[3] = {
        pipe_composite.allocateDescriptorSet(),
        pipe_composite.allocateDescriptorSet(),
        pipe_composite.allocateDescriptorSet(),
    };

    // ------------------------------------------------------------------------
    // Initial tier creation.
    // ------------------------------------------------------------------------
    TierResources tier = createTierResources(ctx, window,
                                             TIER_PARTICLE_COUNTS[rt.tierIndex],
                                             MAX_CELLS);
    rt.particleCapacity = TIER_PARTICLE_COUNTS[rt.tierIndex];

    // ------------------------------------------------------------------------
    // (Re)write every descriptor set against the current `tier`. Called once
    // here, and again from the tier-change apply path (§ 4.J) after a fresh
    // TierResources is constructed.
    // ------------------------------------------------------------------------
    auto rewriteAllDescriptors = [&]() {
        writeApplyEmitterDescriptor(ctx.device(), ds_apply_emitter,
            tier.particles.handle(), tier.uniform_apply_emitter.handle());
        writeInitialFillDescriptor(ctx.device(), ds_initial_fill,
            tier.particles.handle(), tier.uniform_initial_fill.handle());
        writeMortonCodeDescriptor(ctx.device(), ds_morton_code,
            tier.particles.handle(), tier.morton_codes.handle(),
            tier.uniform_sort.handle());
        writeCellCountDescriptor(ctx.device(), ds_cell_count,
            tier.morton_codes.handle(), tier.cell_counts.handle(),
            tier.uniform_sort.handle());
        writePrefixSumLocalDescriptor(ctx.device(), ds_prefix_sum_local,
            tier.cell_counts.handle(), tier.cell_block_prefixes.handle(),
            tier.cell_block_sums.handle(), tier.uniform_sort.handle());
        writePrefixSumBlockDescriptor(ctx.device(), ds_prefix_sum_block,
            tier.cell_block_sums.handle(), tier.cell_block_prefixes.handle(),
            tier.cell_l2_sums.handle(), tier.cell_l2_prefixes.handle(),
            tier.uniform_sort.handle());
        writePrefixSumBlockL2Descriptor(ctx.device(), ds_prefix_sum_block_l2,
            tier.cell_l2_sums.handle(), tier.cell_l2_prefixes.handle(),
            tier.uniform_sort.handle());
        writePrefixSumAddbackDescriptor(ctx.device(), ds_prefix_sum_addback,
            tier.cell_block_prefixes.handle(), tier.cell_block_prefixes.handle(),
            tier.cell_starts.handle(), tier.uniform_sort.handle());
        writeScatterDescriptor(ctx.device(), ds_scatter,
            tier.morton_codes.handle(), tier.cell_starts.handle(),
            tier.sorted_index.handle(), tier.cell_counts_atomic.handle(),
            tier.uniform_sort.handle());
        writeDensityAlphaDescriptor(ctx.device(), ds_density_alpha,
            tier.particles.handle(), tier.cell_starts.handle(),
            tier.sorted_index.handle(), tier.density_alpha.handle(),
            tier.uniform_dfsph.handle());
        // DFSPH solve: ds[0] reads pressure_a / writes pressure_b; ds[1] swaps.
        writeDfsphSolveDescriptor(ctx.device(), ds_divergence_solve[0],
            tier.particles.handle(), tier.density_alpha.handle(),
            tier.cell_starts.handle(), tier.sorted_index.handle(),
            tier.pressure_a.handle(), tier.pressure_b.handle(),
            tier.uniform_dfsph.handle());
        writeDfsphSolveDescriptor(ctx.device(), ds_divergence_solve[1],
            tier.particles.handle(), tier.density_alpha.handle(),
            tier.cell_starts.handle(), tier.sorted_index.handle(),
            tier.pressure_b.handle(), tier.pressure_a.handle(),
            tier.uniform_dfsph.handle());
        writeDfsphSolveDescriptor(ctx.device(), ds_density_solve[0],
            tier.particles.handle(), tier.density_alpha.handle(),
            tier.cell_starts.handle(), tier.sorted_index.handle(),
            tier.pressure_a.handle(), tier.pressure_b.handle(),
            tier.uniform_dfsph.handle());
        writeDfsphSolveDescriptor(ctx.device(), ds_density_solve[1],
            tier.particles.handle(), tier.density_alpha.handle(),
            tier.cell_starts.handle(), tier.sorted_index.handle(),
            tier.pressure_b.handle(), tier.pressure_a.handle(),
            tier.uniform_dfsph.handle());
        writeIntegrateForcesDescriptor(ctx.device(), ds_integrate_forces,
            tier.particles.handle(), tier.density_alpha.handle(),
            tier.cell_starts.handle(), tier.sorted_index.handle(),
            tier.uniform_dfsph.handle());
        writePressureApplyDescriptor(ctx.device(), ds_pressure_apply,
            tier.particles.handle(), tier.density_alpha.handle(),
            tier.pressure_a.handle(), tier.cell_starts.handle(),
            tier.sorted_index.handle(), tier.uniform_dfsph.handle());
        writeBilateralSmoothDescriptor(ctx.device(), ds_bilateral[0],
            tier.depth_image.view(), tier.smoothed_depth_a.view(),
            tier.sampler_linear, tier.uniform_bilateral.handle());
        writeBilateralSmoothDescriptor(ctx.device(), ds_bilateral[1],
            tier.smoothed_depth_a.view(), tier.smoothed_depth_b.view(),
            tier.sampler_linear, tier.uniform_bilateral.handle());
        writeBilateralSmoothDescriptor(ctx.device(), ds_bilateral[2],
            tier.smoothed_depth_b.view(), tier.smoothed_depth_a.view(),
            tier.sampler_linear, tier.uniform_bilateral.handle());
        writeParticleSpriteDescriptor(ctx.device(), ds_depth,
            tier.particles.handle(), tier.uniform_render_view.handle());
        writeThicknessDescriptor(ctx.device(), ds_thickness,
            tier.particles.handle(), tier.uniform_render_view.handle());
        // Composite reads the FINAL bilateral output. Pre-write one descriptor
        // per possible final view; main loop binds whichever matches the actual
        // bilateral iteration count (Fix D: descriptors are not rewritten while
        // command buffers are in flight).
        writeCompositeDescriptor(ctx.device(), ds_composite[0],
            tier.depth_image.view(), tier.thickness_image.view(),
            tier.sampler_linear, tier.uniform_composite.handle());
        writeCompositeDescriptor(ctx.device(), ds_composite[1],
            tier.smoothed_depth_a.view(), tier.thickness_image.view(),
            tier.sampler_linear, tier.uniform_composite.handle());
        writeCompositeDescriptor(ctx.device(), ds_composite[2],
            tier.smoothed_depth_b.view(), tier.thickness_image.view(),
            tier.sampler_linear, tier.uniform_composite.handle());
    };
    rewriteAllDescriptors();

    // ------------------------------------------------------------------------
    // Uniform packing helpers.
    //
    // Each helper writes a single std140 block to the host-visible UBO via
    // uploadDirect; field order matches the GLSL declaration in the
    // corresponding shader's `layout(std140) uniform` block. cellSize comes
    // from rt.cellSize; supportRadius from rt.supportRadius.
    // ------------------------------------------------------------------------
    auto next_pow2 = [](std::uint32_t v) {
        if (v <= 1) return 1u;
        v -= 1;
        v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
        return v + 1u;
    };
    auto compute_cells_per_axis = [&](glm::uvec3& outAxes, std::uint32_t& outMax) {
        glm::vec3 ext = rt.domainMax - rt.domainMin;
        glm::uvec3 axes;
        axes.x = std::max(1u, std::uint32_t(std::ceil(ext.x / rt.cellSize)));
        axes.y = std::max(1u, std::uint32_t(std::ceil(ext.y / rt.cellSize)));
        axes.z = std::max(1u, std::uint32_t(std::ceil(ext.z / rt.cellSize)));
        axes.x = next_pow2(axes.x);
        axes.y = next_pow2(axes.y);
        axes.z = next_pow2(axes.z);
        outAxes = axes;
        outMax  = std::max({axes.x, axes.y, axes.z});
    };

    // Kernel mass for SPH: m = rho_0 * spacing^3, spacing = 2 * particleRadius.
    auto particle_mass = [&]() {
        float spacing = 2.0f * rt.particleRadius;
        return DENSITY_0 * spacing * spacing * spacing;
    };
    auto kernel_norm_3d_value = [&]() {
        float h = rt.supportRadius;
        return 8.0f / (float(M_PI) * h * h * h);
    };
    auto grad_kernel_norm_3d_value = [&]() {
        float h = rt.supportRadius;
        return 48.0f / (float(M_PI) * h * h * h * h);
    };

    auto pack_sort_uniform = [&]() {
        glm::uvec3 axes; std::uint32_t maxAxis;
        compute_cells_per_axis(axes, maxAxis);
        std::uint32_t totalCells = std::min<std::uint32_t>(MAX_CELLS, maxAxis * maxAxis * maxAxis);
        struct alignas(16) Layout {
            std::uint32_t particleCount;
            std::uint32_t _pad0a;
            std::uint32_t totalCells;
            std::uint32_t _pad0b;
            float cellsXYZ[4];          // xyz = axes per axis (rounded pow2), w = pad
            float domainMin_pad[4];     // xyz = domainMin, w = cellSize
            float domainMax_pad[4];     // xyz = domainMax
        } u{};
        u.particleCount = rt.particleCount;
        u.totalCells    = totalCells;
        u.cellsXYZ[0]   = float(axes.x);
        u.cellsXYZ[1]   = float(axes.y);
        u.cellsXYZ[2]   = float(axes.z);
        u.domainMin_pad[0] = rt.domainMin.x;
        u.domainMin_pad[1] = rt.domainMin.y;
        u.domainMin_pad[2] = rt.domainMin.z;
        u.domainMin_pad[3] = rt.cellSize;
        u.domainMax_pad[0] = rt.domainMax.x;
        u.domainMax_pad[1] = rt.domainMax.y;
        u.domainMax_pad[2] = rt.domainMax.z;
        tier.uniform_sort.uploadDirect(&u, sizeof(u));
    };

    auto pack_dfsph_uniform = [&](float dt, float mode) {
        glm::uvec3 axes; std::uint32_t maxAxis;
        compute_cells_per_axis(axes, maxAxis);
        struct alignas(16) Layout {
            std::uint32_t particleCount;          //  0
            std::uint32_t cellsPerAxisX;          //  4
            std::uint32_t cellsPerAxisY;          //  8
            std::uint32_t cellsPerAxisZ;          // 12
            float supportRadius;                  // 16
            float particleMass;                   // 20
            float density0;                       // 24
            float kernelNorm3D;                   // 28
            float gradKernelNorm3D;               // 32
            float dt;                             // 36
            float viscosity;                      // 40
            float cohesion;                       // 44
            float vorticityStrength;              // 48
            float jacobiRelax;                    // 52
            float _pad0;                          // 56
            float _pad1;                          // 60
            float gravity_pad[4];                 // 64
            float domainMin_cellSize[4];          // 80
            float domainMax_pad[4];               // 96
        } u{};
        static_assert(sizeof(Layout) == 112, "DFSPH canonical UBO size drift");
        u.particleCount   = rt.particleCount;
        u.cellsPerAxisX   = axes.x;
        u.cellsPerAxisY   = axes.y;
        u.cellsPerAxisZ   = axes.z;
        u.supportRadius   = rt.supportRadius;
        u.particleMass    = particle_mass();
        u.density0        = DENSITY_0;
        u.dt              = dt;
        u.viscosity       = rt.viscosity;
        u.cohesion        = rt.cohesion;
        u.vorticityStrength = rt.vorticityStrength;
        u.kernelNorm3D    = kernel_norm_3d_value();
        u.gradKernelNorm3D= grad_kernel_norm_3d_value();
        u.jacobiRelax     = DFSPH_JACOBI_RELAX;
        u.gravity_pad[0]  = 0.0f;
        u.gravity_pad[1]  = -9.81f;
        u.gravity_pad[2]  = 0.0f;
        u.gravity_pad[3]  = mode;
        u.domainMin_cellSize[0] = rt.domainMin.x;
        u.domainMin_cellSize[1] = rt.domainMin.y;
        u.domainMin_cellSize[2] = rt.domainMin.z;
        u.domainMin_cellSize[3] = rt.cellSize;
        u.domainMax_pad[0] = rt.domainMax.x;
        u.domainMax_pad[1] = rt.domainMax.y;
        u.domainMax_pad[2] = rt.domainMax.z;
        tier.uniform_dfsph.uploadDirect(&u, sizeof(u));
    };

    auto pack_render_view_uniform = [&]() {
        const std::uint32_t W = window.extent().width;
        const std::uint32_t H = window.extent().height;
        glm::mat4 view = camera.view();
        glm::mat4 proj = camera.projection();
        glm::mat4 vp   = proj * view;
        glm::mat4 ivp  = glm::inverse(vp);
        struct alignas(16) Layout {
            glm::mat4 viewProj;
            glm::mat4 view;
            glm::mat4 proj;
            glm::mat4 invViewProj;
            glm::vec4 cameraPos_pad;
            glm::vec4 viewport_pad;
            float particleRadius;
            float pointScale;
            float thicknessPerParticle;
            float _pad0;
        } u{};
        u.viewProj    = vp;
        u.view        = view;
        u.proj        = proj;
        u.invViewProj = ivp;
        u.cameraPos_pad = glm::vec4(camera.position(), 0.0f);
        u.viewport_pad  = glm::vec4(float(W), float(H), 0.0f, 0.0f);
        u.particleRadius      = rt.particleRadius;
        // pointScale maps view-space radius to pixels at depth=1: pointScale =
        // viewport_height / (2 * tan(fov/2)).
        u.pointScale          = float(H) / (2.0f * std::tan(glm::radians(camera.fovDeg()) * 0.5f));
        u.thicknessPerParticle= rt.thicknessPerParticle;
        tier.uniform_render_view.uploadDirect(&u, sizeof(u));
    };

    auto pack_bilateral_uniform = [&](int direction) {
        const std::uint32_t W = window.extent().width;
        const std::uint32_t H = window.extent().height;
        struct alignas(16) Layout {
            float viewport_size[2];
            float sigmaSpatial;
            float sigmaDepth;
            int   passDirection;
            int   _pad0;
            float _pad1[2];
        } u{};
        u.viewport_size[0] = float(W);
        u.viewport_size[1] = float(H);
        u.sigmaSpatial     = rt.bilateralSigmaSpatial;
        u.sigmaDepth       = rt.bilateralSigmaDepth;
        u.passDirection    = direction;
        tier.uniform_bilateral.uploadDirect(&u, sizeof(u));
    };

    auto pack_composite_uniform = [&]() {
        const std::uint32_t W = window.extent().width;
        const std::uint32_t H = window.extent().height;
        glm::mat4 view = camera.view();
        glm::mat4 proj = camera.projection();
        glm::mat4 ivp  = glm::inverse(proj * view);
        glm::mat4 inv_view = glm::inverse(view);
        struct alignas(16) Layout {
            glm::mat4 invViewProj;
            glm::mat4 invView;
            glm::vec4 cameraPos_pad;
            glm::vec4 waterTint_roughness;
            glm::vec4 skyZenith_pad;
            glm::vec4 skyHorizon_pad;
            glm::vec4 absorption_thickness_pad;
            glm::vec4 viewport_pad;
            float near_plane;
            float far_plane;
            float exposure;
            float fresnel_F0;
        } u{};
        u.invViewProj = ivp;
        u.invView     = inv_view;
        u.cameraPos_pad        = glm::vec4(camera.position(), 0.0f);
        u.waterTint_roughness  = glm::vec4(rt.waterTint, rt.surfaceRoughness);
        u.skyZenith_pad        = glm::vec4(rt.skyZenith, 0.0f);
        u.skyHorizon_pad       = glm::vec4(rt.skyHorizon, 0.0f);
        u.absorption_thickness_pad = glm::vec4(rt.thicknessPerParticle, 0.0f, 0.0f, 0.0f);
        u.viewport_pad   = glm::vec4(float(W), float(H), 0.0f, 0.0f);
        u.near_plane     = NEAR_PLANE;
        u.far_plane      = FAR_PLANE;
        u.exposure       = rt.exposure;
        u.fresnel_F0     = 0.02f;
        tier.uniform_composite.uploadDirect(&u, sizeof(u));
    };

    auto pack_initial_fill_uniform = [&](std::uint32_t brick_count, std::uint32_t total_count) {
        const SphPreset& p = SPH_PRESETS[size_t(rt.presetIndex)];
        struct alignas(16) Layout {
            float brickMin_radius[4];
            float brickMax_pad[4];
            float dropletCenter_radius[4];
            float dropletVel_addFlag[4];
            std::uint32_t brickParticleCount;
            std::uint32_t totalInitialParticles;
            std::uint32_t _pad0;
            std::uint32_t _pad1;
        } u{};
        u.brickMin_radius[0] = p.initial_brick_min.x;
        u.brickMin_radius[1] = p.initial_brick_min.y;
        u.brickMin_radius[2] = p.initial_brick_min.z;
        u.brickMin_radius[3] = rt.particleRadius;
        u.brickMax_pad[0]    = p.initial_brick_max.x;
        u.brickMax_pad[1]    = p.initial_brick_max.y;
        u.brickMax_pad[2]    = p.initial_brick_max.z;
        u.dropletCenter_radius[0] = p.droplet_center.x;
        u.dropletCenter_radius[1] = p.droplet_center.y;
        u.dropletCenter_radius[2] = p.droplet_center.z;
        u.dropletCenter_radius[3] = p.droplet_radius;
        u.dropletVel_addFlag[0]   = p.droplet_velocity_init.x;
        u.dropletVel_addFlag[1]   = p.droplet_velocity_init.y;
        u.dropletVel_addFlag[2]   = p.droplet_velocity_init.z;
        u.dropletVel_addFlag[3]   = p.add_droplet ? 1.0f : 0.0f;
        u.brickParticleCount     = brick_count;
        u.totalInitialParticles  = total_count;
        tier.uniform_initial_fill.uploadDirect(&u, sizeof(u));
    };

    auto pack_apply_emitter_uniform = [&](float dt) {
        struct EmitterGpu {
            float pos_radius[4];
            float vel_rate_age_pad[4];
        };
        struct alignas(16) Layout {
            std::uint32_t emitterCount;
            std::uint32_t particleCount;
            std::uint32_t presetParticleCount;
            std::uint32_t particleCapacity;
            float         particleRadius;
            float         dt;
            float         _pad[2];
            EmitterGpu    emitters[EMITTER_CAP];
        } u{};
        u.emitterCount        = std::uint32_t(rt.emitters.size());
        u.particleCount       = rt.particleCount;
        u.presetParticleCount = rt.presetParticleCount;
        u.particleCapacity    = rt.particleCapacity;
        u.particleRadius      = rt.particleRadius;
        u.dt                  = dt;
        for (size_t i = 0; i < rt.emitters.size() && i < size_t(EMITTER_CAP); ++i) {
            u.emitters[i].pos_radius[0] = rt.emitters[i].pos.x;
            u.emitters[i].pos_radius[1] = rt.emitters[i].pos.y;
            u.emitters[i].pos_radius[2] = rt.emitters[i].pos.z;
            u.emitters[i].pos_radius[3] = rt.emitters[i].radius;
            u.emitters[i].vel_rate_age_pad[0] = rt.emitters[i].velocity.x;
            u.emitters[i].vel_rate_age_pad[1] = rt.emitters[i].velocity.y;
            u.emitters[i].vel_rate_age_pad[2] = rt.emitters[i].velocity.z;
            u.emitters[i].vel_rate_age_pad[3] = rt.emitters[i].emissionRate;
        }
        tier.uniform_apply_emitter.uploadDirect(&u, sizeof(u));
    };

    // ------------------------------------------------------------------------
    // Initial-fill seeding. For each preset we estimate the brick count from
    // its volume, add the droplet (if any), dispatch initial_fill, and update
    // rt.particleCount / rt.presetParticleCount accordingly.
    // ------------------------------------------------------------------------
    auto seedInitialFill = [&]() {
        const SphPreset& p = SPH_PRESETS[size_t(rt.presetIndex)];
        float spacing = 2.0f * rt.particleRadius;
        glm::vec3 ext = p.initial_brick_max - p.initial_brick_min;
        std::uint32_t dx = std::max(1u, std::uint32_t(ext.x / spacing));
        std::uint32_t dy = std::max(1u, std::uint32_t(ext.y / spacing));
        std::uint32_t dz = std::max(1u, std::uint32_t(ext.z / spacing));
        std::uint32_t brick = std::min<std::uint32_t>(rt.particleCapacity, dx * dy * dz);

        // Droplet count: pack into a sphere of radius `droplet_radius` at
        // particle spacing.
        std::uint32_t droplet = 0u;
        if (p.add_droplet) {
            std::uint32_t dr = std::max(1u, std::uint32_t(p.droplet_radius / spacing));
            droplet = (4u * dr * dr * dr) / 3u;
            droplet = std::min<std::uint32_t>(droplet, rt.particleCapacity - brick);
        }
        std::uint32_t total = brick + droplet;
        pack_initial_fill_uniform(brick, total);

        ctx.runOneShot([&](VkCommandBuffer cb) {
            pipe_initial_fill.dispatch(cb, ds_initial_fill,
                                       (total + 255u) / 256u, 1u, 1u);
        });

        rt.particleCount       = total;
        rt.presetParticleCount = total;
    };
    seedInitialFill();

    // ------------------------------------------------------------------------
    // Hot-reload registration (§ 4.E). Each flag is set by the worker thread
    // when a watched file changes; the main loop checks flags after poll()
    // and calls the corresponding pipeline's reload() on the next frame.
    // ------------------------------------------------------------------------
    bool reload_apply_emitter=false, reload_initial_fill=false;
    bool reload_morton_code=false, reload_cell_count=false;
    bool reload_prefix_sum_local=false, reload_prefix_sum_block=false;
    bool reload_prefix_sum_block_l2=false, reload_prefix_sum_addback=false;
    bool reload_scatter=false, reload_density_alpha=false;
    bool reload_divergence_solve=false, reload_density_solve=false;
    bool reload_integrate_forces=false, reload_pressure_apply=false;
    bool reload_bilateral_smooth=false;
    bool reload_depth=false, reload_thickness=false, reload_composite=false;

    auto W_watch = [&](const std::string& rel, bool* flag) {
        reloader.watch(SD + "/" + rel,
                       [flag](const std::filesystem::path&){ *flag = true; });
    };
    W_watch("apply_emitter.comp.glsl",       &reload_apply_emitter);
    W_watch("initial_fill.comp.glsl",        &reload_initial_fill);
    W_watch("morton_code.comp.glsl",         &reload_morton_code);
    W_watch("cell_count.comp.glsl",          &reload_cell_count);
    W_watch("prefix_sum_local.comp.glsl",    &reload_prefix_sum_local);
    W_watch("prefix_sum_block.comp.glsl",    &reload_prefix_sum_block);
    W_watch("prefix_sum_block_l2.comp.glsl", &reload_prefix_sum_block_l2);
    W_watch("prefix_sum_addback.comp.glsl",  &reload_prefix_sum_addback);
    W_watch("scatter.comp.glsl",             &reload_scatter);
    W_watch("density_alpha.comp.glsl",       &reload_density_alpha);
    W_watch("divergence_solve.comp.glsl",    &reload_divergence_solve);
    W_watch("density_solve.comp.glsl",       &reload_density_solve);
    W_watch("integrate_forces.comp.glsl",    &reload_integrate_forces);
    W_watch("pressure_apply.comp.glsl",      &reload_pressure_apply);
    W_watch("bilateral_smooth.comp.glsl",    &reload_bilateral_smooth);
    W_watch("particle_sprite.vert.glsl",     &reload_depth);
    W_watch("particle_sprite.frag.glsl",     &reload_depth);
    W_watch("thickness.vert.glsl",           &reload_thickness);
    W_watch("thickness.frag.glsl",           &reload_thickness);
    W_watch("fullscreen.vert.glsl",          &reload_composite);
    W_watch("composite.frag.glsl",           &reload_composite);

    // ------------------------------------------------------------------------
    // F5 / F9 capture lambdas (§ 4.F).
    //
    // Bare buffer names per state_writer's `.bin` auto-append convention.
    // Top-level meta key = "sphWater" per Phase 11 spec § 5.C.
    // Camera state via camera.toJson(json&) -> void (probe-verified).
    // ------------------------------------------------------------------------
    auto capture_save = [&]() {
        renderer.waitIdle();
        const std::uint32_t N = rt.particleCount;
        if (N == 0) {
            gpusims::ui::pushToast("No particles to capture", false);
            return;
        }
        std::vector<std::uint8_t> particles_bytes(std::size_t(N) * 128);
        std::vector<std::uint8_t> density_alpha_bytes(std::size_t(N) * 16);
        std::vector<std::uint8_t> pressure_bytes(std::size_t(N) * 4);
        std::vector<std::uint8_t> morton_bytes(std::size_t(N) * 4);
        std::vector<std::uint8_t> sorted_bytes(std::size_t(N) * 4);
        std::vector<std::uint8_t> cell_starts_bytes(std::size_t(MAX_CELLS) * 4);

        tier.particles.readback(ctx,     particles_bytes.data(),    particles_bytes.size());
        tier.density_alpha.readback(ctx, density_alpha_bytes.data(),density_alpha_bytes.size());
        tier.pressure_a.readback(ctx,    pressure_bytes.data(),     pressure_bytes.size());
        tier.morton_codes.readback(ctx,  morton_bytes.data(),       morton_bytes.size());
        tier.sorted_index.readback(ctx,  sorted_bytes.data(),       sorted_bytes.size());
        tier.cell_starts.readback(ctx,   cell_starts_bytes.data(),  cell_starts_bytes.size());

        capture_writer.beginFrame(std::uint32_t(rt.iteration));
        json meta;
        meta["iteration"]      = std::uint64_t(rt.iteration);
        meta["particle_count"] = N;
        meta["tier_index"]     = rt.tierIndex;
        meta["preset_index"]   = rt.presetIndex;
        meta["substeps"]       = rt.substeps;
        json cam_j;
        camera.toJson(cam_j);
        meta["camera"]         = cam_j;
        capture_writer.setMeta("sphWater", meta);

        capture_writer.saveBuffer("particles", particles_bytes.data(), particles_bytes.size(),
            {{"count", std::uint64_t(N)}, {"stride", 128}, {"format", "raw"}});
        capture_writer.saveBuffer("density_alpha", density_alpha_bytes.data(), density_alpha_bytes.size(),
            {{"count", std::uint64_t(N)}, {"stride", 16}, {"format", "raw"}});
        capture_writer.saveBuffer("pressure", pressure_bytes.data(), pressure_bytes.size(),
            {{"count", std::uint64_t(N)}, {"stride", 4}, {"format", "raw"}});
        capture_writer.saveBuffer("morton_codes", morton_bytes.data(), morton_bytes.size(),
            {{"count", std::uint64_t(N)}, {"stride", 4}, {"format", "raw"}});
        capture_writer.saveBuffer("sorted_index", sorted_bytes.data(), sorted_bytes.size(),
            {{"count", std::uint64_t(N)}, {"stride", 4}, {"format", "raw"}});
        capture_writer.saveBuffer("cell_starts", cell_starts_bytes.data(), cell_starts_bytes.size(),
            {{"count", std::uint64_t(MAX_CELLS)}, {"stride", 4}, {"format", "raw"}});

        capture_writer.endFrame();
        gpusims::ui::pushToast(("Saved capture #" + std::to_string(rt.iteration)).c_str(), true);
        logInfo("F5: saved capture {}", rt.iteration);
    };

    auto capture_load = [&]() {
        auto latest = StateReader::findLatest("captures");
        if (!latest.has_value()) {
            gpusims::ui::pushToast("No captures to load", false);
            logWarn("F9: no captures found");
            return;
        }
        auto cap = StateReader::open(*latest);
        if (!cap.has_value()) {
            gpusims::ui::pushToast("Failed to open capture", false);
            logError("F9: failed to open {}", latest->string());
            return;
        }
        json meta = cap->meta("sphWater");
        if (meta.is_null()) {
            gpusims::ui::pushToast("Capture has no sphWater meta", false);
            logError("F9: capture missing sphWater key");
            return;
        }
        renderer.waitIdle();

        std::uint32_t saved_count = meta.value("particle_count", rt.particleCount);
        auto particles_buf      = cap->buffer("particles");
        auto density_alpha_buf  = cap->buffer("density_alpha");
        auto pressure_buf       = cap->buffer("pressure");
        auto morton_buf         = cap->buffer("morton_codes");
        auto sorted_buf         = cap->buffer("sorted_index");
        auto cell_starts_buf    = cap->buffer("cell_starts");

        // Size guards (use the SAVED count, not the current tier's count).
        if (particles_buf.size() != std::size_t(saved_count) * 128 ||
            density_alpha_buf.size() != std::size_t(saved_count) * 16 ||
            pressure_buf.size() != std::size_t(saved_count) * 4 ||
            morton_buf.size() != std::size_t(saved_count) * 4 ||
            sorted_buf.size() != std::size_t(saved_count) * 4) {
            gpusims::ui::pushToast("Capture buffer size mismatch", false);
            logError("F9: buffer-size mismatch (saved count {}) — bailing", saved_count);
            return;
        }

        tier.particles.stage(ctx,     particles_buf.data(),     particles_buf.size());
        tier.density_alpha.stage(ctx, density_alpha_buf.data(), density_alpha_buf.size());
        tier.pressure_a.stage(ctx,    pressure_buf.data(),      pressure_buf.size());
        tier.morton_codes.stage(ctx,  morton_buf.data(),        morton_buf.size());
        tier.sorted_index.stage(ctx,  sorted_buf.data(),        sorted_buf.size());
        if (!cell_starts_buf.empty()) {
            const std::size_t up = std::min(cell_starts_buf.size(),
                                            std::size_t(MAX_CELLS) * 4);
            tier.cell_starts.stage(ctx, cell_starts_buf.data(), up);
        }
        std::vector<std::uint8_t> zero(std::size_t(saved_count) * 4, 0);
        tier.pressure_b.stage(ctx, zero.data(), zero.size());

        rt.particleCount = saved_count;
        rt.iteration     = meta.value("iteration", std::uint64_t(0));
        rt.presetIndex   = meta.value("preset_index", rt.presetIndex);
        rt.substeps      = meta.value("substeps", rt.substeps);
        if (meta.contains("camera")) {
            camera.fromJson(meta["camera"]);
        }

        gpusims::ui::pushToast(("Loaded capture #" + std::to_string(rt.iteration)).c_str(), true);
        logInfo("F9: loaded capture {}", rt.iteration);
    };

    // Tier-change apply (§ 4.J): pendingTierIndex is set by the dropdown,
    // tier_apply_clicked by the Apply button. We perform the recreation at
    // end-of-frame (after endFrame) so no command buffer mid-recording sees
    // a destroyed Buffer.
    bool tier_apply_clicked = false;

    bool show_demo = false;
    auto last_time = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window.glfwWindow())) {
        glfwPollEvents();

        // Hot-reload poll + flag-driven pipeline recreation. Done before the
        // frame begins so the reload's deletion-queue entry has somewhere to
        // land. If a reload fails the existing pipeline stays valid and the
        // error goes to the log; we surface it via the reloader's recent-event
        // ImGui toast in the panel below.
        reloader.poll();

        // Frame timing.
        auto now = std::chrono::high_resolution_clock::now();
        float frame_dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;
        frame_dt = std::clamp(frame_dt, 1.0f / 240.0f, 1.0f / 15.0f);

        // Mouse edges.
        g_mouse_edge.poll(window.glfwWindow());

        // Camera.
        camera.setAspect(window.aspect());
        camera.update(frame_dt, g_input.snapshot(window.glfwWindow()));

        // F5 / F9 rising edges → real capture-save / capture-load.
        static bool was_f5 = false, was_f9 = false;
        bool now_f5 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F5) == GLFW_PRESS;
        bool now_f9 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F9) == GLFW_PRESS;
        if (now_f5 && !was_f5) capture_save();
        if (now_f9 && !was_f9) capture_load();
        was_f5 = now_f5;
        was_f9 = now_f9;

        // LMB click-to-place emitter — appends an Emitter to rt.emitters,
        // which the apply_emitter dispatch consumes via uniform_apply_emitter.
        if (g_mouse_edge.lmb_pressed) {
            glm::vec3 plane_anchor = 0.5f * (rt.domainMin + rt.domainMax);
            glm::vec3 hit;
            if (unprojectToPaintPlane(camera, window.glfwWindow(), plane_anchor, hit)) {
                rt.paintPlanePoint = hit;
                if (int(rt.emitters.size()) < EMITTER_CAP) {
                    rt.emitters.push_back(Emitter{hit, 0.1f, glm::vec3(0.0f, 5.0f, 0.0f), 5000.0f, 0.0f});
                }
            }
        }
        rt.lmbHeld = g_input.snapshot(window.glfwWindow()).mouse_left;

        // ----------------------------------------------------------------
        // Frame begin (renderer manages swapchain image acquire).
        // ----------------------------------------------------------------
        gv::Frame* frame = renderer.beginFrame();
        if (!frame) continue;
        VkCommandBuffer cmd = frame->command_buffer;

        // Apply pending reloads now that the frame's deletion queue is alive.
        // Each call defers old VkPipeline / VkShaderModule destruction onto
        // this frame so it's safe under in-flight execution.
        auto try_reload = [&](auto& pipe, bool& flag, const char* name) {
            if (!flag) return;
            std::string err;
            if (pipe.reload(ctx, compiler, *frame, &err)) {
                logInfo("[sph-water] reloaded {}", name);
                reloader.reportSuccess(SD + "/" + name);
            } else {
                logError("[sph-water] reload {} failed: {}", name, err);
                reloader.reportFailure(SD + "/" + name, err);
            }
            flag = false;
        };
        try_reload(pipe_apply_emitter,       reload_apply_emitter,       "apply_emitter.comp.glsl");
        try_reload(pipe_initial_fill,        reload_initial_fill,        "initial_fill.comp.glsl");
        try_reload(pipe_morton_code,         reload_morton_code,         "morton_code.comp.glsl");
        try_reload(pipe_cell_count,          reload_cell_count,          "cell_count.comp.glsl");
        try_reload(pipe_prefix_sum_local,    reload_prefix_sum_local,    "prefix_sum_local.comp.glsl");
        try_reload(pipe_prefix_sum_block,    reload_prefix_sum_block,    "prefix_sum_block.comp.glsl");
        try_reload(pipe_prefix_sum_block_l2, reload_prefix_sum_block_l2, "prefix_sum_block_l2.comp.glsl");
        try_reload(pipe_prefix_sum_addback,  reload_prefix_sum_addback,  "prefix_sum_addback.comp.glsl");
        try_reload(pipe_scatter,             reload_scatter,             "scatter.comp.glsl");
        try_reload(pipe_density_alpha,       reload_density_alpha,       "density_alpha.comp.glsl");
        try_reload(pipe_divergence_solve,    reload_divergence_solve,    "divergence_solve.comp.glsl");
        try_reload(pipe_density_solve,       reload_density_solve,       "density_solve.comp.glsl");
        try_reload(pipe_integrate_forces,    reload_integrate_forces,    "integrate_forces.comp.glsl");
        try_reload(pipe_pressure_apply,      reload_pressure_apply,      "pressure_apply.comp.glsl");
        try_reload(pipe_bilateral_smooth,    reload_bilateral_smooth,    "bilateral_smooth.comp.glsl");
        try_reload(pipe_depth,               reload_depth,               "particle_sprite.{vert,frag}.glsl");
        try_reload(pipe_thickness,           reload_thickness,           "thickness.{vert,frag}.glsl");
        try_reload(pipe_composite,           reload_composite,           "composite.frag.glsl");

        // Profiler frame begin.
        profiler.beginFrame(cmd, frame->in_flight_index);

        // ----------------------------------------------------------------
        // Per-frame uniform packing (host-visible UBOs).
        // ----------------------------------------------------------------
        const float substep_dt = std::clamp(frame_dt / float(std::max(rt.substeps, 1)), DT_MIN, DT_MAX);
        rt.dt = substep_dt;
        pack_sort_uniform();
        pack_dfsph_uniform(substep_dt, 0.0f);  // FORCES mode for forces calls
        pack_render_view_uniform();
        pack_composite_uniform();
        pack_apply_emitter_uniform(substep_dt);

        // ----------------------------------------------------------------
        // DFSPH dispatch chain (§ 4.G), once per substep.
        // ----------------------------------------------------------------
        const std::uint32_t wg_particle = (rt.particleCount + WG_DIM_DFSPH - 1) / WG_DIM_DFSPH;
        const std::uint32_t wg_cell     = (MAX_CELLS + WG_DIM_SORT - 1) / WG_DIM_SORT;

        auto cs_barrier = [&]() {
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);
        };

        for (int sub = 0; !rt.paused && sub < rt.substeps && rt.particleCount > 0; ++sub) {
            // Apply emitters (no-op if rt.emitters is empty; pipeline still
            // runs with emitterCount=0 — early-out in the shader).
            if (!rt.emitters.empty()) {
                auto _ = profiler.scope(cmd, "apply_emitter");
                pipe_apply_emitter.dispatch(cmd, ds_apply_emitter, wg_particle, 1, 1);
                cs_barrier();
            }

            // Spatial hash: zero counters then count.
            vkCmdFillBuffer(cmd, tier.cell_counts.handle(),         0, VK_WHOLE_SIZE, 0u);
            vkCmdFillBuffer(cmd, tier.cell_counts_atomic.handle(),  0, VK_WHOLE_SIZE, 0u);
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);

            {
                auto _ = profiler.scope(cmd, "morton_code");
                pipe_morton_code.dispatch(cmd, ds_morton_code, wg_particle, 1, 1);
            }
            cs_barrier();
            {
                auto _ = profiler.scope(cmd, "cell_count");
                pipe_cell_count.dispatch(cmd, ds_cell_count, wg_particle, 1, 1);
            }
            cs_barrier();
            {
                auto _ = profiler.scope(cmd, "prefix_sum_local");
                pipe_prefix_sum_local.dispatch(cmd, ds_prefix_sum_local, wg_cell, 1, 1);
            }
            cs_barrier();
            // prefix_sum_block: mode-0 (reduce) over block_sums.
            {
                auto _ = profiler.scope(cmd, "prefix_sum_block");
                std::uint32_t mode = 0u;
                pipe_prefix_sum_block.dispatch(cmd, ds_prefix_sum_block,
                                               wg_cell, 1, 1, &mode, sizeof(mode));
            }
            cs_barrier();
            {
                auto _ = profiler.scope(cmd, "prefix_sum_block_l2");
                pipe_prefix_sum_block_l2.dispatch(cmd, ds_prefix_sum_block_l2, 1, 1, 1);
            }
            cs_barrier();
            // prefix_sum_block: mode-1 (addback L2) over block_prefixes.
            {
                auto _ = profiler.scope(cmd, "prefix_sum_block_addback");
                std::uint32_t mode = 1u;
                pipe_prefix_sum_block.dispatch(cmd, ds_prefix_sum_block,
                                               wg_cell, 1, 1, &mode, sizeof(mode));
            }
            cs_barrier();
            {
                auto _ = profiler.scope(cmd, "prefix_sum_addback");
                pipe_prefix_sum_addback.dispatch(cmd, ds_prefix_sum_addback, wg_cell, 1, 1);
            }
            cs_barrier();
            {
                auto _ = profiler.scope(cmd, "scatter");
                pipe_scatter.dispatch(cmd, ds_scatter, wg_particle, 1, 1);
            }
            cs_barrier();

            // Density + alpha factor.
            {
                auto _ = profiler.scope(cmd, "density_alpha");
                pipe_density_alpha.dispatch(cmd, ds_density_alpha, wg_particle, 1, 1);
            }
            cs_barrier();

            // Divergence-free Jacobi inner loop. Zero both pressure buffers
            // at the start; alternate ds_divergence_solve[i%2].
            if (rt.divSolverEnabled) {
                vkCmdFillBuffer(cmd, tier.pressure_a.handle(), 0, VK_WHOLE_SIZE, 0u);
                vkCmdFillBuffer(cmd, tier.pressure_b.handle(), 0, VK_WHOLE_SIZE, 0u);
                gv::memoryBarrier(cmd,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);
                int iters = std::max(rt.minIterDivergence, 1);
                for (int i = 0; i < iters; ++i) {
                    auto _ = profiler.scope(cmd, "divergence_solve");
                    pipe_divergence_solve.dispatch(cmd, ds_divergence_solve[i % 2], wg_particle, 1, 1);
                    cs_barrier();
                }
                {
                    auto _ = profiler.scope(cmd, "pressure_apply_div");
                    pipe_pressure_apply.dispatch(cmd, ds_pressure_apply, wg_particle, 1, 1);
                }
                cs_barrier();
            }

            // Integrate non-pressure forces (gravity + viscosity).
            {
                auto _ = profiler.scope(cmd, "integrate_forces");
                pipe_integrate_forces.dispatch(cmd, ds_integrate_forces, wg_particle, 1, 1);
            }
            cs_barrier();

            // Density-constancy Jacobi inner loop.
            vkCmdFillBuffer(cmd, tier.pressure_a.handle(), 0, VK_WHOLE_SIZE, 0u);
            vkCmdFillBuffer(cmd, tier.pressure_b.handle(), 0, VK_WHOLE_SIZE, 0u);
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);
            {
                int iters = std::max(rt.minIterDensity, 1);
                for (int i = 0; i < iters; ++i) {
                    auto _ = profiler.scope(cmd, "density_solve");
                    pipe_density_solve.dispatch(cmd, ds_density_solve[i % 2], wg_particle, 1, 1);
                    cs_barrier();
                }
            }
            {
                auto _ = profiler.scope(cmd, "pressure_apply_density");
                pipe_pressure_apply.dispatch(cmd, ds_pressure_apply, wg_particle, 1, 1);
            }
            cs_barrier();

            // Re-pack uniform with mode=1 (POSITION_ONLY) and dispatch again.
            // Position-update advances x += dt*v with AABB clamp.
            pack_dfsph_uniform(substep_dt, 1.0f);
            {
                auto _ = profiler.scope(cmd, "integrate_position");
                pipe_integrate_forces.dispatch(cmd, ds_integrate_forces, wg_particle, 1, 1);
            }
            cs_barrier();

            if (!rt.paused) rt.iteration++;
        }

        // ----------------------------------------------------------------
        // Alembic export (§ 4.I).
        //
        // ParticleFrame requires interleaved x/y/z float arrays per attribute.
        // We readback the full particle SoA and unpack pos@offset 0 + vel@16.
        // ----------------------------------------------------------------
        if (abc::isAvailable() && alembic_writer && !rt.paused && rt.exportAlembic
            && rt.particleCount > 0
            && (rt.alembicFrameCounter % std::max(1, rt.alembicEveryNFrames)) == 0)
        {
            const std::uint32_t N = rt.particleCount;
            std::vector<std::uint8_t> bytes(std::size_t(N) * 128);
            tier.particles.readback(ctx, bytes.data(), bytes.size());

            std::vector<float> positions(std::size_t(N) * 3);
            std::vector<float> velocities(std::size_t(N) * 3);
            for (std::uint32_t i = 0; i < N; ++i) {
                const std::uint8_t* base = bytes.data() + std::size_t(i) * 128;
                std::memcpy(positions.data()  + i * 3, base + 0,  sizeof(float) * 3);
                std::memcpy(velocities.data() + i * 3, base + 16, sizeof(float) * 3);
            }
            abc::ParticleFrame fr{};
            fr.positions  = positions.data();
            fr.velocities = velocities.data();
            fr.count      = N;
            if (!alembic_writer->writeFrame(fr)) {
                logWarn("[sph-water] Alembic writeFrame returned false");
            }
        }
        if (!rt.paused) rt.alembicFrameCounter++;

        // ----------------------------------------------------------------
        // Screen-space fluid render (§ 4.G).
        //
        // Pass B (off-screen):   particle_sprite -> depth_image (R32_SFLOAT)
        // Pass A (compute):      bilateral_smooth × N (depth_image -> smoothed_a/b)
        // Pass C (off-screen):   thickness (additive) -> thickness_image
        // Passes D + E:          composite + ImGui -> swapchain (renderer.beginRendering)
        //
        // Off-screen passes call vkCmdBeginRendering directly per § 2.2
        // (Renderer::beginRendering only writes the swapchain image).
        // Convention 4 banking: see § 7.1 of the spec.
        // ----------------------------------------------------------------
        const VkExtent2D fb_extent = window.extent();

        VkViewport viewport{};
        viewport.x = 0.0f; viewport.y = 0.0f;
        viewport.width  = float(fb_extent.width);
        viewport.height = float(fb_extent.height);
        viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = fb_extent;

        // Depth pass.
        gv::Image::transitionLayout(cmd, tier.depth_image.handle(),
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        {
            VkRenderingAttachmentInfo depth_color{};
            depth_color.sType        = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depth_color.imageView    = tier.depth_image.view();
            depth_color.imageLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            depth_color.loadOp       = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_color.storeOp      = VK_ATTACHMENT_STORE_OP_STORE;
            depth_color.clearValue.color = {{1.0f, 1.0f, 1.0f, 1.0f}};  // 1.0 = "no fragment"

            VkRenderingInfo ri{};
            ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            ri.renderArea.extent = fb_extent;
            ri.layerCount        = 1;
            ri.colorAttachmentCount = 1;
            ri.pColorAttachments    = &depth_color;
            vkCmdBeginRendering(cmd, &ri);
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
            {
                auto _ = profiler.scope(cmd, "depth_pass");
                pipe_depth.bind(cmd, ds_depth);
                if (rt.particleCount > 0) {
                    vkCmdDraw(cmd, rt.particleCount, 1, 0, 0);
                }
            }
            vkCmdEndRendering(cmd);
        }

        gv::Image::transitionLayout(cmd, tier.depth_image.handle(),
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
        gv::Image::transitionLayout(cmd, tier.smoothed_depth_a.handle(),
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        gv::Image::transitionLayout(cmd, tier.smoothed_depth_b.handle(),
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        // Bilateral smooth × N iterations. Iter 0 reads depth_image (sampled)
        // and writes smoothed_a. Subsequent iters ping-pong smoothed_a ↔ b.
        // The composite descriptor needs to point at whichever buffer the
        // FINAL iteration wrote — track that as `final_smoothed_view`.
        VkImageView final_smoothed_view = tier.depth_image.view();
        VkImage     final_smoothed_img  = tier.depth_image.handle();
        if (rt.bilateralIterations > 0) {
            pack_bilateral_uniform(0);
            const std::uint32_t wgX = (fb_extent.width  + WG_DIM_BILATERAL - 1) / WG_DIM_BILATERAL;
            const std::uint32_t wgY = (fb_extent.height + WG_DIM_BILATERAL - 1) / WG_DIM_BILATERAL;
            for (int i = 0; i < rt.bilateralIterations; ++i) {
                VkDescriptorSet ds;
                VkImage out_img;
                VkImage in_img;
                VkImageView out_view;
                if (i == 0)              { ds = ds_bilateral[0]; in_img = tier.depth_image.handle();      out_img = tier.smoothed_depth_a.handle(); out_view = tier.smoothed_depth_a.view(); }
                else if (i % 2 == 1)     { ds = ds_bilateral[1]; in_img = tier.smoothed_depth_a.handle(); out_img = tier.smoothed_depth_b.handle(); out_view = tier.smoothed_depth_b.view(); }
                else                     { ds = ds_bilateral[2]; in_img = tier.smoothed_depth_b.handle(); out_img = tier.smoothed_depth_a.handle(); out_view = tier.smoothed_depth_a.view(); }
                (void)in_img;
                {
                    auto _ = profiler.scope(cmd, "bilateral_smooth");
                    pipe_bilateral_smooth.dispatch(cmd, ds, wgX, wgY, 1);
                }
                gv::memoryBarrier(cmd,
                    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
                final_smoothed_view = out_view;
                final_smoothed_img  = out_img;
            }
        }
        // Final smoothed buffer needs SHADER_READ_ONLY_OPTIMAL for composite.
        // depth_image is already GENERAL post-depth-pass; smoothed_a/b are GENERAL
        // throughout the bilateral ping-pong (Fix C: ES "everything in GENERAL").
        gv::Image::transitionLayout(cmd, final_smoothed_img,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // Select the composite descriptor whose pre-written depth-input view
        // matches `final_smoothed_img`. Pre-written at init: [0]=depth_image,
        // [1]=smoothed_a, [2]=smoothed_b.
        std::uint32_t composite_idx = 0;
        if (final_smoothed_img == tier.smoothed_depth_a.handle())      composite_idx = 1;
        else if (final_smoothed_img == tier.smoothed_depth_b.handle()) composite_idx = 2;
        (void)final_smoothed_view;

        // Thickness pass.
        gv::Image::transitionLayout(cmd, tier.thickness_image.handle(),
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        {
            VkRenderingAttachmentInfo thick_color{};
            thick_color.sType        = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            thick_color.imageView    = tier.thickness_image.view();
            thick_color.imageLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            thick_color.loadOp       = VK_ATTACHMENT_LOAD_OP_CLEAR;
            thick_color.storeOp      = VK_ATTACHMENT_STORE_OP_STORE;
            thick_color.clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

            VkRenderingInfo ri{};
            ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            ri.renderArea.extent = fb_extent;
            ri.layerCount        = 1;
            ri.colorAttachmentCount = 1;
            ri.pColorAttachments    = &thick_color;
            vkCmdBeginRendering(cmd, &ri);
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
            {
                auto _ = profiler.scope(cmd, "thickness_pass");
                pipe_thickness.bind(cmd, ds_thickness);
                if (rt.particleCount > 0) {
                    vkCmdDraw(cmd, rt.particleCount, 1, 0, 0);
                }
            }
            vkCmdEndRendering(cmd);
        }
        gv::Image::transitionLayout(cmd, tier.thickness_image.handle(),
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // ----------------------------------------------------------------
        // ImGui new frame + panel (build draw data; render below in pass D+E).
        // ----------------------------------------------------------------
        gpusims::ui::newImGuiFrame();
        if (ImGui::Begin("sph-water")) {
            ImGui::TextUnformatted("Phase 11 - DFSPH + screen-space fluid");
            ImGui::Separator();

            const char* preset_names[] = {"Dam-Break", "Central-Fountain", "Droplet-Impact", "Pour-from-Source"};
            int preset = rt.presetIndex;
            if (ImGui::Combo("Preset", &preset, preset_names, IM_ARRAYSIZE(preset_names))) {
                apply_preset(rt, preset);
                renderer.waitIdle();
                seedInitialFill();
            }

            const char* tier_names[] = {"256k", "1M (default)", "2M", "4M (capture-mode)"};
            int tier_idx = rt.pendingTierIndex;
            if (ImGui::Combo("Tier", &tier_idx, tier_names, IM_ARRAYSIZE(tier_names))) {
                rt.pendingTierIndex = tier_idx;
            }
            ImGui::SameLine();
            if (ImGui::Button("Apply##tier")) {
                tier_apply_clicked = true;
            }

            ImGui::Separator();
            ImGui::Text("Particles: %u / %u", rt.particleCount, rt.particleCapacity);
            ImGui::Text("Iteration: %llu", static_cast<unsigned long long>(rt.iteration));
            ImGui::Text("Emitters: %zu", rt.emitters.size());
            ImGui::Checkbox("Paused", &rt.paused);
            ImGui::SliderInt("Substeps", &rt.substeps, 1, 8);

            ImGui::Separator();
            if (ImGui::CollapsingHeader("Solver")) {
                ImGui::SliderInt("min iter (density)",     &rt.minIterDensity,     1, 16);
                ImGui::SliderInt("min iter (divergence)",  &rt.minIterDivergence,  1, 16);
                ImGui::SliderFloat("viscosity",            &rt.viscosity,          0.0f, 0.1f, "%.4f");
                ImGui::SliderFloat("cohesion",             &rt.cohesion,           0.0f, 0.5f, "%.3f");
                ImGui::SliderFloat("vorticity",            &rt.vorticityStrength,  0.0f, 3.0f, "%.2f");
                ImGui::Checkbox("divergence solver",       &rt.divSolverEnabled);
            }
            if (ImGui::CollapsingHeader("Render")) {
                ImGui::SliderInt("bilateral iterations",   &rt.bilateralIterations, 0, 16);
                ImGui::SliderFloat("sigma spatial (px)",   &rt.bilateralSigmaSpatial, 0.5f, 16.0f);
                ImGui::SliderFloat("sigma depth (NDC)",    &rt.bilateralSigmaDepth,   0.001f, 0.5f, "%.4f");
                ImGui::SliderFloat("thickness/particle",   &rt.thicknessPerParticle,  0.0f, 0.05f, "%.4f");
                ImGui::ColorEdit3("water tint",            glm::value_ptr(rt.waterTint));
                ImGui::ColorEdit3("sky zenith",            glm::value_ptr(rt.skyZenith));
                ImGui::ColorEdit3("sky horizon",           glm::value_ptr(rt.skyHorizon));
                ImGui::SliderFloat("exposure",             &rt.exposure, 0.1f, 5.0f);
            }
            if (ImGui::CollapsingHeader("Alembic export")) {
                if (abc::isAvailable()) {
                    ImGui::Checkbox("export ABC",          &rt.exportAlembic);
                    ImGui::SliderInt("export every N frames", &rt.alembicEveryNFrames, 1, 60);
                } else {
                    ImGui::TextDisabled("Alembic not built (stub mode).");
                }
            }
            if (ImGui::CollapsingHeader("Camera")) {
                camera.drawImGui("Camera");
            }
            ImGui::Separator();
            profiler.drawImGui("GPU Profiler");
            ImGui::Checkbox("ImGui demo", &show_demo);
        }
        ImGui::End();
        if (show_demo) ImGui::ShowDemoWindow(&show_demo);
        gpusims::ui::drawToasts();

        // ----------------------------------------------------------------
        // Pass D + E: composite (fullscreen triangle) + ImGui to swapchain.
        // ----------------------------------------------------------------
        renderer.beginRendering(*frame, {{0.05f, 0.08f, 0.10f, 1.0f}});
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        {
            auto _ = profiler.scope(cmd, "composite");
            pipe_composite.bind(cmd, ds_composite[composite_idx]);
            vkCmdDraw(cmd, 3, 1, 0, 0);   // fullscreen triangle
        }
        gpusims::ui::renderImGui(cmd);
        renderer.endRendering(*frame);

        profiler.endFrame(cmd);
        renderer.endFrame(*frame);

        // ----------------------------------------------------------------
        // Tier-change apply (§ 4.J). End-of-frame so no recorded command
        // buffer references buffers about to be destroyed.
        // ----------------------------------------------------------------
        if (tier_apply_clicked && rt.pendingTierIndex != rt.tierIndex) {
            renderer.waitIdle();
            destroyTierResources(ctx, tier);
            rt.tierIndex        = rt.pendingTierIndex;
            rt.particleCapacity = TIER_PARTICLE_COUNTS[rt.tierIndex];
            tier = createTierResources(ctx, window, rt.particleCapacity, MAX_CELLS);
            rewriteAllDescriptors();
            seedInitialFill();
            rt.iteration = 0;
            gpusims::ui::pushToast(
                ("Tier changed to " + std::to_string(rt.particleCapacity) + " particles").c_str(),
                true);
        }
        tier_apply_clicked = false;

        // Esc to quit.
        if (glfwGetKey(window.glfwWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window.glfwWindow(), GLFW_TRUE);
        }
    }

    renderer.waitIdle();
    destroyTierResources(ctx, tier);
    gpusims::ui::shutdownImGui();
    return 0;
}
