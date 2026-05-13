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
    // SPEC-NOTE (Phase 11 § 2.4 + § 4.B.8): the spec calls for
    //   `VkPhysicalDeviceVulkan13Features::subgroupSizeControl = VK_TRUE` and
    //   `computeFullSubgroups = VK_TRUE` to be set at Context creation, and
    //   for `VkPipelineShaderStageRequiredSubgroupSizeCreateInfo` to pin each
    //   sort/bilateral pipeline at creation. The synced common-cpp
    //   `ContextCreateInfo` surface has no subgroup-size-control fields; the
    //   spec's hard-rule-5 authorization for in-flight common-cpp additions
    //   has NOT been applied here. Phase 11 follow-up: add the Context
    //   subgroup-size-control surface and the ComputePipelineDesc
    //   required-subgroup-size field, then re-wire below.
    // ----------------------------------------------------------------------
    gv::Context ctx;
    gv::Window  window(ctx, 1920, 1080, "GPU-Sims · SPH Water");
    gv::Renderer renderer(ctx, window);
    gv::ShaderCompiler compiler(ctx);

    // Subgroup-size query placeholder. Without the in-flight common-cpp
    // surface additions, we can still inspect VkPhysicalDeviceSubgroupProperties
    // directly here (skipped in this scaffold; Phase 11 follow-up).
    logInfo("sph-water scaffold: subgroup-size-control pipeline pinning NOT YET applied (see § 2.4)");

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
    (void)profiler;
    (void)reloader;

    // ------------------------------------------------------------------------
    // SCAFFOLD: per-tier buffer allocation, pipeline creation, dispatch chain,
    // screen-space fluid render passes, F5/F9 capture/load — all deferred.
    // See the file-header status block for the full list of deferred sections.
    //
    // The shader files in shaders/ are full implementations; the host-side
    // wiring lands in the Phase 11 follow-up commit after the common-cpp
    // surface additions (Context::subgroup-size-control, ComputePipelineDesc
    // required-subgroup-size) per hard rule 5.
    // ------------------------------------------------------------------------

    bool show_demo = false;
    auto last_time = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window.glfwWindow())) {
        glfwPollEvents();

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

        // F5 / F9 rising edges (scaffold: detect, log; full capture/load deferred).
        static bool was_f5 = false, was_f9 = false;
        bool now_f5 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F5) == GLFW_PRESS;
        bool now_f9 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F9) == GLFW_PRESS;
        if (now_f5 && !was_f5) {
            logInfo("F5: capture trigger (scaffold; full implementation deferred per § 4.B.15)");
        }
        if (now_f9 && !was_f9) {
            logInfo("F9: reload trigger (scaffold; full implementation deferred per § 4.B.15)");
        }
        was_f5 = now_f5;
        was_f9 = now_f9;

        // LMB click-to-place emitter (scaffold: compute paint-plane intersection).
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

        // ImGui new frame + panel.
        gpusims::ui::newImGuiFrame();
        if (ImGui::Begin("sph-water")) {
            ImGui::TextUnformatted("Phase 11 scaffold — dispatch chain + render passes deferred");
            ImGui::Separator();

            // Preset dropdown.
            const char* preset_names[] = {"Dam-Break", "Central-Fountain", "Droplet-Impact", "Pour-from-Source"};
            int preset = rt.presetIndex;
            if (ImGui::Combo("Preset", &preset, preset_names, IM_ARRAYSIZE(preset_names))) {
                apply_preset(rt, preset);
            }

            // Tier dropdown.
            const char* tier_names[] = {"256k", "1M (default)", "2M", "4M (capture-mode)"};
            int tier = rt.tierIndex;
            if (ImGui::Combo("Tier", &tier, tier_names, IM_ARRAYSIZE(tier_names))) {
                rt.pendingTierIndex = tier;
            }

            ImGui::Separator();
            ImGui::Text("Particles: %u / %u", rt.particleCount, rt.particleCapacity);
            ImGui::Text("Iteration: %llu", static_cast<unsigned long long>(rt.iteration));
            ImGui::Text("Emitters: %zu", rt.emitters.size());

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
            ImGui::Checkbox("ImGui demo", &show_demo);
        }
        ImGui::End();
        if (show_demo) ImGui::ShowDemoWindow(&show_demo);

        // ----------------------------------------------------------------
        // Render — scaffold draws clear-color only; full screen-space fluid
        // render passes (§ 4.B.12) are deferred. The 5-pass pipeline
        // (depth -> bilateral × N -> thickness -> composite -> swapchain)
        // requires direct vkCmdBeginRenderingKHR for the off-screen passes,
        // plus the pipelines created via the deferred § 4.B.10 logic.
        // ----------------------------------------------------------------
        renderer.beginRendering(*frame, {{0.05f, 0.08f, 0.10f, 1.0f}});
        gpusims::ui::renderImGui(frame->command_buffer);
        renderer.endRendering(*frame);
        renderer.endFrame(*frame);

        if (!rt.paused) {
            rt.iteration++;
        }

        // Esc to quit.
        if (glfwGetKey(window.glfwWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window.glfwWindow(), GLFW_TRUE);
        }
    }

    renderer.waitIdle();
    gpusims::ui::shutdownImGui();
    return 0;
}
