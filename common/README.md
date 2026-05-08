# common — shared infrastructure across stacks

This directory holds reusable building blocks consumed by the simulations in
the per-category folders (`closed-form/`, `volumetric-grid/`, etc.). Each
sub-package targets one stack:

| Package      | Stack                          | Status (Phase 1)               |
|--------------|--------------------------------|--------------------------------|
| `common-cpp` | C — Native C++ / Vulkan 1.3    | **Implemented**                |
| `common-web` | B — TypeScript / WebGPU        | Drafted in Phase 1.5           |
| `common-py`  | D — Python / Taichi            | Drafted with first Stack D sim |

Stack A (Shadertoy) does not have a `common-` package — single-file GLSL
sources don't share infrastructure.

## common-cpp — Vulkan 1.3 (Implemented)

Path: `common/common-cpp/`

Layout:

```
common-cpp/
├── CMakeLists.txt
├── cmake/
│   ├── deps.cmake             FetchContent: GLFW, GLM, ImGui, spdlog,
│   │                           nlohmann_json, VMA, shaderc
│   ├── optional_deps.cmake    find_package: OpenVDB, Alembic (gated)
│   ├── imgui.cmake            Custom ImGui static-lib target
│   └── vma.cmake              VMA implementation .cpp wrapper
├── include/
│   └── gpusims/               Public API consumed via <gpusims/...>
│       ├── camera.hpp
│       ├── gpu_profiler.hpp
│       ├── hot_reload.hpp
│       ├── imgui_setup.hpp
│       ├── log.hpp
│       ├── state_reader.hpp
│       ├── state_writer.hpp
│       ├── vdb_writer.hpp     OpenVDB writer (stub unless USE_OPENVDB=ON)
│       ├── alembic_writer.hpp Alembic writer (stub unless USE_ALEMBIC=ON)
│       └── vk/                Vulkan-specific abstractions
│           ├── buffer.hpp
│           ├── compute_pipeline.hpp
│           ├── context.hpp
│           ├── debug.hpp
│           ├── frame.hpp
│           ├── graphics_pipeline.hpp
│           ├── image.hpp
│           ├── renderer.hpp
│           ├── shader_compiler.hpp
│           └── window.hpp
├── src/                       Implementations (mirrors include/)
└── examples/
    └── hello/                 Reference application; copy to start a new sim
        ├── CMakeLists.txt
        ├── main.cpp
        └── shaders/
            ├── gradient.comp.glsl
            ├── fullscreen.vert.glsl
            └── fullscreen.frag.glsl
```

Per-sim consumption pattern (CMake):

```cmake
add_executable(my_sim main.cpp)
target_link_libraries(my_sim PRIVATE gpusims::common_cpp)
```

Per-sim consumption pattern (C++):

```cpp
#include <gpusims/camera.hpp>
#include <gpusims/vk/context.hpp>
#include <gpusims/vk/renderer.hpp>
#include <gpusims/vk/compute_pipeline.hpp>

gpusims::vk::Context  ctx;
gpusims::vk::Window   window(ctx, 1920, 1080, "my_sim");
gpusims::vk::Renderer renderer(ctx, window);
// ...
```

### Build dependencies

Required (Ubuntu 24.04):

```
sudo apt install build-essential cmake ninja-build git \
    libvulkan-dev vulkan-validationlayers \
    spirv-tools glslang-tools \
    libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev \
    libwayland-dev libxkbcommon-dev libgl1-mesa-dev
```

Optional (enable as needed):

```
sudo apt install libopenvdb-dev libalembic-dev
```

Configure with optional deps:

```
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DGPU_SIMS_USE_OPENVDB=ON \
    -DGPU_SIMS_USE_ALEMBIC=ON
```

### Hello-world

The example at `examples/hello/` exercises every Phase 1 subsystem end-to-end
(Vulkan context + window + swapchain, compute pipeline, graphics pipeline,
camera, hot-reload with include-graph tracking, GPU profiler with ring-
buffered timestamp queries, state save/load, ImGui). Build it with:

```
cmake -S . -B build -G Ninja -DGPU_SIMS_BUILD_EXAMPLES=ON
cmake --build build
./build/bin/gpu_sims_hello
```

Controls: WASDQE to move, hold right mouse to look, hold shift to boost.
F5 saves a state capture; F9 reloads the most recent. Edit any of the three
shader files while the program is running — pipelines reload on save. A
green toast confirms success; a red toast with the GLSL compiler error
appears on failure (and the previous shader stays bound).

## common-web — WebGPU / TypeScript (Phase 1.5)

Will live at `common/common-web/`. Drafted as a separate phase before the
first Stack B sim implementation (strange-attractors).

## common-py — Python / Taichi (Drafted with first Stack D sim)

Will live at `common/common-py/`. The first Stack D sim is most likely
`lenia-fft` or `mpm-multimaterial`; the package is created at that point.
