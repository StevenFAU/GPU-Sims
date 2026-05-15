---
title: Phase 12 LBM pre-spec probe — repo state for architect drafting
date: 2026-05-15
author: architect1
phase: 12
status: probe (read-only)
scope: ground-truth state for Phase 12 lattice-boltzmann spec drafting — repo HEAD, Phase 11.5 collision surface, common-cpp public API, eulerian-smoke precedent, references vendoring posture, capture-format contract, build system, SDF/airfoil sanity check
---

> Read-only probe. No source modified, no commits, no builds. Verbatim quotes
> include file:line citations. Section E was partially blocked: web access
> (`WebSearch` / `WebFetch`) is denied in this environment, so the LBM-library
> survey is delivered as a recap from priors + an explicit "needs human
> lookup" call-out rather than current release / line-count facts.

---

## A — Repo HEAD state

### `git log -1 --format='%H %ci %s'` on `main`

```
447ebf00bac4c8b46c00441c142496405078b18e 2026-05-14 21:34:10 -0400 fix(integrity): grandfather retrospective grammar examples + sweep retro doc
```

### `git status --short` (working tree)

```
 M common/common-cpp/include/gpusims/alembic_writer.hpp
 M common/common-cpp/include/gpusims/vdb_writer.hpp
A  docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md
A  docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md
A  docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md
A  docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md
 M particle-fluids/sph-water/README.md
 M particle-fluids/sph-water/docs/load-bearing-decisions.md
 M particle-fluids/sph-water/docs/notes.md
 M particle-fluids/sph-water/shaders/apply_velocity.comp.glsl
 M particle-fluids/sph-water/shaders/compute_aij_pj.comp.glsl
 M particle-fluids/sph-water/shaders/compute_density_adv.comp.glsl
 M particle-fluids/sph-water/shaders/compute_density_change.comp.glsl
 M particle-fluids/sph-water/shaders/compute_pressure_accel.comp.glsl
 M particle-fluids/sph-water/shaders/density_alpha.comp.glsl
 M particle-fluids/sph-water/shaders/jacobi_update_density.comp.glsl
 M particle-fluids/sph-water/shaders/jacobi_update_divergence.comp.glsl
 M particle-fluids/sph-water/src/main.cpp
 M tools/integrity/docs/grandfather-catalog.md
 M tools/integrity/integrity/cat2_contracts/checks/__init__.py
A  tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py
 M tools/integrity/integrity/grandfather.py
A  tools/integrity/tests/fixtures/bad_stub_label/common/common-cpp/include/widget/widget.hpp
A  tools/integrity/tests/fixtures/bad_stub_label/common/common-cpp/src/widget.cpp
A  tools/integrity/tests/fixtures/bad_stub_label/common/common-py/gpusims_common/widget.py
A  tools/integrity/tests/fixtures/good_stub_label/common/common-cpp/include/widget/widget.hpp
A  tools/integrity/tests/fixtures/good_stub_label/common/common-cpp/src/widget.cpp
A  tools/integrity/tests/fixtures/good_stub_label/common/common-py/gpusims_common/permanent.py
A  tools/integrity/tests/test_cat2_stub_label_stale.py
?? docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md
```

The working tree is dirty along two seams: (1) sph-water Phase 11.5 in-progress
edits (sph-water shaders, main.cpp, docs) and (2) integrity-toolkit v1.1
batch-1 work (stub-label-stale Cat 2 check + fixtures, modifications to
`vdb_writer.hpp` / `alembic_writer.hpp` headers, grandfather catalog updates).
Neither has been committed.

### Commits after `0243278` (Phase 11 substantive anchor in `project-state.md` § 11)

`project-state.md`:858 still records:

```
- Latest commit: ``0243278`` — Phase 11 sph-water sphere of substantive work fully landed (...).
```

The `git log 0243278..HEAD` set contains **40 commits** between `0243278`
(2026-05-13 17:14) and the HEAD `447ebf0` (2026-05-14 21:34). None of these
are reflected in § 11 "Latest commit". One-line summaries (chronological,
oldest first):

```
97b8888 chore(phase11): backfill PHASE_11_SHA placeholder
c478cc5 docs(phase11): retro tracking doc — fabrication taxonomy + banking items
577e0c1 fix(markdown): MD029 ol-prefix in phase11 fabrication-shapes list
596550d fix(phase11): Alembic FetchContent build under strict project warnings
28927ca fix(phase11): depth pass writes to color, not gl_FragDepth
be924ef fix(phase11): integrate_forces UBO layout + missing position pass
83a01d6 Phase 11 (sph-water): canonical DFSPH UBO layout
7294ee4 Phase 11 (sph-water): integrate_forces mode via push constant
fc1d63a docs(retro/phase11): bank fabrication-shape categories 8 and 9
2b53045 feat(sph-water): add DFSPH solver restructure surface (commit 2a)
4adc84a feat(sph-water): rewrite DFSPH substep dispatch chain (commit 2b)
e1fec89 docs(diagnostics): land Layer 3 Batch B lenia audit reports
fa95a6e docs(diagnostics): land Phase 11.5 Layer 1 audit corpus
c3c95aa docs(diagnostics): land Layer 2 common-cpp structural audit
00787e5 docs(diagnostics): land Layer 3 prioritization triage
0008207 fix(common-cpp): land pre-existing working-tree fixes surfaced by Layer 2 audit
56ac393 docs(integrity): land v1 spec for the cross-stack integrity toolkit
51bd8d0 feat(integrity): scaffold toolkit package + runner (commit 1)
980b374 docs(integrity): land commit 1 audit report
0822f6a feat(integrity): Cat 1 citation parsing + intra-repo resolution (commit 2)
3d35e4e docs(integrity): land commit 2 audit report
6c40bda feat(integrity): Cat 1 upstream-citation + anchor verification + unregistered-upstream (commit 3)
0d19808 docs(integrity): land commit 3 audit report
641dc7a docs(integrity): correct § 12 check mappings + generalize cat2 field-read
8af5672 feat(integrity): grandfather-sweep pre-v1 findings (commit 4a)
f79c00e docs(integrity): land commit 4a audit report
f7e012d feat(integrity): add CI workflow (commit 4b)
1e886e6 fix(integrity): add Vulkan + windowing deps to CI workflow
ab39303 docs(integrity): land commit 4b audit report
96de0c3 feat(integrity): Cat 2 Stack D contract verification (commit 5)
b5b0310 docs(integrity): land commit 5 audit report
5a1c193 feat(integrity): Cat 2 Stack C contract verification (commit 6)
b0f7bce fix(integrity): simplify Stack C fixtures to not require stdc++ headers
d546304 fix(integrity): force-track fixture compile_commands.json + land commit 6 audit
cc9e8c5 feat(integrity): Cat 2 Stack B contract verification (commit 7)
203b14b fix(integrity): add @types/node to TS helper devDependencies
fc20ef7 fix(integrity): install root workspace deps in CI for Stack B + skip suppressed in github output
f576b5e feat(integrity): Cat 3 cubic-kernel numerical correctness (commit 8 — final)
bbc38f0 docs(integrity): land commit 8 audit report + grandfather post-commit-8 findings
fe7f38c docs(retro): land integrity toolkit v1 retrospective
447ebf0 fix(integrity): grandfather retrospective grammar examples + sweep retro doc
```

Two major trajectories represented here:

1. **Phase 11 polish + Phase 11.5 (DFSPH restructure)** — commits up through
   `4adc84a` (commit 2b). Touches sph-water + common-cpp. The substantive
   anchor referenced by § 11 (`0243278`) is now ~40 commits behind HEAD.
2. **Integrity toolkit v1** — commits `56ac393`..`447ebf0` (~30 commits).
   Brand-new `tools/integrity/` package; CI workflow extension; sweep audits
   landed under `docs/diagnostics/_audits/`. **Not a sim. Lives in
   `tools/integrity/`. Has its own CI surface (`.github/workflows/integrity-toolkit.yml`
   per commit `f7e012d`).** Does not block Phase 12 work; does mean § 11's
   "Latest commit" line is well behind the actual repo.

There is also additional uncommitted v1.1 work for the integrity toolkit
sitting in the working tree (see § A's status output, the `integrity_v1_1_*`
audit-doc additions and the `stub_label_stale` Cat 2 check files).

---

## B — Phase 11.5 active touch zones

### Commits since 2026-05-13 touching `common/common-cpp/`, `volumetric-grid/`, `docs/`, `references/`

From `git log --since='2026-05-13' --name-only --pretty=format:'%h %s'` filtered
to those paths (`references/` returns no commits — it is gitignored, see § E):

**common/common-cpp/ touches (4 commits):**

- `9e0ca2f feat(common-cpp): add subgroup-size-control surface for Stack C consumers` — touches
  `include/gpusims/vk/compute_pipeline.hpp`, `include/gpusims/vk/context.hpp`,
  `src/vk/compute_pipeline.cpp`, `src/vk/context.cpp`, plus
  `examples/hello/main.cpp`, `examples/hello/shaders/trivial.comp.glsl`.
- `1f02fc1 feat(phase11): sph-water DFSPH dispatch chain + screen-space fluid render` — touches
  `include/gpusims/vk/buffer.hpp`, `include/gpusims/vk/graphics_pipeline.hpp`,
  `src/vk/buffer.cpp`, `src/vk/graphics_pipeline.cpp`.
- `596550d fix(phase11): Alembic FetchContent build under strict project warnings` — touches
  `cmake/optional_deps.cmake`, `src/alembic_writer.cpp`.
- `0008207 fix(common-cpp): land pre-existing working-tree fixes surfaced by Layer 2 audit` — touches
  `include/gpusims/gpu_profiler.hpp`, `src/gpu_profiler.cpp`, `src/vk/context.cpp`.
- `5a1c193 feat(integrity): Cat 2 Stack C contract verification (commit 6)` — touches a broad
  read-only set of `include/gpusims/**.hpp` (writer/reader/vk/* + `alembic_writer.hpp`,
  `vdb_writer.hpp`, `state_writer.hpp`, `state_reader.hpp`, etc.) — these were
  edits to add `integrity-allow` allow-list comments, **not** API changes.

**volumetric-grid/ touches:** none in the Phase 11.5 commit set.

**docs/ touches:** large — Phase 11.5 audit corpus under
`docs/diagnostics/_audits/` (`phase11_5_*`, `commoncpp_*`), `docs/retro/phase11.md`,
integrity-toolkit build/landing reports, `docs/integrity-toolkit-spec.md`,
`docs/sim-specs/lenia-fft.md`. No edits to `docs/sim-specs/lattice-boltzmann.md`.

**Open WIP branches:** `git branch -a` reports only:

```
* main
  remotes/origin/main
```

No feature branches. No stashes (`git stash list` empty). Phase 11.5
work-in-progress lives entirely in the dirty working tree on `main`.

### Targeted check — `volumetric-grid/eulerian-smoke/shaders/raymarch.frag.glsl`

`git log --all --since='2026-05-13' --name-only -- volumetric-grid/eulerian-smoke/shaders/raymarch.frag.glsl`
returns **no commits** touching this file since 2026-05-13. The file is also
not in `git status --short`. Phase 11.5 has neither touched nor (per the
`docs/diagnostics/_audits/` corpus) flagged this file as a planned edit.

The rule-of-three promotion candidate for shared volume raymarch (per Phase 8
project-state-§ 5 banking) is therefore unchanged from its Phase 8 shipped
state. Phase 12 can plan around the current file content (quoted verbatim in § D
below) without expecting Phase 11.5 collision.

---

## C — common-cpp current public surface (post-Phase-11)

### `common/common-cpp/include/gpusims/vk/context.hpp` — `ContextCreateInfo` + `Context` public methods

Verbatim:

```cpp
struct ContextCreateInfo {
    std::string                application_name = "gpu_sims";
    std::uint32_t              application_version = VK_MAKE_API_VERSION(0, 0, 1, 0);

    // Additional instance extensions to enable beyond what GLFW + debug needs.
    std::vector<const char*>   extra_instance_extensions;

    // Additional device extensions to enable beyond the defaults.
    std::vector<const char*>   extra_device_extensions;

    // Set true to require a discrete GPU. Default false (any conformant GPU
    // is acceptable; integrated is fine for hello-world on AMD).
    bool                       require_discrete_gpu = false;

    // Enable VK_KHR_swapchain. true unless this is a headless render-only sim.
    bool                       enable_swapchain = true;

    // Phase 11 (sph-water) consumer #1 of subgroup-size-control.
    // When true: enables VkPhysicalDeviceVulkan13Features::subgroupSizeControl
    // and queries VkPhysicalDeviceSubgroupSizeControlProperties at device-create
    // time. Throws if the device does not support the feature — fail-loud rather
    // than silently producing platform-dependent wavefront-size behavior.
    bool                       enable_subgroup_size_control = false;
};
```
— `common/common-cpp/include/gpusims/vk/context.hpp:24–47`.

Subgroup-size-control accessors added in `9e0ca2f`, verbatim:

```cpp
// Subgroup-size-control properties.
//
// Populated at device-create time when ContextCreateInfo::
// enable_subgroup_size_control was true. When the feature was NOT requested
// these return 0 / 0 / 0 / false — consult them only when the feature was
// requested.
//
// Values originate from VkPhysicalDeviceSubgroupSizeControlProperties
// queried via vkGetPhysicalDeviceProperties2 during Context construction.
std::uint32_t subgroupSizeMin()              const { return subgroup_size_min_; }
std::uint32_t subgroupSizeMax()              const { return subgroup_size_max_; }
std::uint32_t requiredSubgroupSizeStages()   const { return required_subgroup_size_stages_; }
bool          subgroupSizeControlEnabled()   const { return subgroup_size_control_enabled_; }
```
— `common/common-cpp/include/gpusims/vk/context.hpp:80–95`.

Other `Context` public methods (handles + properties + utilities), verbatim:

```cpp
// ----------------------------------------------------------------------
// Handles
// ----------------------------------------------------------------------
VkInstance       instance()        const { return instance_; }
VkPhysicalDevice physicalDevice()  const { return physical_device_; }
VkDevice         device()          const { return device_; }
VkQueue          graphicsQueue()   const { return graphics_queue_; }
VkQueue          computeQueue()    const { return compute_queue_; }
std::uint32_t    graphicsQueueFamily() const { return graphics_queue_family_; }
std::uint32_t    computeQueueFamily()  const { return compute_queue_family_; }
VmaAllocator     allocator()       const { return allocator_; }

// Properties of the chosen physical device, available for sims that
// want to scale parameters by hardware capability.
const VkPhysicalDeviceProperties&    deviceProperties()    const { return props_; }
const VkPhysicalDeviceMemoryProperties& memoryProperties() const { return mem_props_; }
```
— `common/common-cpp/include/gpusims/vk/context.hpp:58–78` (some lines elide
`integrity-allow` comment annotations).

```cpp
// Wait until all work submitted to all queues is complete. Use sparingly
// (mostly at shutdown or before reloading large resources).
void waitIdle() const;

// Convenience: allocate a transient command buffer, run callback, submit,
// wait, and free. Used for one-shot setup work (image transitions, etc.).
void runOneShot(const std::function<void(VkCommandBuffer)>& fn);
```
— `common/common-cpp/include/gpusims/vk/context.hpp:97–104`.

### `common/common-cpp/include/gpusims/vk/window.hpp` — ctor + present-mode surface

Constructor signature, verbatim:

```cpp
Window(Context& ctx, std::uint32_t width, std::uint32_t height,
       const std::string& title);
```
— `common/common-cpp/include/gpusims/vk/window.hpp:22–23`.

Present-mode / VSync surface — there is **no public present-mode getter or
setter**. The current present mode is a private cached value:

```cpp
VkPresentModeKHR                present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
```
— `common/common-cpp/include/gpusims/vk/window.hpp:85`.

Public swapchain accessors:

```cpp
GLFWwindow*       glfwWindow()   const { return window_; }
VkSurfaceKHR      surface()      const { return surface_; }
VkSwapchainKHR    swapchain()    const { return swapchain_; }
VkFormat          colorFormat()  const { return color_format_; }
VkExtent2D        extent()       const { return extent_; }
std::uint32_t     imageCount()   const { return static_cast<std::uint32_t>(images_.size()); }
VkImage           image(std::uint32_t i)     const { return images_[i]; }
VkImageView       imageView(std::uint32_t i) const { return image_views_[i]; }

// Aspect ratio for camera projection.
float aspect() const {
    return extent_.height == 0 ? 1.0f
        : static_cast<float>(extent_.width) / static_cast<float>(extent_.height);
}
```
— `common/common-cpp/include/gpusims/vk/window.hpp:53–71`.

Per-frame surface:

```cpp
// Per-frame
bool shouldClose() const;
void pollEvents();
void waitEvents();

// Acquire the next swapchain image. Returns:
//   - imageIndex on success
//   - std::nullopt if swapchain is out-of-date and was recreated; caller
//     should skip this frame and retry next iteration.
std::optional<std::uint32_t> acquireNextImage(VkSemaphore image_available);

// Present the rendered swapchain image. wait_semaphore signals when render
// is complete. Returns false if the swapchain became out-of-date during
// presentation; caller should recreate next frame.
bool present(std::uint32_t image_index, VkSemaphore wait_semaphore);

// Force-recreate the swapchain (e.g., after window-resize message).
void recreateSwapchain();
```
— `common/common-cpp/include/gpusims/vk/window.hpp:29–50`.

No banked Phase 8.5 present-mode retraction surface is present in the
current header — only the hard-coded `VK_PRESENT_MODE_FIFO_KHR` private
member. If Phase 12 wants run-time present-mode control it is a new
surface.

### `common/common-cpp/include/gpusims/vk/compute_pipeline.hpp` — `ComputePipelineDesc` (full)

Verbatim, including the `9e0ca2f` subgroup-size fields:

```cpp
struct DescriptorBinding {
    std::uint32_t      binding;
    VkDescriptorType   type;
    std::uint32_t      count = 1;
    VkShaderStageFlags stages = VK_SHADER_STAGE_COMPUTE_BIT;
};

struct ComputePipelineDesc {
    std::filesystem::path        shader_path;       // for hot-reload tracking
    std::vector<DescriptorBinding> bindings;
    std::uint32_t                push_constant_size = 0;       // bytes; 0 = none

    // Phase 11 sph-water: subgroup-size-control fields. Both default to
    // sentinel "unconstrained" values; the existing default-null pNext-chain
    // path is preserved when both fields are at their defaults.
    //
    // INVARIANT (must be preserved by all future maintainers):
    //   If required_subgroup_size != 0 OR require_full_subgroups == true,
    //   compute_pipeline.cpp builds VkPipelineShaderStageRequiredSubgroupSize
    //   CreateInfo and chains it into VkPipelineShaderStageCreateInfo.pNext
    //   (and/or sets the REQUIRE_FULL_SUBGROUPS_BIT flag). Otherwise pNext
    //   stays null and the flag stays zero.
    //
    //   Collapsing the conditional (always building the extension struct
    //   regardless of values) would force every compute pipeline to carry the
    //   subgroup-size extension, breaking compatibility with drivers/devices
    //   that don't support VK_EXT_subgroup_size_control.
    std::uint32_t                required_subgroup_size = 0;     // 0 = unconstrained
    bool                         require_full_subgroups = false; // false = unconstrained
};
```
— `common/common-cpp/include/gpusims/vk/compute_pipeline.hpp:25–54`.

`ComputePipeline` public methods, verbatim:

```cpp
static ComputePipeline create(Context&                   ctx,
                              ShaderCompiler&            compiler,
                              const ComputePipelineDesc& desc);
...
bool reload(Context&                   ctx,
            ShaderCompiler&            compiler,
            Frame&                     current_frame,
            std::string*               out_error = nullptr);

// Allocate a descriptor set from the wrapper's internal pool.
VkDescriptorSet allocateDescriptorSet();

// Bind & dispatch. Caller is responsible for inserting any required
// memory barriers around the dispatch.
void dispatch(VkCommandBuffer cmd,
              VkDescriptorSet ds,
              std::uint32_t   gx,
              std::uint32_t   gy,
              std::uint32_t   gz,
              const void*     push_constants = nullptr,
              std::uint32_t   push_size      = 0);

// Handles for callers that want to bind/dispatch directly.
VkPipeline            handle()        const { return pipeline_; }
VkPipelineLayout      pipelineLayout()const { return pipeline_layout_; }
VkDescriptorSetLayout descriptorSetLayout() const { return ds_layout_; }
const std::filesystem::path& shaderPath() const { return desc_.shader_path; }
const std::vector<std::filesystem::path>& includes() const { return last_includes_; }
```
— `common/common-cpp/include/gpusims/vk/compute_pipeline.hpp:58–106`.

### `common/common-cpp/include/gpusims/vk/buffer.hpp` (full public surface)

```cpp
enum class MemoryUsage {
    DeviceLocal,            // GPU-only; fastest for compute/render reads. Use staging to upload.
    HostVisibleSequential,  // CPU-mapped, write-combined; ideal for upload buffers (one-pass writes).
    HostVisibleRandom,      // CPU-mapped with cached reads; for readback or per-frame dynamic data.
};

class Buffer {
public:
    static Buffer create(Context&         ctx,
                         std::size_t      bytes,
                         VkBufferUsageFlags usage,
                         MemoryUsage      mem,
                         const char*      debug_name = nullptr);

    Buffer() = default;
    ~Buffer();

    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    // Handles
    VkBuffer      handle()      const { return buffer_; }
    VmaAllocation allocation()  const { return allocation_; }
    std::size_t   sizeBytes()   const { return size_; }

    // Mapped pointer; non-null only for HostVisible* memory usages.
    void*         mapped()      const { return mapped_; }

    // Convenience: copy `bytes` from src into the mapped pointer. Aborts in
    // Debug if this buffer is not host-visible.
    void uploadDirect(const void* src, std::size_t bytes, std::size_t offset = 0);

    // Stage-and-copy upload for DeviceLocal buffers. Allocates a transient
    // staging buffer, copies src into it, and submits a copy on the graphics
    // queue. Synchronous (waits for copy to complete).
    void stage(Context& ctx, const void* src, std::size_t bytes, std::size_t offset = 0);

    // Symmetric counterpart to stage(): copy bytes out of a DeviceLocal buffer
    // into host memory. Allocates a transient host-visible staging buffer,
    // submits a vkCmdCopyBuffer on the graphics queue, waits, and memcpys
    // the staging contents into dst. Synchronous. Phase 11 sph-water consumer
    // for F5 capture-save + Alembic-export per-frame readback.
    void readback(Context& ctx, void* dst, std::size_t bytes, std::size_t offset = 0);

    // Get a VkBufferDeviceAddress (for buffer device addresses).
    VkDeviceAddress deviceAddress(VkDevice device) const;
};
```
— `common/common-cpp/include/gpusims/vk/buffer.hpp:15–72` (selected, eliding
`integrity-allow` comment lines and the private section).

### `common/common-cpp/include/gpusims/vk/image.hpp` (full public surface)

```cpp
enum class ImageType {
    e2D,
    e3D,
};

struct ImageCreateInfo {
    ImageType         type   = ImageType::e2D;
    VkExtent3D        extent{};                              // depth=1 for 2D
    VkFormat          format = VK_FORMAT_R8G8B8A8_UNORM;
    std::uint32_t     mip_levels   = 1;
    std::uint32_t     array_layers = 1;                       // ignored for 3D
    VkSampleCountFlagBits samples  = VK_SAMPLE_COUNT_1_BIT;
    VkImageUsageFlags usage    = 0;                           // caller must set
    VkImageLayout     initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    const char*       debug_name = nullptr;
};

class Image {
public:
    static Image create(Context& ctx, const ImageCreateInfo& info);
    ...
    VkImage           handle()    const { return image_; }
    VkImageView       view()      const { return view_; }
    VmaAllocation     allocation()const { return allocation_; }
    VkExtent3D        extent()    const { return info_.extent; }
    VkFormat          format()    const { return info_.format; }
    ImageType         type()      const { return info_.type; }

    // Helper for transitioning an image's layout. Records pipeline barriers
    // into `cmd`. The user is responsible for tracking current layout — we
    // don't automatically remember (explicit > implicit).
    static void transitionLayout(VkCommandBuffer cmd,
                                 VkImage         image,
                                 VkImageAspectFlags aspect,
                                 VkImageLayout   old_layout,
                                 VkImageLayout   new_layout,
                                 std::uint32_t   mip_levels = 1,
                                 std::uint32_t   array_layers = 1);

    // Synchronous host -> device copy. Allocates a transient host-visible
    // staging buffer, copies via vkCmdCopyBufferToImage on the graphics queue,
    // and waits. Image is transitioned to GENERAL on return. Works for both
    // 2D and 3D images; `bytes` must equal extent.width*height*depth*texelSize.
    void upload(const void* src, std::size_t bytes);

    // Synchronous device -> host copy. The image must currently be in
    // VK_IMAGE_LAYOUT_GENERAL. Allocates a transient staging buffer,
    // copies via vkCmdCopyImageToBuffer, waits, and reads back. Image is
    // restored to GENERAL on return. Works for 2D and 3D.
    void readback(void* dst, std::size_t bytes);
};
```
— `common/common-cpp/include/gpusims/vk/image.hpp:15–82` (selected; `ImageType::e3D`
support is first-class, host-visible staging is the upload/readback path).

**Sampler creation note.** The image module exposes `view()` but does **not**
own a sampler factory. Sampler creation is the responsibility of the
consumer (eulerian-smoke creates `sampler_linear` and `sampler_lut` inline
in `volumetric-grid/eulerian-smoke/src/main.cpp`; there is no shared
`gpusims::vk::Sampler` wrapper).

### `common/common-cpp/include/gpusims/state_writer.hpp` — `saveBuffer` signature (incl. meta variant)

The single `saveBuffer` signature with meta-json variant, verbatim:

```cpp
// Save a binary blob.
//   name: identifier (used as filename stem and key in state.json)
//   data: pointer to bytes
//   bytes: byte count
//   meta: per-buffer metadata (e.g., {"count": 1000000, "stride": 32})
//         describes how to interpret the binary data on reload.
void saveBuffer(const std::string&    name,
                const void*           data,
                std::size_t           bytes,
                const nlohmann::json& meta = {});
```
— `common/common-cpp/include/gpusims/state_writer.hpp:36–46`.

The full public surface:

```cpp
class StateWriter {
public:
    explicit StateWriter(std::filesystem::path root_dir);

    // Begin a new capture. Creates capture_<frame_idx_padded>/ subdir.
    void beginFrame(std::uint32_t frame_idx);

    // Set arbitrary metadata; copied into state.json under the "meta" key.
    void setMeta(const std::string& key, const nlohmann::json& value);

    void saveBuffer(const std::string&    name,
                    const void*           data,
                    std::size_t           bytes,
                    const nlohmann::json& meta = {});

    // Write state.json and close the frame's directory.
    void endFrame();

    // Path to current capture directory (valid between begin/endFrame).
    const std::filesystem::path& currentDir() const { return current_dir_; }
};
```
— `common/common-cpp/include/gpusims/state_writer.hpp:24–53`.

There is **one** `saveBuffer` signature — meta is folded into the same call
as the optional 4th argument. There is no overload variant.

### `common/common-cpp/include/gpusims/state_reader.hpp` (full public surface)

```cpp
class StateReader {
public:
    static std::optional<StateReader> open(const std::filesystem::path& capture_dir);

    // Locate the most recent capture_NNNN/ subdirectory under `root`. Returns
    // nullopt if no captures exist or `root` does not exist. Selection is by
    // the largest NNNN suffix (sorted lexicographically on padded names).
    static std::optional<std::filesystem::path>
    findLatest(const std::filesystem::path& root);

    // Top-level metadata. Returns null json if key not present.
    nlohmann::json meta(const std::string& key) const;

    // Buffer descriptor (the per-buffer meta passed to StateWriter::saveBuffer).
    nlohmann::json bufferMeta(const std::string& name) const;

    // Raw bytes for a saved buffer. Empty if not present.
    std::vector<std::uint8_t> buffer(const std::string& name) const;

    std::uint32_t frameIndex() const { return frame_idx_; }

    const std::filesystem::path& dir() const { return dir_; }
};
```
— `common/common-cpp/include/gpusims/state_reader.hpp:23–49`.

### `common/common-cpp/include/gpusims/vdb_writer.hpp` — `writeVec3Grid` status

Header surface (verbatim):

```cpp
namespace vdb {

// Write a single dense float grid to a .vdb file. Returns false on failure.
bool writeFloatGrid(const std::filesystem::path& path,
                    const float*                 data,
                    glm::ivec3                   dims,
                    float                        voxel_size,
                    glm::vec3                    origin   = glm::vec3(0.0f),
                    const char*                  grid_name = "density");

// Write a single dense vec3 grid to a .vdb file (interleaved xyz floats).
bool writeVec3Grid(const std::filesystem::path& path,
                   const float*                 data,
                   glm::ivec3                   dims,
                   float                        voxel_size,
                   glm::vec3                    origin   = glm::vec3(0.0f),
                   const char*                  grid_name = "velocity");

// Convenience for sequence frames. Writes to <base>_<NNNN>.vdb.
bool writeFloatFrame(const std::filesystem::path& base,
                     std::uint32_t                frame_idx,
                     const float*                 data,
                     glm::ivec3                   dims,
                     float                        voxel_size,
                     glm::vec3                    origin   = glm::vec3(0.0f),
                     const char*                  grid_name = "density");

// True if this build was compiled with OpenVDB support.
bool isAvailable();

}  // namespace vdb
```
— `common/common-cpp/include/gpusims/vdb_writer.hpp:23–55`.

**`writeVec3Grid` is implemented**, not stub. Verbatim from
`common/common-cpp/src/vdb_writer.cpp:97`:

```cpp
bool writeVec3Grid(const std::filesystem::path& path,
                   const float*                 data,
                   glm::ivec3                   dims,
                   float                        voxel_size,
                   glm::vec3                    origin,
                   const char*                  grid_name) {
#if GPU_SIMS_HAVE_OPENVDB
    if (!data || dims.x <= 0 || dims.y <= 0 || dims.z <= 0) return false;
    initOpenVdbOnce();
    try {
        openvdb::Vec3SGrid::Ptr grid = openvdb::Vec3SGrid::create(openvdb::Vec3s(0.0f));
        grid->setName(grid_name ? grid_name : "velocity");
        grid->setTransform(openvdb::math::Transform::createLinearTransform(voxel_size));
        grid->setGridClass(openvdb::GRID_STAGGERED);

        // Manual fill: openvdb::tools::copyFromDense doesn't have a Vec3 specialization
        // we can rely on across versions, so we set values voxel-by-voxel.
        auto accessor = grid->getAccessor();
        for (int z = 0; z < dims.z; ++z) {
            for (int y = 0; y < dims.y; ++y) {
                for (int x = 0; x < dims.x; ++x) {
                    const std::size_t i = static_cast<std::size_t>(
                        x + dims.x * (y + dims.y * z)) * 3;
                    accessor.setValue(openvdb::Coord(x, y, z),
                                      openvdb::Vec3s(data[i + 0], data[i + 1], data[i + 2]));
                }
            }
        }

        if (origin != glm::vec3(0.0f)) {
            grid->transform().postTranslate(openvdb::Vec3d(origin.x, origin.y, origin.z));
```
— `common/common-cpp/src/vdb_writer.cpp:97–127`.

`grid->setGridClass(openvdb::GRID_STAGGERED)` is notable: the vec3 grid is
written as a staggered MAC-style grid, which matches the eulerian-smoke
velocity-field convention but may need translation if Phase 12 LBM emits
cell-centered velocities (LBM macroscopic `u = (1/ρ) Σ f_i e_i` is
cell-centered, not staggered).

There is also a stub-label-staleness flag in the header — verbatim:

```cpp
// integrity-allow: cat2.stub-label-stale; pre-v1.1 stale Phase-N stub label on real implementation (canonical spec section 12 row 5 -- tracked for migration as the corresponding header is next edited); n/a
// In Phase 1, this is a stub: if GPU_SIMS_HAVE_OPENVDB is not defined at
// compile time (i.e., GPU_SIMS_USE_OPENVDB=OFF in CMake), all functions log
// a warning on first call and return false. When OpenVDB is enabled, real
// implementations are provided.
```
— `common/common-cpp/include/gpusims/vdb_writer.hpp:12–17`.

I.e., the header still carries an obsolete "this is a stub" comment from
Phase 1 that contradicts the actual implementation (since Phase 8 first-real-
consumer for `writeFloatGrid`, and `writeVec3Grid` is also real). The integrity-
toolkit v1.1 work-in-progress in the dirty working tree is the migration
that will rewrite this comment.

---

## D — eulerian-smoke precedent

### `volumetric-grid/eulerian-smoke/src/main.cpp`

The file is 2238 lines. Quoting the **specific zones** the probe brief calls
out (substep loop, descriptor-set construction, raymarch graphics pipeline,
obstacle plumbing if any, F5 capture site). Wider context available at the
file directly.

#### (a) Substep loop structure

Per-substep dispatch chain (`volumetric-grid/eulerian-smoke/src/main.cpp:1879–2012`):

```cpp
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
    gv::memoryBarrier(cmd, ...);

    // (2) Advect density.
    {
        auto scope = profiler.scope(cmd, "advect_density");
        pipe_advect_scalar.dispatch(cmd, ds_advect_density[p][slot], wg, wg, wg);
    }
    // (3) Advect temperature.
    {
        auto scope = profiler.scope(cmd, "advect_temperature");
        pipe_advect_scalar.dispatch(cmd, ds_advect_temperature[p][slot], wg, wg, wg);
    }
    gv::memoryBarrier(cmd, ...);

    // (4) Apply buoyancy (in-place on velocity_new).
    pipe_buoyancy.dispatch(cmd, ds_buoyancy[p][slot], wg, wg, wg);
    gv::memoryBarrier(cmd, ...);

    // (5) Compute curl.
    pipe_curl.dispatch(cmd, ds_curl[p][slot], wg, wg, wg);
    gv::memoryBarrier(cmd, ...);

    // (6) Apply vorticity confinement (in-place on velocity_new).
    pipe_vorticity.dispatch(cmd, ds_vorticity[p][slot], wg, wg, wg);
    gv::memoryBarrier(cmd, ...);

    // (7) Emit sources (in-place on velocity_new, density_new, temperature_new).
    pipe_emit.dispatch(cmd, ds_emit[p][slot], wg, wg, wg);
    gv::memoryBarrier(cmd, ...);

    // (8) Apply boundaries (in-place on velocity_new; zeros at the 5 no-slip faces).
    pipe_boundaries.dispatch(cmd, ds_boundaries[p][slot], wg, wg, wg);
    gv::memoryBarrier(cmd, ...);

    // (9) Compute divergence.
    pipe_divergence.dispatch(cmd, ds_divergence[p][slot], wg, wg, wg);
    gv::memoryBarrier(cmd, ...);

    // (10) Jacobi pressure inner loop (ping-pong on pressure).
    {
        auto scope = profiler.scope(cmd, "jacobi_pressure");
        uint32_t jp = 0;
        for (int i = 0; i < rt.pressureIters; ++i) {
            pipe_jacobi.dispatch(cmd, ds_jacobi[jp][slot], wg, wg, wg);
            gv::memoryBarrier(cmd, ...);
            jp ^= 1u;
        }
    }

    // (11) Project: subtract grad(p) from velocity (in-place on velocity_new).
    uint32_t project_p = uint32_t((rt.pressureIters - 1u) & 1u);
    pipe_project.dispatch(cmd, ds_project[project_p][slot], wg, wg, wg);

    gv::memoryBarrier(cmd, ...);
    rt.iteration++;
}

// --------------------------------------------------------------------
// Final barrier before the fragment-shader raymarch reads density + temperature.
// --------------------------------------------------------------------
gv::memoryBarrier(cmd,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
```

Notable: parity is taken from `rt.iteration & 1u` (the iteration counter
flips every substep), each compute stage has a `memoryBarrier`, the inner
Jacobi pressure loop has its OWN `jp` parity, and the project step computes
`project_p = (rt.pressureIters - 1u) & 1u` to read the correct final
pressure image. Workgroup count is `(gridSize + WG_DIM - 1) / WG_DIM` with
`WG_DIM = 8` (declared at line 68).

#### (b) Descriptor-set construction style

Pipeline-internal descriptor allocation (`volumetric-grid/eulerian-smoke/src/main.cpp:1168–1208`):

```cpp
auto alloc_sets = [&](auto& pipeline, uint32_t count) {
    std::vector<VkDescriptorSet> v(count);
    for (auto& ds : v) ds = pipeline.allocateDescriptorSet();
    return v;
};

constexpr uint32_t kParities = 2;
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_advect_velocity{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_advect_density{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_advect_temperature{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_buoyancy{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_curl{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_vorticity{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_emit{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_boundaries{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_divergence{};
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
```

Wiring (`volumetric-grid/eulerian-smoke/src/main.cpp:1215–1279`):

```cpp
auto wireAllDescriptors = [&]() {
    VkDevice dev = ctx.device();
    for (uint32_t p = 0; p < kParities; ++p) {
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
            // ... [eleven writeXxxDescriptor calls per (parity, slot) pair] ...
            writeRaymarchDescriptor(dev, ds_raymarch[p][s],
                den_new_v, tem_new_v, blackbody_lut.view(), bluenoise_lut.view(),
                sampler_linear, sampler_lut, ub_raymarch[s].handle());
        }
    }
};
wireAllDescriptors();
```

The `writeXxxDescriptor` helpers are sim-local free functions (declared near
line 531 — "All eleven descriptor-write helpers (one per pipeline: 10 compute
+ 1 graphics raymarch)"). They wrap `vkUpdateDescriptorSets` per pipeline.
This is **not** an abstraction in `common-cpp` — every Stack C sim writes its
own descriptor-write helpers.

#### (c) Volume raymarch graphics-pipeline construction

`volumetric-grid/eulerian-smoke/src/main.cpp:1144–1158`:

```cpp
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
```

`CIS` = `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`, `UB` =
`VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (local aliases declared earlier near
line 1108). Five bindings: 4 combined-image-samplers (density, temperature,
blackbody LUT, blue-noise) + 1 uniform buffer (RaymarchUniforms).

#### (d) Obstacle-mask buffer plumbing

`grep -E "(obstacle|Obstacle)"` against both `volumetric-grid/eulerian-smoke/src/main.cpp`
and `volumetric-grid/eulerian-smoke/shaders/*.glsl` returns **zero matches**.
Eulerian-smoke has **no obstacle-mask plumbing**. The only boundary handling
is the no-slip apply-boundaries kernel:

> `// (8) Apply boundaries (in-place on velocity_new; zeros at the 5 no-slip faces).`

i.e., domain-edge no-slip only; no interior obstacles. Phase 12 LBM (which
needs an airfoil obstacle mask per `volumetric-grid/lattice-boltzmann/README.md:7`)
will be net-new infrastructure (see § H).

#### (e) Capture (F5) call site

The save lambda (`volumetric-grid/eulerian-smoke/src/main.cpp:1421–1454`):

```cpp
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
```

F5 / F9 rising-edge tracking (`volumetric-grid/eulerian-smoke/src/main.cpp:1665–1670`):

```cpp
bool now_f5 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F5) == GLFW_PRESS;
bool now_f9 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F9) == GLFW_PRESS;
if (now_f5 && !prev_f5) capture_save();
if (now_f9 && !prev_f9) capture_load();
```

Note: the `velocity.bin` name is passed with `.bin` already attached;
`StateWriter` appends `.bin` again on disk, producing `velocity.bin.bin`.
This is the documented Phase 8 double-extension quirk
(`docs/tier1-capture-format-reference.md:199–210`).

### `volumetric-grid/eulerian-smoke/shaders/raymarch.frag.glsl` — full file

Quoted verbatim (147 lines total at `volumetric-grid/eulerian-smoke/shaders/raymarch.frag.glsl:1–147`):

```glsl
#version 450
#extension GL_GOOGLE_include_directive : require

// Volume raymarch fragment shader.
// Per-ray integral: see phase8_eulerian_smoke.md § 2.12 for the math.
//
// Inputs:
//   - u_density        sampler3D r32f         smoke density field
//   - u_temperature    sampler3D r32f         temperature field
//   - u_blackbody_lut  sampler2D rgba8        256x4 LUT (4 color ramps; row by colorRamp idx)
//   - u_bluenoise      sampler2D r8           256x256 blue-noise jitter pattern
//
// Output: tonemap result in the swapchain color attachment.

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler3D u_density;
layout(set = 0, binding = 1) uniform sampler3D u_temperature;
layout(set = 0, binding = 2) uniform sampler2D u_blackbody_lut;
layout(set = 0, binding = 3) uniform sampler2D u_bluenoise;

layout(set = 0, binding = 4) uniform RaymarchUniforms {
    mat4  invViewProj;
    vec4  cameraPos;          // .xyz = pos, .w = unused
    vec4  volumeMin;          // .xyz = (0,0,0), .w = unused
    vec4  volumeMax;          // .xyz = (1,1,1), .w = unused
    vec4  lightDir;           // .xyz = normalized, .w = unused
    vec4  lightColor;         // .xyz, .w = ambient strength
    vec4  bgTopColor;         // .xyz, .w = unused
    vec4  bgBottomColor;      // .xyz, .w = unused
    int   raymarchSteps;
    int   shadowMarchSteps;
    float densityAbsorption;
    float emissionStrength;
    float scatteringStrength;
    float exposure;
    float colorRampRow;       // 0..3 selects LUT row (blackbody / sunset / cold / mono)
    float shadowMarchSoftness;
} rm;

// Slab/AABB intersection: returns (t_near, t_far) for the ray's intersection with the unit cube.
// Returns (1, -1) on miss (which the caller treats as no-hit).
vec2 intersectBox(vec3 origin, vec3 dir, vec3 boxMin, vec3 boxMax) {
    vec3 invDir = 1.0 / dir;
    vec3 t0 = (boxMin - origin) * invDir;
    vec3 t1 = (boxMax - origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float t_near = max(max(tmin.x, tmin.y), tmin.z);
    float t_far  = min(min(tmax.x, tmax.y), tmax.z);
    return vec2(t_near, t_far);
}

vec3 sampleBlackbody(float t) {
    float tClamp = clamp(t * 0.5, 0.0, 1.0);   // t in [0, 2] mapped to LUT u in [0, 1]
    float rowU = (rm.colorRampRow + 0.5) / 4.0;
    return texture(u_blackbody_lut, vec2(tClamp, rowU)).rgb;
}

void main() {
    // Reconstruct ray from clip-space.
    vec2 ndc = v_uv * 2.0 - 1.0;
    // WebGPU-style Y points up (matches gl-matrix perspectiveZO). No flip here.
    vec4 near_h = rm.invViewProj * vec4(ndc, 0.0, 1.0);
    vec4 far_h  = rm.invViewProj * vec4(ndc, 1.0, 1.0);
    vec3 ray_origin = near_h.xyz / near_h.w;
    vec3 ray_dir    = normalize((far_h.xyz / far_h.w) - ray_origin);

    // Background gradient (vertical, top -> bottom).
    vec3 bg = mix(rm.bgBottomColor.xyz, rm.bgTopColor.xyz, clamp(v_uv.y, 0.0, 1.0));

    // Box intersection in normalized [0,1]^3 space.
    vec2 t_range = intersectBox(ray_origin, ray_dir, rm.volumeMin.xyz, rm.volumeMax.xyz);
    float t_near = max(t_range.x, 0.0);
    float t_far  = t_range.y;
    if (t_far <= t_near) {
        outColor = vec4(bg, 1.0);
        return;
    }

    // Blue-noise jitter to break slice-banding (one tap per pixel).
    ivec2 bn_coord = ivec2(gl_FragCoord.xy) & ivec2(255);
    float jitter = texelFetch(u_bluenoise, bn_coord, 0).r;

    float step_size = (t_far - t_near) / float(rm.raymarchSteps);
    float t = t_near + jitter * step_size;

    vec3 L = vec3(0.0);
    float T = 1.0;
    vec3 lightDir = normalize(rm.lightDir.xyz);
    float ambient = rm.lightColor.w;

    // Shadow-march step size: probe one-volume-diagonal worth of distance across N steps.
    float shadow_step = (1.732 / float(rm.shadowMarchSteps));   // sqrt(3) for unit cube diagonal

    for (int i = 0; i < rm.raymarchSteps; ++i) {
        if (T < 0.01) break;   // early-out

        vec3 pos = ray_origin + t * ray_dir;
        float density     = texture(u_density,     pos).r;
        float temperature = texture(u_temperature, pos).r;

        if (density > 0.001) {
            float absorption = density * rm.densityAbsorption;
            float sample_T   = exp(-absorption * step_size);

            // Emission contribution (temperature -> black-body color).
            vec3 L_e = rm.emissionStrength * sampleBlackbody(temperature) * density;

            // Shadow march toward the key light.
            float shadow_T = 1.0;
            float shadow_t = shadow_step;
            for (int s = 0; s < rm.shadowMarchSteps; ++s) {
                vec3 shadow_pos = pos + shadow_t * lightDir;
                if (any(lessThan(shadow_pos, rm.volumeMin.xyz)) ||
                    any(greaterThan(shadow_pos, rm.volumeMax.xyz))) break;
                float shadow_density = texture(u_density, shadow_pos).r;
                shadow_T *= exp(-shadow_density * rm.densityAbsorption * shadow_step * rm.shadowMarchSoftness);
                if (shadow_T < 0.01) break;
                shadow_t += shadow_step;
            }

            // Scattering contribution.
            vec3 L_s = rm.scatteringStrength * shadow_T * rm.lightColor.xyz * density;

            // Ambient (uniform soft-light fill).
            vec3 L_a = ambient * density * rm.lightColor.xyz;

            // Accumulate (front-to-back compositing).
            L += T * step_size * (L_e + L_s + L_a);
            T *= sample_T;
        }

        t += step_size;
    }

    // Alpha-blend with background by remaining transmittance.
    L += T * bg;

    // Inline Reinhard tonemap.
    L = L * rm.exposure;
    L = L / (L + 1.0);

    outColor = vec4(L, 1.0);
}
```

**Inputs the file consumes** (load-bearing for any rule-of-three generalization):

- Two `sampler3D r32f` scalar fields (`u_density`, `u_temperature`). To
  visualize an LBM velocity magnitude or vorticity magnitude scalar field
  Phase 12 could feed a single such field; the existing structure handles
  one well.
- One `sampler2D rgba8` color-ramp LUT (`u_blackbody_lut`, with row
  selector `rm.colorRampRow`).
- One `sampler2D r8` blue-noise jitter.
- `RaymarchUniforms` block — physically-driven absorption/emission/scattering;
  not directly meaningful for "velocity magnitude" or "vorticity magnitude"
  display. A vector-field promotion would need to either swap or extend this
  to add (a) a value-range remap, (b) a transfer-function LUT, (c)
  velocity-direction encoding if showing more than magnitude.

The shader is single-scattering with a single shadow-march step per voxel
(double-loop: outer raymarch, inner shadow march). No multi-scattering, no
multi-light. The unit-cube `[0,1]^3` assumption (slab intersection,
`shadow_step = sqrt(3)/N`) is baked-in; any non-unit volume aspect (e.g.,
the LBM `512×256×256` aspect ratio at the README) breaks the
`shadow_step = sqrt(3)/N` line.

### `volumetric-grid/eulerian-smoke/CMakeLists.txt` — full file

`volumetric-grid/eulerian-smoke/CMakeLists.txt:1–65`:

```cmake
# Eulerian Smoke — first Tier-2 flagship Stack C sim.
# Consumes gpusims::common_cpp. Builds the binary `eulerian_smoke`.
#
# This sim is the first real consumer of common-cpp's OpenVDB writer
# (gpusims::vdb::writeFloatFrame). VDB export is gated at runtime by
# gpusims::vdb::isAvailable(), which returns true only when common-cpp
# was built with -DGPU_SIMS_USE_OPENVDB=ON. Stub mode compiles and runs
# fine; the VDB toggle in the panel becomes a no-op.

add_executable(eulerian_smoke
    src/main.cpp
)

target_link_libraries(eulerian_smoke
    PRIVATE
        gpusims::common_cpp
)

target_compile_features(eulerian_smoke PRIVATE cxx_std_20)

# Pass the shader directory into the binary so it can locate shaders regardless
# of the cwd at launch time. Mirrors common-cpp/examples/hello and reaction-diffusion-3d.
target_compile_definitions(eulerian_smoke PRIVATE
    GPU_SIMS_ES_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

# Output binary name keeps snake_case (Phase 1 convention; matches gpu_sims_hello,
# reaction_diffusion_3d).
set_target_properties(eulerian_smoke PROPERTIES
    OUTPUT_NAME       "eulerian_smoke"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Shaders are compiled at runtime via gpusims::vk::ShaderCompiler.
# Copy them next to the binary so cwd-relative loading works during smoke tests.
set(SHADER_SOURCES
    shaders/fullscreen.vert.glsl
    shaders/raymarch.frag.glsl
    shaders/advect_velocity.comp.glsl
    shaders/advect_scalar.comp.glsl
    shaders/apply_buoyancy.comp.glsl
    shaders/compute_curl.comp.glsl
    shaders/apply_vorticity_confinement.comp.glsl
    shaders/emit_sources.comp.glsl
    shaders/apply_boundaries.comp.glsl
    shaders/compute_divergence.comp.glsl
    shaders/jacobi_pressure.comp.glsl
    shaders/apply_pressure.comp.glsl
)

foreach(SHADER ${SHADER_SOURCES})
    set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(DST ${CMAKE_BINARY_DIR}/bin/eulerian-smoke/${SHADER})
    add_custom_command(
        OUTPUT ${DST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DST}
        DEPENDS ${SRC}
        COMMENT "Copying ${SHADER}"
    )
    list(APPEND SHADER_OUTPUTS ${DST})
endforeach()

add_custom_target(eulerian_smoke_shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(eulerian_smoke eulerian_smoke_shaders)
```

Notable: 65 lines total; only `gpusims::common_cpp` link dependency; the
`GPU_SIMS_ES_SHADER_DIR` compile-definition pattern + per-sim `set(SHADER_SOURCES …)` +
`foreach` copy step is the Stack C per-sim CMake template. No CMake-time
shader compilation — `gpusims::vk::ShaderCompiler` does it at runtime.

---

## E — References vendoring posture

### `references/` gitignore + SPlisHSPlasH status

From `.gitignore`:

```
# Allow tracking specific sample/reference VDB/ABC files by
# Phase 11.5: SPlisHSPlasH vendored upstream reference (clone-on-setup, gitignored).
/references/
```

Confirmed: **`references/` is gitignored**. Local working-tree directory
listing shows `references/SPlisHSPlasH` as the sole vendored upstream (matches
`docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md`'s
front-matter pinning `references/SPlisHSPlasH/ @ 6bff55a6eaf14083d34650f22a268ce156b62b54`,
"SPlisHSPlasH 2.16.1").

### LBM v1-anchor candidate libraries

**Important caveat:** the LBM-reference research task was attempted with a
sub-agent that has access to `WebSearch` / `WebFetch`. Both tools were denied
in this environment. Without live web access I cannot produce verified
current-release dates, exact SPDX license strings, or kernel line counts.
The following is from prior knowledge and should be **independently verified
by the human before locking a v1 anchor**.

- **waLBerla** (Erlangen, FAU)
  - Repo (best known): <https://i10git.cs.fau.de/walberla/walberla>
  - License (best known): GNU GPL v3 (this should be verified — some
    Erlangen-Nuremberg components have been re-licensed historically).
  - Latest tag/release: **needs human lookup**.
  - D3Q19 BGK kernel LOC: **needs human lookup**. waLBerla's LBM core is
    code-generated via `lbmpy`; the "kernel" exists as Python codegen output
    rather than a hand-written `.cu`/`.cpp` file. Line counts depend on
    generator settings.
  - GPU support: yes (CUDA, originally; HIP/SYCL more recent).
- **OpenLB** (Karlsruhe)
  - Repo (best known): <https://www.openlb.net/> (Git via the project page).
  - License (best known): GNU GPL v2-or-later. **License concern flagged in
    the brief still applies: GPL is contagious for a derived/vendored
    reference.** This may exclude OpenLB on license-compatibility grounds with
    the MIT-licensed GPU-Sims repo, unless the intent is "study, don't
    re-distribute".
  - Latest tag/release: **needs human lookup**.
  - D3Q19 BGK kernel LOC: **needs human lookup**.
- **Palabos** (Switzerland, originally University of Geneva)
  - Repo (best known): <https://gitlab.com/unigespc/palabos>
  - License (best known): AGPL v3 (similar GPL-family compatibility concerns
    to OpenLB; AGPL is even more contagious than GPL).
  - Latest tag/release: **needs human lookup**.
  - D3Q19 BGK kernel LOC: **needs human lookup**.
  - GPU support: yes (Palabos has a GPU implementation; CUDA).
- **Krüger et al., "The Lattice Boltzmann Method" (2017) companion code**
  - The book is published by Springer. The textbook has a companion code
    repository whose exact URL is not in my offline memory with confidence.
    Common entry points used historically have been on Krüger's group page and
    a "lbm-principles-and-practice" or "LBM-book" GitHub repo.
  - License: **needs human lookup**.
  - This is the lightest-weight reference and the natural "minimal pedagogical
    D3Q19 BGK reference" candidate; line counts for the bare BGK kernel are
    typically a few dozen lines.

**Recommendation per brief:** deferred to human; no design call made here. The
key facts to chase down (whichever candidate is selected): (a) license, (b)
current tag/release SHA, (c) location and LOC of the D3Q19 BGK collision
kernel for the anchor pin.

---

## F — Capture-format tier-1 contract recap

### § 1 sim-namespace table — staleness check

`docs/tier1-capture-format-reference.md:7–23`:

```markdown
## 1. Top-level `meta` keys observed across shipped sims (8 rows below)

Every shipped sim writes **exactly one** sim-namespaced top-level key in `state.json.meta`. The key is the activation signature for the Tier-3 module that diagnoses that sim.

> **Note (Phase 11):** This table currently lists sims through Phase 8 plus Phase 11 (sph-water). Phase 9 (mpm-multimaterial) and Phase 10 (lenia-fft) rows are scheduled for a separate ledger-backfill commit per `phase11_deferred_backfill.md` Item 1. Their top-level meta keys are `mpmMultimaterial` and `lenia` respectively; consult their CHANGELOG entries / project-state ledger for canonical confirmation.

| Sim | Phase | Stack | Top-level meta key |
|-----|-------|-------|--------------------|
| strange-attractors | 2 | B (TS) | `strangeAttractors` |
| reaction-diffusion-3d | 3 | C (C++) | `reactionDiffusion3d` |
| mandelbulb-explorer | 4 | B (TS) | `mandelbulbExplorer` |
| reaction-diffusion-2d | 5 | B (TS) | `reactionDiffusion2d` |
| physarum | 6 | B (TS) | `physarum` |
| boids-3d | 7 | B (TS) | `boids3d` |
| eulerian-smoke | 8 | C (C++) | `eulerianSmoke` |
| sph-water | 11 | C (C++) | `sphWater` |
```

The Phase-9 / Phase-10 backfill called out by the doc's own self-note is
**still not done**. The table still omits `mpmMultimaterial` and `lenia` rows
(confirmed by reading the current file). The Phase 11 retro item 1 referenced
by the user's brief is unresolved at HEAD.

The Phase 12 sim's expected key (per the camelCase-named-after-sim
convention): `latticeBoltzmann`.

### § 2 saveBuffer precedent — eulerian-smoke block

`docs/tier1-capture-format-reference.md:112–122` (the only volumetric-grid
data point):

```markdown
### eulerian-smoke (Phase 8, Stack C)
```cpp
saveBuffer("velocity.bin",    {count: N, stride: 8, format: "rgba16f",  shape: [G, G, G]});
saveBuffer("density.bin",     {count: N, stride: 4, format: "r32f",     shape: [G, G, G]});
saveBuffer("temperature.bin", {count: N, stride: 4, format: "r32f",     shape: [G, G, G]});
saveBuffer("pressure.bin",    {count: N, stride: 4, format: "r32f",     shape: [G, G, G]});
```
**Note:** buffer names include `.bin` extension here (`velocity.bin`, etc.), unlike the other sims which pass bare names (`u`, `v`, `trail`, `entities`). `StateWriter` appends `.bin` on bare names; passing `velocity.bin` produces a file literally named `velocity.bin.bin` on disk. Worth verifying against an actual capture — possible bug in Phase 8.

Velocity buffer uses `rgba16f` despite being a 3-component vector — the 4th component is unused. Tier-1 should expose vector fields as `[G, G, G, 4]` and let Tier-2 vector-field diagnostics slice off the unused channel.
```

The Phase 8 double-extension bug is unfixed at HEAD (the smoke source quoted
in § D's "(e) Capture (F5) call site" still passes literal `velocity.bin` /
`density.bin` / etc. names).

---

## G — Build-system state

### `.github/workflows/build-native.yml` — full file (Release + Debug)

`.github/workflows/build-native.yml:1–113`:

```yaml
name: Build (native)

on:
  push:
    paths:
      - 'CMakeLists.txt'
      - 'common/common-cpp/**'
      - '.github/workflows/build-native.yml'
  pull_request:
    paths:
      - 'CMakeLists.txt'
      - 'common/common-cpp/**'
      - '.github/workflows/build-native.yml'
  workflow_dispatch:

jobs:
  build-ubuntu:
    name: Ubuntu 24.04 / Vulkan / Release
    runs-on: ubuntu-24.04

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install build dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            git \
            pkg-config \
            libgl1-mesa-dev \
            libxinerama-dev \
            libxcursor-dev \
            libxi-dev \
            libxrandr-dev \
            libwayland-dev \
            libxkbcommon-dev \
            libvulkan-dev \
            vulkan-tools \
            vulkan-validationlayers \
            libopenvdb-dev \
            libboost-iostreams-dev \
            libimath-dev \
            spirv-tools \
            glslang-tools

      - name: Verify Vulkan SDK is available
        run: |
          vulkaninfo --summary || true
          glslangValidator --version

      - name: Configure (Release, examples ON, OpenVDB ON, Alembic ON)
        run: |
          cmake -S . -B build \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DGPU_SIMS_BUILD_EXAMPLES=ON \
            -DGPU_SIMS_USE_OPENVDB=ON \
            -DGPU_SIMS_USE_ALEMBIC=ON

      - name: Build
        run: cmake --build build --parallel

      - name: List built artifacts
        run: |
          echo "=== Build directory ==="
          find build -type f -executable -not -path '*/\.*' | head -50
          echo "=== Hello binary ==="
          ls -la build/common/common-cpp/examples/hello/ || true

  build-ubuntu-debug:
    name: Ubuntu 24.04 / Vulkan / Debug + Validation
    runs-on: ubuntu-24.04

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install build dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            git \
            pkg-config \
            libgl1-mesa-dev \
            libxinerama-dev \
            libxcursor-dev \
            libxi-dev \
            libxrandr-dev \
            libwayland-dev \
            libxkbcommon-dev \
            libvulkan-dev \
            vulkan-validationlayers \
            libimath-dev \
            spirv-tools \
            glslang-tools

      - name: Configure (Debug, examples ON, Alembic ON)
        run: |
          cmake -S . -B build \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Debug \
            -DGPU_SIMS_BUILD_EXAMPLES=ON \
            -DGPU_SIMS_USE_ALEMBIC=ON

      - name: Build
        run: cmake --build build --parallel
```

**Key facts for Phase 12:**

- **Path triggers** restrict CI to `CMakeLists.txt`, `common/common-cpp/**`,
  and the workflow file itself. **A per-sim subdirectory change (e.g.,
  `volumetric-grid/lattice-boltzmann/**`) does NOT trigger this workflow.**
  Phase 8 (eulerian-smoke) had the same pattern; Phase 11 (sph-water) the
  same. If Phase 12 wants CI coverage of LBM source on push, the trigger
  paths need to be extended (or a separate workflow added).
- **Release job builds with OpenVDB ON + Alembic ON.** Debug job builds with
  Alembic ON but **OpenVDB OFF** (and does not install `libopenvdb-dev`).
  Phase 12's velocity-field VDB export would compile in Debug only in
  stub-mode unless the Debug job's apt list + cmake flag are extended.
- **Release-only `vulkan-tools`** — `vulkaninfo` only runs in the Release
  job. Debug job has `vulkan-validationlayers` but not `vulkan-tools` and
  not `libopenvdb-dev`/`libboost-iostreams-dev`.
- **Debug-only validation layers** by package availability (only the Debug
  apt list installs `vulkan-validationlayers` with no separate Release
  reference, though both lists include it — the Debug build is the
  validation-layer signal per `project-state.md:Quick reference` build
  notes).

### Open path-trigger gotchas from Phase 11

The Phase 11 retro and Phase 11.5 audit corpus (`docs/diagnostics/_audits/phase11_5_*`)
do not surface any unresolved CI path-trigger gotchas specific to that build
job. The known integrity-toolkit CI workflow added in `f7e012d` /
`1e886e6` (commit 4b + fix) lives in a separate workflow file (out of scope
for Phase 12 unless Phase 12 itself extends the integrity toolkit). The
known path-trigger fact relevant to Phase 12 is the one above: **the build
workflow does not currently re-run on changes inside per-sim subdirectories**.

---

## H — Sanity check on the airfoil

### SDF infrastructure in common-cpp / eulerian-smoke

`grep -nirE "(SDF|sdf|airfoil|NACA|signed_distance|signedDistance|sdSphere|sdBox|obstacleMask|obstacle_mask)"`
applied to `common/common-cpp/` and `volumetric-grid/` returns only the
following non-airfoil matches:

```
volumetric-grid/lattice-boltzmann/README.md:7:512×256×256 D3Q19 LBM around an airfoil, with live streamlines.
volumetric-grid/README.md:8:- [`lattice-boltzmann/`](lattice-boltzmann/) — 512×256×256 D3Q19 LBM around an airfoil with live streamlines. **Stack C (Native C++).**
```

**No SDF infrastructure exists in `common-cpp`**, and **no obstacle / SDF
plumbing exists in eulerian-smoke**. The only mentions of "airfoil" in the
repo are the two README placeholder lines. Voxelizing a NACA airfoil
cross-section into an obstacle mask is **net-new work for Phase 12** — there
is no rule-of-three baseline.

Closest existing precedents (none are SDF, none are obstacle-mask):

- Eulerian-smoke `apply_boundaries.comp.glsl` is no-slip on the five domain
  faces (boundary-of-the-volume zero-velocity), not interior obstacles.
- Eulerian-smoke `emit_sources.comp.glsl` does have a sparse-source kernel
  with falloff (the emitter cap-8 system), which is the closest "interior
  per-voxel cell-mask" pattern in Stack C, but it's an additive emission, not
  a Bool obstacle.

If Phase 12 needs a reusable SDF/obstacle-mask facility, the natural design
choices appear to be: (a) generate the mask on the CPU and `Image::upload`
it as a `r8` / `r8ui` 3D image (consistent with the eulerian-smoke 3D-image
upload pattern), or (b) generate on the GPU via a dedicated compute kernel.
No design call made here — flagging only that this is fresh ground.

### `docs/sim-specs/lattice-boltzmann.md` status

The spec sheet at `docs/sim-specs/lattice-boltzmann.md:1–10`:

```markdown
# Lattice Boltzmann — Specification

> **Status:** Specification pending — not yet drafted by the architect chat
> **Category:** Volumetric grid
> **Primary stack:** C (Native C++)
> **Secondary stack(s):** —
> **Target machine:** Desktop, A100 hero
> **Folder:** [`volumetric-grid/lattice-boltzmann`](../../volumetric-grid/lattice-boltzmann/)
```

— spec is stub-only (the rest of the file is section-header placeholders).
Project-state.md § 6 row for `volumetric-grid/lattice-boltzmann/` confirms:
"Sim-spec stub".

---

## Summary — surprises and project-state.md contradictions

1. **`project-state.md` § 11 "Latest commit" line is `0243278`** but HEAD is
   `447ebf0` — ~40 commits have landed without that line being refreshed.
   Half of those commits are in-flight Phase 11.5 work (DFSPH restructure +
   common-cpp Layer 2 fixes) and half are an entirely new tools area
   (`tools/integrity/` v1 — eight feature commits + their audit docs and
   retros). The "Latest commit" line is the cleanest contradiction with
   actual state.

2. **`tier1-capture-format-reference.md` § 1's self-acknowledged backfill
   debt is still outstanding.** The sim table still lacks
   `mpmMultimaterial` (Phase 9) and `lenia` (Phase 10) rows. The doc's own
   note flags this as Phase 11 retro item 1. Confirmed unresolved at HEAD.

3. **`vdb_writer.hpp` header carries a "Phase 1 stub" comment** that
   contradicts the actual `vdb_writer.cpp` real implementation of all three
   functions (`writeFloatGrid`, `writeVec3Grid`, `writeFloatFrame`) under
   `#if GPU_SIMS_HAVE_OPENVDB`. The integrity-toolkit v1.1 work-in-progress
   sitting in the dirty working tree (`stub_label_stale.py` Cat 2 check +
   fixtures) is the migration that will catch / fix this. Phase 12 can
   safely treat `writeVec3Grid` as fully usable — the only false signal is
   the header comment, not the implementation.

4. **`writeVec3Grid` writes `openvdb::Vec3SGrid` with class
   `GRID_STAGGERED`.** Eulerian-smoke uses staggered MAC-style velocity, so
   this matches. LBM macroscopic velocity is **cell-centered**, not
   staggered, so feeding LBM velocity into `writeVec3Grid` would tag the
   grid class incorrectly. Not a blocker (downstream tools may not care) but
   flagged.

5. **Eulerian-smoke has no obstacle / SDF / airfoil infrastructure of any
   kind.** No rule-of-three baseline exists for obstacle masks; Phase 12 is
   net-new ground for that surface (see § H).

6. **No present-mode / VSync surface is exposed by `vk::Window`.** Present
   mode is a private `VK_PRESENT_MODE_FIFO_KHR` constant. If Phase 12 wants
   runtime present-mode control (uncapped FPS for capture-mode hero-runs at
   the 512×256×256 scale), that's a new public surface, not banked from
   Phase 8.5.

7. **`build-native.yml` Debug job builds without OpenVDB.** If Phase 12
   wants VDB export validated in the Debug job, the Debug apt list +
   cmake flags need extending; Phase 8 + Phase 11 left the asymmetry intact.

8. **CI build-native path triggers do not cover per-sim subdirectories.** A
   Phase 12 change touching only `volumetric-grid/lattice-boltzmann/**`
   will not re-run the native build job under current triggers.

9. **`raymarch.frag.glsl` hardcodes the unit-cube `[0,1]^3` assumption** in
   the slab intersection AND the `shadow_step = sqrt(3)/N` line. A rule-of-
   three promotion that handles a non-unit-aspect domain (LBM `512×256×256`)
   would need to generalize both lines.

10. **Web research blocked.** Section E's LBM-library survey is from prior
    knowledge only; current release tags, exact licenses, and kernel LOCs
    require human verification before locking a v1 anchor. If web access is
    granted later, the four candidate-library lookups (waLBerla / OpenLB /
    Palabos / Krüger book code) can be repeated.

No design recommendations issued. Probe complete.
