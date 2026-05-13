// =============================================================================
// gpu_sims_hello — common-cpp Phase 1 reference application
// =============================================================================

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <stdexcept>

#include <GLFW/glfw3.h>
#include <imgui.h>

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
#include <gpusims/vk/graphics_pipeline.hpp>
#include <gpusims/vk/image.hpp>
#include <gpusims/vk/renderer.hpp>
#include <gpusims/vk/shader_compiler.hpp>
#include <gpusims/vk/window.hpp>

namespace gv = gpusims::vk;

// -----------------------------------------------------------------------------
// GLFW input wrapper — accumulates per-frame input deltas into one struct.
// -----------------------------------------------------------------------------
struct Input {
    double last_x = 0.0, last_y = 0.0;
    double scroll = 0.0;
    bool   first  = true;

    gpusims::CameraInputState snapshotAndReset(GLFWwindow* w) {
        gpusims::CameraInputState s;
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
        if (first) {
            last_x = x; last_y = y; first = false;
        }
        s.mouse_dx = static_cast<float>(x - last_x);
        s.mouse_dy = static_cast<float>(y - last_y);
        last_x = x; last_y = y;

        s.scroll_dy = static_cast<float>(scroll);
        scroll      = 0.0;

        // ImGui consumes input when it has focus; suppress camera input
        // so dragging in the inspector doesn't move the camera.
        if (ImGui::GetIO().WantCaptureMouse) {
            s.mouse_left = s.mouse_right = s.mouse_middle = false;
            s.mouse_dx   = s.mouse_dy = 0.0f;
            s.scroll_dy  = 0.0f;
        }
        if (ImGui::GetIO().WantCaptureKeyboard) {
            s.key_w = s.key_a = s.key_s = s.key_d = s.key_q = s.key_e = false;
        }
        return s;
    }
};
static Input g_input;

static void scrollCallback(GLFWwindow*, double, double dy) {
    g_input.scroll += dy;
}

// -----------------------------------------------------------------------------
// Hot-reload helper. We poll once per frame on the main thread.
// -----------------------------------------------------------------------------
struct ReloadFlags {
    bool compute  = false;
    bool graphics = false;
};

// -----------------------------------------------------------------------------
// Update descriptor sets. Called on creation and after any swapchain/image
// recreate (none in hello-world, but the pattern is reusable).
// -----------------------------------------------------------------------------
static void writeComputeDescriptor(VkDevice ds_device,
                                   VkDescriptorSet ds,
                                   VkImageView     image_view) {
    VkDescriptorImageInfo info{};
    info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    info.imageView   = image_view;
    info.sampler     = VK_NULL_HANDLE;

    VkWriteDescriptorSet w{};
    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet          = ds;
    w.dstBinding      = 0;
    w.descriptorCount = 1;
    w.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w.pImageInfo      = &info;
    vkUpdateDescriptorSets(ds_device, 1, &w, 0, nullptr);
}

static void writeGraphicsDescriptor(VkDevice device,
                                    VkDescriptorSet ds,
                                    VkImageView     image_view,
                                    VkSampler       sampler) {
    VkDescriptorImageInfo info{};
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.imageView   = image_view;
    info.sampler     = sampler;

    VkWriteDescriptorSet w{};
    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet          = ds;
    w.dstBinding      = 0;
    w.descriptorCount = 1;
    w.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.pImageInfo      = &info;
    vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
}

// -----------------------------------------------------------------------------
// Push-constant layout shared with gradient.comp.glsl.
// -----------------------------------------------------------------------------
struct GradientPush {
    float resolution[2];
    float time;
    float pad;
};

// =============================================================================
// Phase 11 sph-water smoke test for subgroup-size-control surface.
//
// Exercises the new common-cpp APIs added by the Phase 11 follow-up:
//   - ContextCreateInfo::enable_subgroup_size_control
//   - Context::subgroupSizeMin/Max/requiredSubgroupSizeStages/...Enabled
//   - ComputePipelineDesc::required_subgroup_size / require_full_subgroups
//
// Ships with the surface-addition commit so the new API has proof-of-life
// independent of the Turn 4 main.cpp consumer code.
// =============================================================================
static int runSubgroupSizeControlSmokeTest() {
    using namespace gpusims;
    logInfo("hello: subgroup-size-control smoke test starting");

    gv::ContextCreateInfo cdesc{};
    cdesc.application_name              = "gpu_sims_hello_subgroup";
    cdesc.enable_subgroup_size_control  = true;
    gv::Context ctx(cdesc);

    logInfo("hello: subgroup-size min={} max={} stages=0x{:x} enabled={}",
            ctx.subgroupSizeMin(),
            ctx.subgroupSizeMax(),
            ctx.requiredSubgroupSizeStages(),
            ctx.subgroupSizeControlEnabled());

    // Sanity checks.
    if (ctx.subgroupSizeMin() == 0u ||
        ctx.subgroupSizeMax() < ctx.subgroupSizeMin() ||
        !ctx.subgroupSizeControlEnabled()) {
        logError("hello: subgroup-size sanity check failed");
        return 1;
    }

    // Build a trivial compute pipeline with required_subgroup_size pinned.
    // The shader uses local_size_x = 32 so we pick a required size in
    // [min, max] that's also a multiple of/compatible with the kernel's
    // local-size (clamp the chosen value into the device's reported range).
    gv::ShaderCompiler compiler(ctx);
    const std::filesystem::path shader_dir = GPU_SIMS_HELLO_SHADER_DIR;
    compiler.addIncludeDir(shader_dir);

    gv::ComputePipelineDesc pdesc;
    pdesc.shader_path = shader_dir / "trivial.comp.glsl";
    pdesc.bindings.push_back({0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT});
    pdesc.push_constant_size = 0;
    pdesc.required_subgroup_size = std::clamp(
        std::max(32u, ctx.subgroupSizeMin()),
        ctx.subgroupSizeMin(),
        ctx.subgroupSizeMax());
    pdesc.require_full_subgroups = true;

    try {
        auto pipe = gv::ComputePipeline::create(ctx, compiler, pdesc);
        logInfo("hello: compute pipeline with required_subgroup_size={} created OK",
                pdesc.required_subgroup_size);
        (void)pipe;  // destruct at scope end — exercises the destructor path too
    } catch (const std::exception& e) {
        logError("hello: pipeline creation with required_subgroup_size={} failed: {}",
                 pdesc.required_subgroup_size, e.what());
        return 1;
    }

    logInfo("hello: subgroup-size-control smoke test PASSED");
    return 0;
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
int main(int argc, char** argv) {
    using namespace gpusims;
    initLogger();

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--test-subgroup-size") == 0) {
            return runSubgroupSizeControlSmokeTest();
        }
    }

    logInfo("hello: starting up");

    // ---- Vulkan + window + renderer -----------------------------------------
    gv::Context  ctx;
    gv::Window   window(ctx, 1280, 720, "gpu_sims_hello");
    gv::Renderer renderer(ctx, window);

    glfwSetScrollCallback(window.glfwWindow(), scrollCallback);

    // ---- Shader compiler + hot-reload --------------------------------------
    gv::ShaderCompiler compiler(ctx);
    const std::filesystem::path shader_dir = GPU_SIMS_HELLO_SHADER_DIR;
    compiler.addIncludeDir(shader_dir);

    HotReloader hot;

    // ---- Compute target image (1280x720 RGBA8, storage + sampled) ----------
    gv::ImageCreateInfo img_ci{};
    img_ci.type           = gv::ImageType::e2D;
    img_ci.extent         = { 1280u, 720u, 1u };
    img_ci.format         = VK_FORMAT_R8G8B8A8_UNORM;
    img_ci.usage          = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    img_ci.initial_layout = VK_IMAGE_LAYOUT_GENERAL;
    img_ci.debug_name     = "hello.gradient_image";
    auto gradient_image = gv::Image::create(ctx, img_ci);

    // Sampler for the graphics pipeline to read it.
    VkSampler sampler = VK_NULL_HANDLE;
    {
        VkSamplerCreateInfo sci{};
        sci.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sci.magFilter    = VK_FILTER_LINEAR;
        sci.minFilter    = VK_FILTER_LINEAR;
        sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.maxLod       = 1.0f;
        if (vkCreateSampler(ctx.device(), &sci, nullptr, &sampler) != VK_SUCCESS) {
            logError("hello: vkCreateSampler failed");
            return 1;
        }
    }

    // ---- Compute pipeline (gradient.comp) ----------------------------------
    gv::ComputePipelineDesc cdesc;
    cdesc.shader_path = shader_dir / "gradient.comp.glsl";
    cdesc.bindings.push_back({0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT});
    cdesc.push_constant_size = sizeof(GradientPush);
    auto compute_pipe = gv::ComputePipeline::create(ctx, compiler, cdesc);

    VkDescriptorSet compute_ds = compute_pipe.allocateDescriptorSet();
    writeComputeDescriptor(ctx.device(), compute_ds, gradient_image.view());

    // ---- Graphics pipeline (fullscreen.vert + .frag) ------------------------
    gv::GraphicsPipelineDesc gdesc;
    gdesc.vertex_shader_path   = shader_dir / "fullscreen.vert.glsl";
    gdesc.fragment_shader_path = shader_dir / "fullscreen.frag.glsl";
    gdesc.color_formats        = { window.colorFormat() };
    gdesc.bindings.push_back({0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT});
    auto graphics_pipe = gv::GraphicsPipeline::create(ctx, compiler, gdesc);

    VkDescriptorSet graphics_ds = graphics_pipe.allocateDescriptorSet();
    writeGraphicsDescriptor(ctx.device(), graphics_ds, gradient_image.view(), sampler);

    // ---- Hot-reload watches (recompile compute / graphics on edit) ---------
    ReloadFlags reload_flags;
    auto register_compute_watch = [&]() {
        hot.watch(cdesc.shader_path, [&](const std::filesystem::path&) {
            reload_flags.compute = true;
        });
    };
    auto register_graphics_watch = [&]() {
        hot.watch(gdesc.vertex_shader_path,   [&](const std::filesystem::path&) {
            reload_flags.graphics = true;
        });
        hot.watch(gdesc.fragment_shader_path, [&](const std::filesystem::path&) {
            reload_flags.graphics = true;
        });
    };
    register_compute_watch();
    register_graphics_watch();

    // ---- Camera, profiler, state I/O --------------------------------------
    Camera camera;
    camera.setMode(Camera::Mode::FreeFly);
    camera.setAspect(window.aspect());

    GpuProfiler profiler(ctx);

    StateWriter state_writer(std::filesystem::current_path() / "captures");
    std::uint32_t next_capture = 0;

    // ---- ImGui --------------------------------------------------------------
    {
        ui::ImGuiInit ic{};
        ic.glfw_window     = window.glfwWindow();
        ic.instance        = ctx.instance();
        ic.physical_device = ctx.physicalDevice();
        ic.device          = ctx.device();
        ic.queue_family    = ctx.graphicsQueueFamily();
        ic.queue           = ctx.graphicsQueue();
        ic.descriptor_pool = VK_NULL_HANDLE;     // common-cpp creates one
        ic.color_format    = window.colorFormat();
        ic.min_image_count = 2;
        ic.image_count     = window.imageCount();
        if (!ui::initImGui(ic)) {
            logError("hello: ui::initImGui failed");
            return 1;
        }
    }

    // ---- Main loop ---------------------------------------------------------
    auto last = std::chrono::steady_clock::now();
    float t   = 0.0f;
    bool  prev_f5 = false, prev_f9 = false;

    while (!window.shouldClose()) {
        window.pollEvents();
        hot.poll();

        // Compute dt
        const auto  now = std::chrono::steady_clock::now();
        const float dt  = std::chrono::duration<float>(now - last).count();
        last = now;
        t   += dt;

        // F5 / F9 (rising-edge detection so each press triggers once)
        const bool f5 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F5) == GLFW_PRESS;
        const bool f9 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F9) == GLFW_PRESS;
        const bool save_now = f5 && !prev_f5;
        const bool load_now = f9 && !prev_f9;
        prev_f5 = f5; prev_f9 = f9;

        if (save_now) {
            state_writer.beginFrame(next_capture);
            nlohmann::json cam_j; camera.toJson(cam_j);
            state_writer.setMeta("camera", cam_j);
            state_writer.setMeta("frame_time_ms", dt * 1000.0f);
            const std::uint32_t status = next_capture;
            state_writer.saveBuffer("status", &status, sizeof(status),
                                    {{"description", "frame counter at save time"}});
            state_writer.endFrame();
            ui::pushToast(("Saved capture_" + std::to_string(next_capture)).c_str(), true);
            ++next_capture;
        }

        if (load_now && next_capture > 0) {
            const auto path = std::filesystem::current_path() / "captures" /
                              ("capture_" + std::string(4 - std::min<std::size_t>(4, std::to_string(next_capture - 1).size()), '0') + std::to_string(next_capture - 1));
            if (auto cap = StateReader::open(path); cap) {
                auto cam_j = cap->meta("camera");
                if (!cam_j.is_null()) camera.fromJson(cam_j);
                ui::pushToast(("Loaded " + path.filename().string()).c_str(), true);
            } else {
                ui::pushToast("Load failed: see log", false);
            }
        }

        // Camera update
        camera.setAspect(window.aspect());
        camera.update(dt, g_input.snapshotAndReset(window.glfwWindow()));

        // ---- Begin frame --------------------------------------------------
        gv::Frame* frame = renderer.beginFrame();
        if (!frame) continue;  // swapchain recreated; skip

        // Apply pending hot-reloads, deferring deletion to current frame.
        if (reload_flags.compute) {
            std::string err;
            if (compute_pipe.reload(ctx, compiler, *frame, &err)) {
                hot.reportSuccess(cdesc.shader_path);
                ui::pushToast("compute reloaded", true);
                writeComputeDescriptor(ctx.device(), compute_ds, gradient_image.view());
            } else {
                hot.reportFailure(cdesc.shader_path, err);
                ui::pushToast(("compute reload failed: " + err).substr(0, 256).c_str(), false, 6.0f);
            }
            reload_flags.compute = false;
        }
        if (reload_flags.graphics) {
            std::string err;
            if (graphics_pipe.reload(ctx, compiler, *frame, &err)) {
                hot.reportSuccess(gdesc.fragment_shader_path);
                ui::pushToast("graphics reloaded", true);
                writeGraphicsDescriptor(ctx.device(), graphics_ds, gradient_image.view(), sampler);
            } else {
                hot.reportFailure(gdesc.fragment_shader_path, err);
                ui::pushToast(("graphics reload failed: " + err).substr(0, 256).c_str(), false, 6.0f);
            }
            reload_flags.graphics = false;
        }

        VkCommandBuffer cmd = frame->command_buffer;
        profiler.beginFrame(cmd, frame->in_flight_index);

        // ---- ImGui overlay ----
        ui::newImGuiFrame();

        ImGui::SetNextWindowPos(ImVec2(10.0f, 440.0f),   ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320.0f, 180.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Hello")) {
            ImGui::Text("F5 = save state, F9 = load most recent");
            ImGui::Text("Captures saved: %u", next_capture);
            ImGui::Separator();
            ImGui::Text("Hot-reload: %zu file(s) watched", hot.watchCount());
            ImGui::Text("Edit any shader in:");
            ImGui::TextWrapped("%s", shader_dir.string().c_str());
        }
        ImGui::End();

        camera.drawImGui();
        profiler.drawImGui();
        ui::drawToasts();

        // ---- Compute pass: write gradient ----
        {
            auto _scope = profiler.scope(cmd, "compute_gradient");

            GradientPush push{};
            push.resolution[0] = static_cast<float>(gradient_image.extent().width);
            push.resolution[1] = static_cast<float>(gradient_image.extent().height);
            push.time          = t;
            push.pad           = 0.0f;

            const std::uint32_t gx = (gradient_image.extent().width  + 15u) / 16u;
            const std::uint32_t gy = (gradient_image.extent().height + 15u) / 16u;
            compute_pipe.dispatch(cmd, compute_ds, gx, gy, 1, &push, sizeof(push));
        }

        // Barrier: COMPUTE write → FRAGMENT read; layout GENERAL → SHADER_READ_ONLY.
        gv::Image::transitionLayout(cmd, gradient_image.handle(),
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    VK_IMAGE_LAYOUT_GENERAL,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // ---- Render pass: fullscreen sample + ImGui ----
        {
            auto _scope = profiler.scope(cmd, "render");
            renderer.beginRendering(*frame);

            graphics_pipe.bind(cmd, graphics_ds);
            vkCmdDraw(cmd, 3, 1, 0, 0);

            ui::renderImGui(cmd);

            renderer.endRendering(*frame);
        }

        // Restore image to GENERAL for next compute write.
        gv::Image::transitionLayout(cmd, gradient_image.handle(),
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    VK_IMAGE_LAYOUT_GENERAL);

        profiler.endFrame(cmd);
        renderer.endFrame(*frame);
    }

    // ---- Shutdown ----------------------------------------------------------
    renderer.waitIdle();
    if (sampler != VK_NULL_HANDLE) vkDestroySampler(ctx.device(), sampler, nullptr);
    ui::shutdownImGui();
    logInfo("hello: clean shutdown");
    return 0;
}
