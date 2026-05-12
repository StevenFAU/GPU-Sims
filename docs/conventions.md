# Coding, shader, naming, and profiling conventions

This document expands [`overarching-spec.md`](overarching-spec.md) § 8 with concrete rules. These conventions apply across every sim and every stack. Per-sim deviations require justification in the sim's spec sheet.

---

## C++ (Stack C)

- **Standard:** C++20.
- **Formatter:** `clang-format` with the project's [`.clang-format`](../.clang-format) (Google base, 100-column, 4-space indent, type-attached pointers).
- **Compiler warnings:** `-Wall -Wextra -Wpedantic`. Treat warnings as errors in CI.
- **Memory:** RAII for every GPU resource (buffers, textures, pipelines, sync objects). No raw owning pointers. `std::unique_ptr` / `std::shared_ptr` for owning, raw pointers only for non-owning observers.
- **Error handling:** Exceptions are fine for genuinely exceptional cases (file not found, GPU device lost). For expected error paths (shader compile failure during hot-reload), use `std::expected` (C++23) or a tagged result type.
- **Includes:** Project headers in `"common/..."` form; system and library headers in `<...>` form. Sorted by `clang-format`'s `IncludeBlocks: Regroup`.
- **Testing:** Each sim includes at minimum a smoke test ("does it launch and run for 60 seconds without crashing"). Algorithm-correctness tests where tractable.

## Python (Stack D)

- **Standard:** Python 3.11+.
- **Formatter and linter:** `ruff` (replaces both `black` and `flake8`). Config in `pyproject.toml`.
- **Type hints:** Required for non-trivial functions. Module-level `from __future__ import annotations` is encouraged.
- **Dependencies:** Per-sim `pyproject.toml`. Pin major versions; lock with `uv` or `poetry` lockfiles. Document GPU-specific install steps (e.g., Taichi backend selection) in the sim's README.
- **Virtual environments:** Per-sim `.venv/`. Never commit virtualenv contents.
- **Cross-backend posture (Stack D-specific).** Use `ti.init(arch=ti.gpu)` so Taichi
  picks CUDA when available, else Vulkan. The AMD RX 6800 XT desktop and the RTX
  2080 Ti lab PC are both first-class targets. CUDA-only hints
  (`ti.loop_config(block_dim=N)`) are no-ops on Vulkan; preserve them. The upstream
  MLS-MPM grid scatter via `+=` on a vector field compiles to vector-element-wise
  atomic floats and works on both backends (CUDA native; Vulkan via
  `VK_EXT_shader_atomic_float`). Avoid explicit `ti.atomic_add(vec_field[I], vec_value)`
  in favor of the implicit `+=` lowering.
- **Kernels in real files on disk.** Taichi `@ti.kernel` decoration captures the
  function's Python AST via Python's `inspect.getsource`, which requires the function
  to live in a real `.py` file. Tests that pass kernel code as strings via
  `python -c "…"` fail with "Cannot find source code for Object." All Stack D
  per-sim tests live at `tests/test_kernels.py` on disk; never collapse them into
  parametrized fixtures or string-based dynamic compilation.
- **No `np.float64` anywhere.** Taichi defaults to `ti.f32`; mismatched dtypes between
  numpy round-trip and Taichi field cause silent precision degradation and downstream
  `from_numpy` shape errors. Every numpy operation in Stack D uses `.astype(np.float32)`
  or initializes with `dtype=np.float32` explicitly. State capture writes float32
  bytes; state load reads float32.
- **No in-process kernel hot-reload.** Taichi's @ti.kernel decoration is JIT;
  editing kernel source requires a fresh Python process. Dev workflow is Ctrl+C,
  edit, rerun. `common-py` deliberately ships no HotReloader — a future
  `gpusims_common.process_watcher` (file-watcher + child-process re-exec) would be
  added once a Stack D sim demands true hot-iteration.
- **Capture format follows `docs/tier1-capture-format-reference.md`.** Stack D
  state capture inherits the cross-stack contract: top-level `state.json.meta` uses
  **exactly one** sim-namespaced wrapper key (camelCase, named after the sim);
  per-buffer fields are `{name, file, count, stride, format, shape}` (NOT
  `{bytes, dtype}`). Format strings follow the existing table (`r32f` /
  `rgba16f` / `rgba16float`); Phase 9 introduces `r32i` for int32 buffers as a
  natural extension. Tier-1 / Tier-3 diagnostics tooling reads this schema; new
  Stack D sims must conform.

## TypeScript (Stack B)

- **Strict mode:** Always. `"strict": true` in `tsconfig.json`.
- **Formatter and linter:** `prettier` + `eslint`. 2-space indent.
- **No `any` without justification.** A single-line comment explaining why is required.
- **Module system:** ES modules. No CommonJS.
- **Build tool:** Vite for development and production builds.

## Shaders

- **Languages by stack:**
  - Stack A (Shadertoy): GLSL 300 ES, single fragment shader.
  - Stack B (WebGPU): WGSL.
  - Stack C (OpenGL): GLSL 460. Stack C (Vulkan): GLSL → SPIR-V, or HLSL SM6 → SPIR-V.
- **File location:** Shader files live next to their host code (e.g., `volumetric-grid/eulerian-smoke/shaders/`).
- **Hot-reload:** All native and web sims integrate `common/`'s hot-reload utility. File changes trigger recompile; on compile failure, the previous good shader continues running.
- **Bind groups / descriptor sets:** Documented in each shader's header comment. The expected layout (binding indices, types, sizes) is the contract between host and shader.
- **Naming:**
  - Uniforms / uniform blocks: `u_camelCase`.
  - Storage buffers: `s_camelCase`.
  - Textures / images: `t_camelCase`.
  - Workgroup sizes: documented in a header comment, declared once per shader.

## Naming conventions

- **Sim folders:** `kebab-case` (e.g., `eulerian-smoke`, `sph-water`).
- **C++ classes:** `PascalCase`. Free functions: `camelCase`. Constants: `kPascalCase`.
- **C++ files:** `snake_case.cpp` / `snake_case.hpp`.
- **Python modules:** `snake_case.py`. Classes `PascalCase`, functions/variables `snake_case`.
- **TypeScript:** Files `kebab-case.ts` for modules; `PascalCase.tsx` for React components. Classes/types `PascalCase`, functions/variables `camelCase`, constants `SCREAMING_SNAKE_CASE`.

## Profiling

- **Every native sim integrates `common/profiling/`.** Per-pass GPU timings are shown in the ImGui overlay and dumpable to CSV.
- **Performance characterization is part of "done."** Each sim's README includes actual frame times at each scale tier on the dev hardware. These numbers are the regression baseline; PRs that change them update the README.
- **Profiling tools:**
  - AMD: RenderDoc, RGP (Radeon GPU Profiler), `perf` for CPU.
  - NVIDIA: Nsight Systems (timeline), Nsight Compute (kernel-level), RenderDoc.
  - Web: Chromium DevTools GPU panel, `performance.now()` for CPU.

## Documentation

- **Every sim folder has its own `README.md`** with:
  1. Title and one-line description
  2. Status (unimplemented / in-progress / shipped)
  3. Stack and target hardware
  4. Build and run instructions
  5. Controls (keyboard, mouse, UI)
  6. Screenshots and/or animated previews
  7. Performance numbers per scale tier
  8. References (papers, reference implementations, with license check)
- **Top-level `README.md`** is a gallery linking to each sim's README.
- **Per-sim spec sheets** under `docs/sim-specs/<name>.md` are the design document. They precede implementation; they are updated when the design evolves.

## Performance regression

- Performance numbers in per-sim READMEs are the baseline.
- A PR that touches simulation or rendering hot paths in a sim must either:
  1. Confirm the existing numbers still hold on the same hardware, or
  2. Update the numbers (and explain in the PR description what changed).
- CI cannot easily validate per-machine performance, so this is enforced by review.

## Commit messages

Conventional Commits format encouraged but not enforced. See [`CONTRIBUTING.md`](../CONTRIBUTING.md) for the category list.

## Reference-implementation licensing

When borrowing or adapting code from a reference implementation:
1. Check the source license. Most graphics-research code is MIT/BSD/Apache; verify per case.
2. Note the source and license in the sim's README under "References."
3. If a license requires preserving an attribution header, preserve it in the relevant source file.
4. If a license is incompatible with MIT (e.g., GPL), do not borrow code; reimplement from the paper.
