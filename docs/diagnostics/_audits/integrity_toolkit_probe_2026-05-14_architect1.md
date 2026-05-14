---
title: Integrity-Toolkit Spec-Preparation Probe (read-only)
date: 2026-05-14
author: architect1
phase: integrity-toolkit-prep
status: probe
scope: read-only — no file modifications, no commits, no builds, no binary runs
audience: architect-1 (spec drafter for cross-stack integrity toolkit)
out-of-scope:
  - tool choice or recommendation
  - any code modification
  - any build invocation
sibling-context:
  - Phase 11.5 Setup-1 (`phase11_5_setup1_2026-05-14_setup1.md`) — the Convention #8 fabrication that motivates this toolkit
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  - Layer-3 prioritization (`sims_prioritization_2026-05-14_triage.md`) — flags lenia-fft `Chakazul/Lenia/Python/LeniaNDK.py:329-335` as the next same-shape fabrication smell
---

> Read-only probe collecting ground-truth repo state for an architect drafting the integrity-toolkit spec.
> Verbatim quotation; "absent" / "none found" reported explicitly where the surface does not exist.

---

## Section A: Repository layout

### `ls -la` (top-level)

```text
drwxrwxr-x  4 otacon otacon   4096 May  8 14:28 agent-based
drwxrwxr-x  2 otacon otacon   4096 May 13 18:00 alembic_export
drwxrwxr-x 11 otacon otacon   4096 May 14 13:38 build
drwxrwxr-x 10 otacon otacon   4096 May 13 17:28 build-test-alembic
drwxrwxr-x  4 otacon otacon   4096 May 11 13:17 captures
-rw-rw-r--  1 otacon otacon  44689 May 13 17:07 CHANGELOG.md
-rw-rw-r--  1 otacon otacon    599 May  8 14:19 CITATION.cff
-rw-rw-r--  1 otacon otacon   1037 May  8 14:18 .clang-format
drwxrwxr-x  2 otacon otacon   4096 May 14 13:51 .claude
drwxrwxr-x  4 otacon otacon   4096 May  8 14:28 closed-form
-rw-rw-r--  1 otacon otacon   4005 May 13 13:58 CMakeLists.txt
-rw-rw-r--  1 otacon otacon   2883 May  8 14:19 CODE_OF_CONDUCT.md
drwxrwxr-x  5 otacon otacon   4096 May 12 08:16 common
drwxrwxr-x  6 otacon otacon   4096 May  9 09:54 continuous-ca
-rw-rw-r--  1 otacon otacon   2295 May  8 14:18 CONTRIBUTING.md
drwxrwxr-x  5 otacon otacon   4096 May 14 09:29 docs
-rw-rw-r--  1 otacon otacon    701 May  8 14:18 .editorconfig
drwxrwxr-x  2 otacon otacon   4096 May 10 18:06 gallery
drwxrwxr-x  8 otacon otacon   4096 May 14 14:42 .git
-rw-rw-r--  1 otacon otacon   1702 May  8 14:18 .gitattributes
drwxrwxr-x  4 otacon otacon   4096 May  8 14:19 .github
-rw-rw-r--  1 otacon otacon   2908 May 14 13:34 .gitignore
drwxrwxr-x  3 otacon otacon   4096 May  8 14:28 hybrid-particle-grid
-rw-rw-r--  1 otacon otacon    496 May 14 13:40 imgui.ini
-rw-rw-r--  1 otacon otacon   1069 May  8 14:18 LICENSE
-rw-rw-r--  1 otacon otacon    317 May  9 11:25 lychee.toml
-rw-rw-r--  1 otacon otacon    258 May 11 22:22 .markdownlint.json
drwxrwxr-x 24 otacon otacon   4096 May 10 20:04 node_modules
-rw-rw-r--  1 otacon otacon    876 May  8 20:02 package.json
-rw-rw-r--  1 otacon otacon  40267 May 10 20:04 package-lock.json
drwxrwxr-x  4 otacon otacon   4096 May  8 14:28 particle-fluids
-rw-rw-r--  1 otacon otacon 123625 May 13 17:22 project-state.md
drwxrwxr-x  3 otacon otacon   4096 May  8 14:28 quantum
-rw-rw-r--  1 otacon otacon   6827 May 13 17:06 README.md
drwxrwxr-x  3 otacon otacon   4096 May 14 11:10 references
drwxrwxr-x  5 otacon otacon   4096 May  8 14:30 render-pipelines
-rw-rw-r--  1 otacon otacon   1195 May  8 14:19 SECURITY.md
-rw-rw-r--  1 otacon otacon    712 May 11 21:19 tsconfig.shared.json
drwxrwxr-x  2 otacon otacon  12288 May 11 18:39 vdb_export
drwxrwxr-x  4 otacon otacon   4096 May  9 10:08 volumetric-grid
drwxrwxr-x  2 otacon otacon   4096 May  8 14:30 web
```

### Depth-2 directory tree (excluding `.*`, `node_modules`, `build*`, `references`)

```text
.
./agent-based
./agent-based/boids-3d
./agent-based/physarum
./alembic_export
./captures
./captures/capture_0000
./captures/capture_0094
./closed-form
./closed-form/mandelbulb-explorer
./closed-form/strange-attractors
./common
./common/common-cpp
./common/common-py
./common/common-web
./continuous-ca
./continuous-ca/lenia-fft
./continuous-ca/neural-ca
./continuous-ca/reaction-diffusion-2d
./continuous-ca/reaction-diffusion-3d
./docs
./docs/diagnostics
./docs/retro
./docs/sim-specs
./gallery
./hybrid-particle-grid
./hybrid-particle-grid/mpm-multimaterial
./particle-fluids
./particle-fluids/pic-flip
./particle-fluids/sph-water
./quantum
./quantum/ising-dwave
./render-pipelines
./render-pipelines/blender
./render-pipelines/houdini
./render-pipelines/optix
./vdb_export
./volumetric-grid
./volumetric-grid/eulerian-smoke
./volumetric-grid/lattice-boltzmann
./web
```

**Observations** (factual, no recommendations):

- Three sim categories per `docs/conventions.md` and `docs/overarching-spec.md` map to top-level dirs (`agent-based/`, `closed-form/`, `continuous-ca/`, `hybrid-particle-grid/`, `particle-fluids/`, `quantum/`, `volumetric-grid/`). The Stack categorization (B/C/D) is per-sim, not per-top-level-dir.
- `common/` has three subdirs: `common-cpp/`, `common-py/`, `common-web/` — one per stack.
- No top-level `scripts/`, `tools/`, or `bin/` directory exists.
- `references/` exists with one child (`SPlisHSPlasH/`).
- `build/`, `build-test-alembic/`, `alembic_export/`, `vdb_export/`, `captures/` are runtime-output / build-output trees (`build/` and `build-test-alembic/` are CMake build directories; the others are sim-runtime capture/export targets). All are listed in `.gitignore`.

---

## Section B: Existing CI configuration

### `ls .github/workflows/`

```text
build-native.yml
build-py.yml
build-web.yml
deploy-pages.yml
markdown.yml
structure.yml
```

### `.github/workflows/build-native.yml` (verbatim)

```yaml:.github/workflows/build-native.yml
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

### `.github/workflows/build-py.yml` (verbatim)

```yaml:.github/workflows/build-py.yml
name: Build (py)

on:
  push:
    paths:
      - 'common/common-py/**'
      - 'hybrid-particle-grid/mpm-multimaterial/python/**'
      - 'continuous-ca/lenia-fft/python/**'
      - '.github/workflows/build-py.yml'
  pull_request:
    paths:
      - 'common/common-py/**'
      - 'hybrid-particle-grid/mpm-multimaterial/python/**'
      - 'continuous-ca/lenia-fft/python/**'
      - '.github/workflows/build-py.yml'
  workflow_dispatch:

jobs:
  build-py:
    name: Python 3.11 / ruff / mypy --strict / Taichi CPU smoke
    runs-on: ubuntu-24.04

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Set up Python 3.11
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'
          cache: 'pip'

      - name: Install common-py + dev deps
        working-directory: common/common-py
        run: |
          python -m pip install --upgrade pip
          pip install -e .[dev]

      - name: Ruff (lint)
        working-directory: common/common-py
        run: ruff check .

      - name: Mypy --strict (type-check)
        working-directory: common/common-py
        run: mypy --strict gpusims_common

      - name: Import smoke
        working-directory: common/common-py
        run: |
          python -c "from gpusims_common import Camera, CameraMode, ParamPanel, StateWriter, StateReader, VdbWriter, AlembicWriter, ParticleFrame, log; print('ok')"

      - name: Taichi CPU-backend kernel smoke
        working-directory: common/common-py
        run: pytest tests/ -v

      - name: Install mpm-multimaterial sim deps
        working-directory: hybrid-particle-grid/mpm-multimaterial/python
        run: |
          pip install -e .

      - name: Ruff (sim)
        working-directory: hybrid-particle-grid/mpm-multimaterial/python
        run: ruff check .

      - name: Mypy --strict (sim)
        working-directory: hybrid-particle-grid/mpm-multimaterial/python
        run: mypy --strict .

      - name: Taichi CPU-backend kernel smoke (sim)
        working-directory: hybrid-particle-grid/mpm-multimaterial/python
        run: pytest tests/ -v

      - name: Install lenia-fft sim deps
        working-directory: continuous-ca/lenia-fft/python
        run: |
          pip install -e .

      - name: Ruff (lenia)
        working-directory: continuous-ca/lenia-fft/python
        run: ruff check .

      - name: Mypy --strict (lenia)
        working-directory: continuous-ca/lenia-fft/python
        run: mypy --strict .

      - name: Taichi CPU-backend kernel smoke (lenia)
        working-directory: continuous-ca/lenia-fft/python
        run: pytest tests/ -v

  combined-install-smoke:
    name: Combined-install smoke (all Stack D sims)
    runs-on: ubuntu-24.04
    needs: [build-py]

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Set up Python 3.11
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'
          cache: 'pip'

      - name: Install all Stack D sims in one venv
        run: |
          python -m pip install --upgrade pip
          pip install -e ./common/common-py[dev]
          pip install -e ./common/common-py/examples/hello
          pip install -e ./hybrid-particle-grid/mpm-multimaterial/python
          pip install -e ./continuous-ca/lenia-fft/python

      - name: Verify cross-package isolation
        run: |
          python - <<'PY'
          from hello.main import *  # noqa: F401,F403
          from mpm_multimaterial.main import SimState as MPMSimState
          from mpm_multimaterial import kernels as mpm_kernels, presets as mpm_presets
          from lenia_fft.main import SimState as LeniaSimState
          from lenia_fft import kernels as lenia_kernels, presets as lenia_presets, fft_backend as lenia_fft_backend  # noqa: F401
          assert MPMSimState is not LeniaSimState, "SimState collision"
          assert mpm_kernels is not lenia_kernels, "kernels collision"
          assert mpm_presets is not lenia_presets, "presets collision"
          print("Stack D sim isolation verified across combined install")
          PY

      - name: Run mpm-multimaterial pytest in combined venv
        working-directory: hybrid-particle-grid/mpm-multimaterial/python
        run: pytest tests/ -v --no-header

      - name: Run lenia-fft pytest in combined venv
        working-directory: continuous-ca/lenia-fft/python
        run: pytest tests/ -v --no-header
```

### `.github/workflows/build-web.yml` (verbatim)

```yaml:.github/workflows/build-web.yml
name: Build (web)

on:
  push:
    paths:
      - 'package.json'
      - 'common/common-web/**'
      - '.github/workflows/build-web.yml'
  pull_request:
    paths:
      - 'package.json'
      - 'common/common-web/**'
      - '.github/workflows/build-web.yml'
  workflow_dispatch:

jobs:
  build-web:
    name: Node 22 / TypeScript / Vite build
    runs-on: ubuntu-24.04

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Set up Node.js 22
        uses: actions/setup-node@v4
        with:
          node-version: '22'
          cache: 'npm'

      - name: Install workspace dependencies
        run: npm ci

      - name: Type-check (strict)
        run: npm run typecheck

      - name: Build common-web library
        run: npm run build --workspace=@gpusims/common-web

      - name: Build hello-world example
        run: npm run build --workspace=@gpusims/hello-web

      - name: List built artifacts
        run: |
          echo "=== common-web dist ==="
          ls -la common/common-web/dist/ || true
          echo "=== hello-web dist ==="
          ls -la common/common-web/examples/hello/dist/ || true
```

### `.github/workflows/deploy-pages.yml` (verbatim)

```yaml:.github/workflows/deploy-pages.yml
name: Deploy GitHub Pages

on:
  push:
    branches: [main]
    paths:
      - 'package.json'
      - 'common/common-web/**'
      - 'closed-form/**/web/**'
      - 'agent-based/**/web/**'
      - 'continuous-ca/**/web/**'
      - 'volumetric-grid/**/web/**'
      - 'particle-fluids/**/web/**'
      - 'hybrid-particle-grid/**/web/**'
      - 'quantum/**/web/**'
      - 'gallery/**'
      - '.github/workflows/deploy-pages.yml'
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

concurrency:
  group: pages
  cancel-in-progress: false

jobs:
  build:
    name: Build sites
    runs-on: ubuntu-24.04
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Set up Node.js 22
        uses: actions/setup-node@v4
        with:
          node-version: '22'
          cache: 'npm'

      - name: Install workspace dependencies
        run: npm ci

      - name: Type-check workspaces
        run: npm run typecheck

      - name: Build common-web
        run: npm run build --workspace=@gpusims/common-web

      - name: Build hello-web
        run: npm run build --workspace=@gpusims/hello-web

      - name: Build strange-attractors-web
        run: npm run build --workspace=@gpusims/strange-attractors-web

      - name: Build mandelbulb-explorer-web
        run: npm run build --workspace=@gpusims/mandelbulb-explorer-web

      - name: Build reaction-diffusion-2d-web
        run: npm run build --workspace=@gpusims/reaction-diffusion-2d-web

      - name: Build physarum-web
        run: npm run build --workspace=@gpusims/physarum-web

      - name: Build boids-3d
        run: npm run build --workspace=@gpusims/boids-3d-web

      - name: Assemble deploy tree
        run: |
          mkdir -p _site
          # Gallery → site root
          cp gallery/index.html _site/index.html
          # hello-web → /hello/
          mkdir -p _site/hello
          cp -r common/common-web/examples/hello/dist/* _site/hello/
          # strange-attractors → /strange-attractors/
          mkdir -p _site/strange-attractors
          cp -r closed-form/strange-attractors/web/dist/* _site/strange-attractors/
          # mandelbulb-explorer → /mandelbulb-explorer/
          mkdir -p _site/mandelbulb-explorer
          cp -r closed-form/mandelbulb-explorer/web/dist/* _site/mandelbulb-explorer/
          # reaction-diffusion-2d → /reaction-diffusion-2d/
          mkdir -p _site/reaction-diffusion-2d
          cp -r continuous-ca/reaction-diffusion-2d/web/dist/* _site/reaction-diffusion-2d/
          # physarum → /physarum/
          mkdir -p _site/physarum
          cp -r agent-based/physarum/web/dist/* _site/physarum/
          # boids-3d → /boids-3d/
          mkdir -p _site/boids-3d
          cp -r agent-based/boids-3d/web/dist/* _site/boids-3d/
          # Copy boids-3d dist (anchor: spec § 5.2)
          # Show what we built
          echo "=== _site contents ==="
          find _site -maxdepth 2 -type f | sort

      - name: Upload Pages artifact
        uses: actions/upload-pages-artifact@v3
        with:
          path: _site

  deploy:
    name: Deploy
    needs: build
    runs-on: ubuntu-24.04
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    steps:
      - name: Deploy to GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
```

### `.github/workflows/markdown.yml` (verbatim)

```yaml:.github/workflows/markdown.yml
name: Markdown

on:
  push:
    branches: [main]
    paths:
      - '**/*.md'
      - '.github/workflows/markdown.yml'
  pull_request:
    paths:
      - '**/*.md'
      - '.github/workflows/markdown.yml'

permissions:
  contents: read

jobs:
  lint:
    name: Lint markdown
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: DavidAnson/markdownlint-cli2-action@v16
        with:
          globs: |
            **/*.md
            !node_modules
            !**/build
            !**/dist

  link-check:
    name: Check internal links
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Check links
        uses: lycheeverse/lychee-action@v1
        with:
          args: >-
            --offline
            --no-progress
            --include-fragments
            --exclude-path node_modules
            --exclude-path build
            --exclude-path dist
            --exclude-path docs/sim-specs/_template.md
            './**/*.md'
          fail: true
```

### `.github/workflows/structure.yml` (verbatim, 99 lines)

```yaml:.github/workflows/structure.yml
name: Structure

on:
  push:
    branches: [main]
  pull_request:

permissions:
  contents: read

jobs:
  validate:
    name: Validate repo structure
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Verify required directories exist
        shell: bash
        run: |
          set -e
          required_dirs=(
            docs
            docs/sim-specs
            common
            volumetric-grid/eulerian-smoke
            volumetric-grid/lattice-boltzmann
            particle-fluids/sph-water
            particle-fluids/pic-flip
            hybrid-particle-grid/mpm-multimaterial
            continuous-ca/reaction-diffusion-2d
            continuous-ca/lenia-fft
            continuous-ca/neural-ca
            agent-based/physarum
            agent-based/boids-3d
            closed-form/mandelbulb-explorer
            closed-form/strange-attractors
            quantum/ising-dwave
            render-pipelines/blender
            render-pipelines/houdini
            render-pipelines/optix
            web
          )
          for d in "${required_dirs[@]}"; do
            if [ ! -d "$d" ]; then
              echo "MISSING DIR: $d"
              exit 1
            fi
          done
          echo "All required directories present."
      - name: Verify required top-level files exist
        shell: bash
        run: |
          set -e
          required_files=(
            LICENSE
            README.md
            .gitignore
            .gitattributes
            .editorconfig
            .clang-format
            CONTRIBUTING.md
            CODE_OF_CONDUCT.md
            SECURITY.md
            CITATION.cff
            CHANGELOG.md
            docs/overarching-spec.md
            docs/root-context-distilled.md
            docs/hardware.md
            docs/stack-decisions.md
            docs/conventions.md
            docs/sim-specs/_template.md
          )
          for f in "${required_files[@]}"; do
            if [ ! -f "$f" ]; then
              echo "MISSING FILE: $f"
              exit 1
            fi
          done
          echo "All required top-level files present."
      - name: Verify all sim-spec stubs exist
        shell: bash
        run: |
          set -e
          sims=(
            strange-attractors mandelbulb-explorer
            physarum boids-3d
            reaction-diffusion-2d reaction-diffusion-3d lenia-fft neural-ca
            eulerian-smoke lattice-boltzmann
            sph-water pic-flip
            mpm-multimaterial
            ising-dwave
          )
          for s in "${sims[@]}"; do
            if [ ! -f "docs/sim-specs/$s.md" ]; then
              echo "MISSING SIM SPEC: docs/sim-specs/$s.md"
              exit 1
            fi
          done
          echo "All sim-spec stubs present."
```

### Local hook / pre-commit state

- `.pre-commit-config.yaml` — **does not exist** (no file at that path).
- `.githooks/` — **does not exist** (no directory).
- `.git/hooks/` — exists but contains only the default git-installed `.sample` files (`applypatch-msg.sample`, `commit-msg.sample`, `fsmonitor-watchman.sample`, `post-update.sample`, `pre-applypatch.sample`, `pre-commit.sample`, `pre-merge-commit.sample`, `prepare-commit-msg.sample`, `pre-push.sample`, `pre-rebase.sample`, `pre-receive.sample`, `push-to-checkout.sample`, `sendemail-validate.sample`, `update.sample`). No active (`-sample`-less) hook is installed.

There is no local pre-commit infrastructure. All gating runs in GitHub Actions.

---

## Section C: Existing test infrastructure per stack

### C.1 Stack C (C++/Vulkan)

#### Files under `common/common-cpp/tests` and `common/common-cpp/examples`

```text
common/common-cpp/examples/hello/CMakeLists.txt
common/common-cpp/examples/hello/main.cpp
common/common-cpp/examples/hello/shaders/fullscreen.vert.glsl
common/common-cpp/examples/hello/shaders/trivial.comp.glsl
common/common-cpp/examples/hello/shaders/fullscreen.frag.glsl
common/common-cpp/examples/hello/shaders/gradient.comp.glsl
```

- `common/common-cpp/tests/` — **does not exist**.
- No standalone Stack C test executable, test directory, or test entry point exists. The only built artifact in CI's "List built artifacts" step is `build/common/common-cpp/examples/hello/`.

#### Top-level CMakeLists in `*/tests/` (repo source only, excluding `build*/_deps`)

```text
(no matches)
```

All `*/tests/CMakeLists.txt` matches under `build/_deps/...` and `build-test-alembic/_deps/...` are third-party (`glfw`, `spdlog`, `nlohmann_json`); none are repo-source test scaffolding.

#### `enable_testing` / `add_test` / `gtest` / `catch2` / `ctest` in repo CMakeLists.txt

```text
(no matches in any non-`build*/` CMakeLists.txt under the repo source tree)
```

The grep returns no hits across `CMakeLists.txt`, `common/common-cpp/CMakeLists.txt`, or per-sim CMakeLists. **Stack C has no test framework wired into the build.** The native build only verifies "examples link and binaries land on disk." No CTest registration, no GoogleTest, no Catch2.

### C.2 Stack B (TypeScript/WebGPU)

#### `find . -name 'package.json' -not -path '*/node_modules/*'`

Repo-source package.json files (excluding `node_modules/`, `build*/_deps/`, `dist/`):

```text
./package.json
./closed-form/mandelbulb-explorer/web/package.json
./continuous-ca/reaction-diffusion-2d/web/package.json
./agent-based/boids-3d/web/package.json
./agent-based/physarum/web/package.json
./closed-form/strange-attractors/web/package.json
./common/common-web/package.json
./common/common-web/examples/hello/package.json
```

#### `./package.json` (root workspace, verbatim)

```json:package.json
{
  "name": "gpu-sims",
  "version": "0.1.0",
  "private": true,
  "description": "GPU-accelerated physics and emergence simulations — workspace root",
  "type": "module",
  "engines": {
    "node": ">=22"
  },
  "workspaces": [
    "common/common-web",
    "common/common-web/examples/*",
    "closed-form/*/web",
    "agent-based/*/web",
    "continuous-ca/*/web",
    "volumetric-grid/*/web",
    "particle-fluids/*/web",
    "hybrid-particle-grid/*/web",
    "quantum/*/web"
  ],
  "scripts": {
    "build:web": "npm run build --workspace=@gpusims/common-web --if-present && npm run build --workspaces --if-present --include-workspace-root=false",
    "dev:hello-web": "npm run dev --workspace=@gpusims/hello-web",
    "typecheck": "npm run typecheck --workspaces --if-present --include-workspace-root=false"
  },
  "devDependencies": {
    "typescript": "^5.6.0"
  }
}
```

#### `common/common-web/package.json` (verbatim)

```json:common/common-web/package.json
{
  "name": "@gpusims/common-web",
  "version": "0.1.0",
  "private": true,
  "description": "Shared WebGPU + TypeScript infrastructure for GPU-Sims Stack B simulations",
  "type": "module",
  "main": "./src/index.ts",
  "types": "./src/index.ts",
  "exports": {
    ".": {
      "types": "./src/index.ts",
      "import": "./src/index.ts"
    },
    "./vite-plugin": {
      "types": "./src/viteWgslPlugin.ts",
      "import": "./src/viteWgslPlugin.ts"
    }
  },
  "scripts": {
    "typecheck": "tsc --noEmit",
    "build": "tsc -p tsconfig.build.json",
    "clean": "rm -rf dist"
  },
  "dependencies": {
    "lil-gui": "^0.20.0",
    "fflate": "^0.8.2",
    "gl-matrix": "^3.4.3"
  },
  "devDependencies": {
    "@types/node": "^22.0.0",
    "@webgpu/types": "^0.1.50",
    "typescript": "^5.6.0",
    "vite": "^7.0.0"
  },
  "peerDependencies": {
    "vite": "^7.0.0"
  },
  "peerDependenciesMeta": {
    "vite": {
      "optional": true
    }
  }
}
```

#### Per-sim Stack B package.json — `scripts` + dependency surfaces (verbatim)

`closed-form/strange-attractors/web/package.json`:

```json:closed-form/strange-attractors/web/package.json
{
  "name": "@gpusims/strange-attractors-web",
  "version": "0.1.0",
  "private": true,
  "description": "Strange Attractors — first GPU-Sims Stack B sim. 2M particles integrating Lorenz/Aizawa/Thomas ODEs via RK4 with HDR additive accumulation.",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "tsc --noEmit && vite build",
    "preview": "vite preview",
    "typecheck": "tsc --noEmit"
  },
  "dependencies": {
    "@gpusims/common-web": "^0.1.0"
  },
  "devDependencies": {
    "@webgpu/types": "^0.1.50",
    "typescript": "^5.6.0",
    "vite": "^7.0.0"
  }
}
```

`closed-form/mandelbulb-explorer/web/package.json`:

```json:closed-form/mandelbulb-explorer/web/package.json
{
  "name": "@gpusims/mandelbulb-explorer-web",
  "version": "0.1.0",
  "private": true,
  "description": "Mandelbulb Explorer — second GPU-Sims Stack B sim. Distance-estimator raymarcher with soft shadows and orbit-trap coloring. First Stack A → B port in the repo.",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "tsc --noEmit && vite build",
    "preview": "vite preview",
    "typecheck": "tsc --noEmit"
  },
  "dependencies": {
    "@gpusims/common-web": "^0.1.0"
  },
  "devDependencies": {
    "@webgpu/types": "^0.1.50",
    "typescript": "^5.6.0",
    "vite": "^7.0.0"
  }
}
```

`continuous-ca/reaction-diffusion-2d/web/package.json`:

```json:continuous-ca/reaction-diffusion-2d/web/package.json
{
  "name": "@gpusims/reaction-diffusion-2d-web",
  "version": "0.1.0",
  "private": true,
  "description": "Reaction-Diffusion 2D — third GPU-Sims Stack B sim. Gray-Scott pattern explorer with six Pearson 1993 named presets, mouse-paint brush, and full-state capture. Second Stack A → B port; first Stack B sim with compute ping-pong on persistent 2D state.",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "tsc --noEmit && vite build",
    "preview": "vite preview",
    "typecheck": "tsc --noEmit"
  },
  "dependencies": {
    "@gpusims/common-web": "^0.1.0"
  },
  "devDependencies": {
    "@webgpu/types": "^0.1.50",
    "typescript": "^5.6.0",
    "vite": "^7.0.0"
  }
}
```

`agent-based/physarum/web/package.json`:

```json:agent-based/physarum/web/package.json
{
  "name": "@gpusims/physarum-web",
  "version": "0.7.0",
  "private": true,
  "description": "Physarum — first agent-system Stack B sim. Multi-species slime-mold transport networks (~10M agents, three-species mutual repulsion, persistent food-source pins). First user of atomic<u32> storage buffers in the repo.",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "tsc --noEmit && vite build",
    "preview": "vite preview",
    "typecheck": "tsc --noEmit"
  },
  "dependencies": {
    "@gpusims/common-web": "^0.1.0"
  },
  "devDependencies": {
    "@webgpu/types": "^0.1.50",
    "typescript": "^5.6.0",
    "vite": "^7.0.0"
  }
}
```

`agent-based/boids-3d/web/package.json`:

```json:agent-based/boids-3d/web/package.json
{
    "name": "@gpusims/boids-3d-web",
    "version": "0.1.0",
    "private": true,
    "type": "module",
    "scripts": {
        "dev": "vite --port 5178",
        "build": "vite build",
        "typecheck": "tsc --noEmit"
    },
    "dependencies": {
        "@gpusims/common-web": "^0.1.0",
        "gl-matrix": "^3.4.3",
        "lil-gui": "^0.20.0"
    },
    "devDependencies": {
        "vite": "^7.0.0",
        "typescript": "^5.6.0",
        "@types/node": "^22.0.0"
    }
}
```

`common/common-web/examples/hello/package.json`:

```json:common/common-web/examples/hello/package.json
{
  "name": "@gpusims/hello-web",
  "version": "0.1.0",
  "private": true,
  "description": "common-web hello-world reference application",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "tsc --noEmit && vite build",
    "preview": "vite preview",
    "typecheck": "tsc --noEmit"
  },
  "dependencies": {
    "@gpusims/common-web": "^0.1.0"
  },
  "devDependencies": {
    "@webgpu/types": "^0.1.50",
    "typescript": "^5.6.0",
    "vite": "^7.0.0"
  }
}
```

#### Root test-config files

```text
package.json     present (above)
tsconfig.json    not present
vitest.config.*  not present
jest.config.*    not present
```

Only `tsconfig.shared.json` (at root) and per-package `tsconfig.json` files exist:

```text
./tsconfig.shared.json
./continuous-ca/reaction-diffusion-2d/web/tsconfig.json
./closed-form/mandelbulb-explorer/web/tsconfig.json
./closed-form/strange-attractors/web/tsconfig.json
./common/common-web/tsconfig.build.json
./common/common-web/tsconfig.json
./agent-based/boids-3d/web/tsconfig.json
./agent-based/physarum/web/tsconfig.json
./common/common-web/examples/hello/tsconfig.json
```

**Stack B summary:** No `vitest`, `jest`, `mocha`, or `playwright` appears anywhere in `dependencies` or `devDependencies`. The only gating per sim is `tsc --noEmit && vite build`. No runtime test framework is established.

### C.3 Stack D (Python/Taichi)

#### Repo-source `pyproject.toml` / `setup.py` (excluding `build*/_deps`, `.venv`, `references/`)

```text
./common/common-py/pyproject.toml
./common/common-py/examples/hello/pyproject.toml
./continuous-ca/lenia-fft/python/pyproject.toml
./hybrid-particle-grid/mpm-multimaterial/python/pyproject.toml
```

(`./references/SPlisHSPlasH/setup.py` and friends are upstream-vendored, not Stack D source.)

#### `common/common-py/pyproject.toml` (verbatim)

```toml:common/common-py/pyproject.toml
[build-system]
requires = ["setuptools>=68", "wheel"]
build-backend = "setuptools.build_meta"

[project]
name = "gpusims-common-py"
version = "0.1.0"
description = "Shared Stack D infrastructure for GPU-Sims (Python / Taichi)"
authors = [{ name = "Steven Cohen" }]
license = { text = "MIT" }
readme = "README.md"
requires-python = ">=3.11"
dependencies = [
    "taichi>=1.7.4,<1.8",
    "numpy>=1.26,<3",
]

[project.optional-dependencies]
dev = [
    "ruff>=0.6,<1.0",
    "mypy>=1.11,<2",
    "pytest>=8,<9",
]

[tool.setuptools]
packages = ["gpusims_common"]
package-dir = { "" = "." }

[tool.setuptools.package-data]
gpusims_common = ["py.typed"]

[tool.ruff]
line-length = 120
target-version = "py311"

[tool.ruff.lint]
select = ["E", "F", "W", "I", "UP", "B", "C4", "PIE", "RUF"]
ignore = [
    "E501",  # line-length handled by formatter; long Taichi kernel lines are OK
    "B008",  # function calls in argument defaults are idiomatic with Taichi types
    "E741",  # `for I in ti.grouped(...)` is canonical Taichi multi-dim index naming
]

[tool.mypy]
python_version = "3.11"
strict = true
warn_unused_ignores = true
disallow_untyped_defs = true
disallow_incomplete_defs = true
check_untyped_defs = true
no_implicit_optional = true
warn_redundant_casts = true
warn_unreachable = true
# Taichi has incomplete stubs; allow Any from taichi imports
[[tool.mypy.overrides]]
module = ["taichi.*", "pyopenvdb.*"]
ignore_missing_imports = true

[[tool.mypy.overrides]]
# @ti.kernel decorations strip return annotations (Taichi 1.7.4 rejects -> None
# at decoration time) and use ti.template() argument annotations in call form
# (Taichi runtime requirement; mypy --strict's valid-type wants subscript form).
# Relaxing strict on test modules that exercise @ti.kernel.
module = ["tests.test_kernels"]
disallow_untyped_defs = false
disallow_untyped_decorators = false
disable_error_code = ["valid-type", "no-untyped-def"]

[tool.pytest.ini_options]
testpaths = ["tests"]
python_files = ["test_*.py"]
```

#### `hybrid-particle-grid/mpm-multimaterial/python/pyproject.toml` (verbatim)

```toml:hybrid-particle-grid/mpm-multimaterial/python/pyproject.toml
[build-system]
requires = ["setuptools>=68", "wheel"]
build-backend = "setuptools.build_meta"

[project]
name = "gpusims-mpm-multimaterial-py"
version = "0.1.0"
description = "MLS-MPM multi-material (water + jelly + snow) using Taichi"
requires-python = ">=3.11"
dependencies = [
    # gpusims-common-py is installed editable by the prior CI step
    # (.github/workflows/build-py.yml: "Install common-py + dev deps"),
    # and locally by `pip install -e ../../../common/common-py` before
    # running `pip install -e .` in this directory. The plain-name dep
    # resolves to the editable install. (PEP 508 file:// URLs with
    # relative paths are rejected by modern pip — banked Phase 9 retro.)
    "gpusims-common-py",
    "taichi>=1.7.4,<1.8",
    "numpy>=1.26,<3",
]

[project.optional-dependencies]
dev = [
    "ruff>=0.6,<1.0",
    "mypy>=1.11,<2",
    "pytest>=8,<9",
]

[tool.setuptools]
packages = ["mpm_multimaterial"]

[tool.ruff]
line-length = 120
target-version = "py311"

[tool.ruff.lint]
select = ["E", "F", "W", "I", "UP", "B", "C4", "PIE", "RUF"]
ignore = [
    "E501",  # long kernel lines are OK
    "B008",  # function calls in argument defaults are idiomatic with Taichi types
    "E741",  # `for I in ti.grouped(...)` is canonical Taichi multi-dim index naming
]

[tool.mypy]
python_version = "3.11"
strict = true
warn_unused_ignores = true
[[tool.mypy.overrides]]
module = ["taichi.*", "pyopenvdb.*"]
ignore_missing_imports = true

[[tool.mypy.overrides]]
# @ti.kernel functions diverge from Python type semantics in multiple ways:
# - Taichi 1.7.4 rejects `-> None` return annotations at decoration time.
# - `ti.template()` arg annotation is call-form (Taichi runtime requirement);
#   mypy --strict valid-type wants subscript form.
# - Inside @ti.kernel scope, `int(vec)` returns ti.Vector[int] (with .cast()
#   method); mypy infers Python int (no .cast). Same gap, different mypy code.
# Cannot satisfy both Taichi runtime and mypy --strict simultaneously; relax
# the affected mypy rules on kernel + kernel-test modules. Non-kernel modules
# of the sim keep full mypy --strict.
module = ["mpm_multimaterial.kernels", "tests.test_kernels"]
disallow_untyped_defs = false
disallow_untyped_decorators = false
disable_error_code = ["valid-type", "no-untyped-def", "attr-defined"]

[tool.pytest.ini_options]
testpaths = ["tests"]
python_files = ["test_*.py"]
```

#### `continuous-ca/lenia-fft/python/pyproject.toml` (verbatim)

```toml:continuous-ca/lenia-fft/python/pyproject.toml
[build-system]
requires = ["setuptools>=68", "wheel"]
build-backend = "setuptools.build_meta"

[project]
name = "gpusims-lenia-fft-py"
version = "0.1.0"
description = "Lenia (FFT / real-space) — Bert Chan's continuous CA, 2D + opt-in 3D"
requires-python = ">=3.11"
dependencies = [
    # gpusims-common-py is installed editable by the prior CI step
    # (.github/workflows/build-py.yml: "Install common-py + dev deps"),
    # and locally by `pip install -e ../../../common/common-py` before
    # running `pip install -e .` in this directory. The plain-name dep
    # resolves to the editable install. (PEP 508 file:// URLs with
    # relative paths are rejected by modern pip — banked Phase 9 retro.)
    "gpusims-common-py",
    "taichi>=1.7.4,<1.8",
    "numpy>=1.26,<3",
]

[project.optional-dependencies]
# GPU-FFT backend extras. Pick ONE that matches your hardware.
# CuPy (NVIDIA CUDA): pip install -e .[cuda]
# PyTorch ROCm (AMD): pip install -e .[rocm]  — also requires the ROCm wheel
#                     installed separately via the PyTorch-recommended index URL.
# PyTorch CUDA (NVIDIA alternative to CuPy): pip install -e .[cuda-torch]
# All three are optional; absent them, the sim falls back to Taichi real-space
# convolution (universal-baseline, works on both CUDA and Vulkan via the
# standard ti.init(arch=ti.gpu) path). See docs/load-bearing-decisions.md
# "Runtime FFT-backend selection (priority order)".
cuda = ["cupy-cuda12x>=13,<14"]
cuda-torch = ["torch>=2.1,<3"]
rocm = ["torch>=2.1,<3"]   # ROCm wheel installed separately; see README

dev = [
    "ruff>=0.6,<1.0",
    "mypy>=1.11,<2",
    "pytest>=8,<9",
]

[tool.setuptools]
packages = ["lenia_fft"]

[tool.ruff]
line-length = 120
target-version = "py311"

[tool.ruff.lint]
select = ["E", "F", "W", "I", "UP", "B", "C4", "PIE", "RUF"]
ignore = [
    "E501",  # long kernel lines are OK
    "B008",  # function calls in argument defaults are idiomatic with Taichi types
    "E741",  # `for I in ti.grouped(...)` is canonical Taichi multi-dim index naming
]

[tool.mypy]
python_version = "3.11"
strict = true
warn_unused_ignores = true
[[tool.mypy.overrides]]
# Optional GPU-FFT deps have incomplete stubs or are absent in CI; allow Any.
module = ["taichi.*", "pyopenvdb.*", "cupy.*", "torch.*"]
ignore_missing_imports = true

[[tool.mypy.overrides]]
# @ti.kernel functions diverge from Python type semantics (banked Phase 9):
# - Taichi 1.7.4 rejects `-> None` return annotations at decoration time.
# - `ti.template()` arg annotation is call-form (Taichi runtime requirement);
#   mypy --strict valid-type wants subscript form.
# - Inside @ti.kernel scope, `int(vec)` returns ti.Vector[int] (with .cast()
#   method); mypy infers Python int (no .cast). Same gap, different mypy code.
# Cannot satisfy both Taichi runtime and mypy --strict simultaneously; relax
# the affected mypy rules on kernel + kernel-test modules. Non-kernel modules
# of the sim keep full mypy --strict.
module = ["lenia_fft.kernels", "tests.test_kernels"]
disallow_untyped_defs = false
disallow_untyped_decorators = false
disable_error_code = ["valid-type", "no-untyped-def", "attr-defined"]

[tool.pytest.ini_options]
testpaths = ["tests"]
python_files = ["test_*.py"]
```

#### `common/common-py/examples/hello/pyproject.toml` (verbatim)

```toml:common/common-py/examples/hello/pyproject.toml
[build-system]
requires = ["setuptools>=68", "wheel"]
build-backend = "setuptools.build_meta"

[project]
name = "gpusims-hello-py"
version = "0.1.0"
description = "common-py hello example: exercises every Phase 9 module"
requires-python = ">=3.11"
dependencies = [
    "gpusims-common-py",
    "taichi>=1.7.4,<1.8",
    "numpy>=1.26,<3",
]

[tool.setuptools]
packages = ["hello"]

[tool.ruff]
line-length = 120
target-version = "py311"

[tool.mypy]
python_version = "3.11"
strict = true
```

#### Existing Stack D test files on disk

```text
common/common-py/tests/test_state_reader.py
common/common-py/tests/test_kernels.py
continuous-ca/lenia-fft/python/tests/test_kernels.py
hybrid-particle-grid/mpm-multimaterial/python/tests/test_kernels.py
```

(Plus `__pycache__/` entries.)

#### `conftest.py` files in repo source

```text
(no matches outside ./common/common-py/.venv/... and ./references/...)
```

No `conftest.py` exists in any repo-source test directory.

**Stack D summary:** pytest 8 is in `dev` extras for every Stack D project. Each sim has `tests/test_kernels.py` on disk. CI runs `pytest tests/ -v` per package after install. The convention is already established — kernel-level smoke tests at `tests/test_kernels.py`, mypy --strict + ruff in the same pipeline.

### C.4 Cross-stack scripts directory

```text
ls -la scripts/ tools/ bin/ 2>/dev/null
(empty — none of these directories exist at the repo root)
```

There is no cross-stack-utility directory. If the toolkit's runner / parser lives in a single directory, it is greenfield (no existing convention to extend).

---

## Section D: Reference vendoring conventions

### `ls -la references/`

```text
drwxrwxr-x 16 otacon otacon 4096 May 14 11:10 SPlisHSPlasH
```

### `find references -maxdepth 2 -type d`

```text
references
references/SPlisHSPlasH
references/SPlisHSPlasH/Tests
references/SPlisHSPlasH/.git
references/SPlisHSPlasH/CMake
references/SPlisHSPlasH/Scripts
references/SPlisHSPlasH/pySPlisHSPlasH
references/SPlisHSPlasH/SPlisHSPlasH
references/SPlisHSPlasH/extern
references/SPlisHSPlasH/Tools
references/SPlisHSPlasH/data
references/SPlisHSPlasH/GUI
references/SPlisHSPlasH/data
references/SPlisHSPlasH/GUI
references/SPlisHSPlasH/doc
references/SPlisHSPlasH/Utilities
references/SPlisHSPlasH/.github
references/SPlisHSPlasH/Simulator
```

`references/SPlisHSPlasH/.git/` is present — this is a **clone** of the upstream, not an unpacked archive. The vendoring posture is "live git remote, gitignored, set up by `phase11_5_setup1`."

### `.gitignore` block for references

From `.gitignore` (relevant section verbatim):

```text:.gitignore
# Phase 11.5: SPlisHSPlasH vendored upstream reference (clone-on-setup, gitignored).
# Anchored to tag 2.16.1 (SHA 6bff55a6eaf14083d34650f22a268ce156b62b54).
# See docs/diagnostics/_audits/phase11_5_setup1_*.md for the setup record and the
# anchor-decision context (the original load-bearing-decisions.md anchor of
# "1.8.10" was non-existent upstream; 2.16.1 was selected as a fresh anchor).
/references/
```

The entire `/references/` tree is **gitignored**. No reference content is checked into the repo; the anchor is documented in-place (in `.gitignore`) plus the setup-1 audit. The convention is: "clone-on-setup, anchor recorded in `.gitignore` comment + per-sim doc + setup audit."

### Per-sim `upstream`/`vendor`/`third_party` directories

```text
find . -type d \( -name 'upstream' -o -name 'vendor' -o -name 'third_party' \) (excluding node_modules, build/, build-test-alembic, .venv)
(no matches at the repo-source level — all matches are inside .venv site-packages, build/_deps, build-test-alembic/_deps, or references/SPlisHSPlasH/extern)
```

No per-sim vendored-upstream tree exists. There is no `agent-based/physarum/upstream/`, `continuous-ca/lenia-fft/vendor/`, etc. **Only `references/SPlisHSPlasH/` exists today** as a vendored reference, and even that lives outside `.git` tracking.

---

## Section E: Existing citation / comment patterns

### Source-tree `file.ext:line` citation samples (selection)

Hits from `common/`, sim source dirs (excluding `.venv`, `build*`, `references/`):

```text
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
common/common-py/examples/hello/hello/main.py:30:# time (TaichiSyntaxError from kernel_impl.py:631 extract_arguments). The
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
continuous-ca/lenia-fft/python/lenia_fft/presets.py:11:v1.1 with the formula documented at LeniaNDK.py:329-335; see
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
continuous-ca/lenia-fft/python/lenia_fft/presets.py:79:# Decoded via 2D port of LeniaNDK.Board.rle2arr (LeniaNDK.py:184-206).
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
continuous-ca/lenia-fft/python/lenia_fft/main.py:204:    Inherited verbatim from MPM main.py:306-318. Documented in
<!-- integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a -->
particle-fluids/sph-water/shaders/jacobi_update_density.comp.glsl:7:// Reference: SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:591 (source term s_i =
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/density_alpha.comp.glsl:2:// as α/ρ² in the multiphase-compatible form per SPlisHSPlasH TimeStepDFSPH.cpp:758-760).
<!-- integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a -->
particle-fluids/sph-water/shaders/density_alpha.comp.glsl:5://   Cubic spline kernel: SPlisHSPlasH 1.8.10 SPHKernels.h:43-78
<!-- integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a -->
particle-fluids/sph-water/shaders/density_alpha.comp.glsl:6://   α-factor:            SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:813-822 / :1175-1188
<!-- integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a -->
particle-fluids/sph-water/shaders/density_alpha.comp.glsl:7://   α floor ε:           SPlisHSPlasH 1.8.10 TimeStepDFSPH.h:28 = 1.0e-5
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/compute_aij_pj.comp.glsl:12:// (TimeStepDFSPH.cpp:1370-1422, fluid-only branch).
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/compute_density_adv.comp.glsl:8:// Reference: SPlisHSPlasH 1.8.10 TimeStepDFSPH::computeDensityAdv (TimeStepDFSPH.cpp:1188-1242).
<!-- integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a -->
particle-fluids/sph-water/shaders/apply_velocity.comp.glsl:6:// density / divergence loops). Mirrors SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:514-515
<!-- integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a -->
particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:4://   Source s_i = -ρ̇_i:           SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:662
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:5://   aij_pj scales by h:           TimeStepDFSPH.cpp:656
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:6://   Pressure update (Jacobi 0.5): TimeStepDFSPH.cpp:692
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:7://   factor scales by 1/h:         TimeStepDFSPH.cpp:442
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/density_solve.comp.glsl:6://   Pass 2: aij_pj scales by h² (not h) per TimeStepDFSPH.cpp:582
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/density_solve.comp.glsl:7://   factor scales by 1/h² (not 1/h) per TimeStepDFSPH.cpp:285
<!-- integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a -->
particle-fluids/sph-water/shaders/density_solve.comp.glsl:10://   Source s_i = 1 - ρ_adv/ρ₀:   SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:590
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/density_solve.comp.glsl:11://   aij_pj *= h²:                 TimeStepDFSPH.cpp:582
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/density_solve.comp.glsl:12://   Pressure update:              TimeStepDFSPH.cpp:606
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/shaders/density_solve.comp.glsl:13://   factor scales 1/h²:           TimeStepDFSPH.cpp:285
<!-- integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a -->
particle-fluids/sph-water/shaders/pressure_apply.comp.glsl:7://   Velocity correction: SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:514-515 (divergence)
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/src/main.cpp:114:// DFSPH defaults — SPlisHSPlasH 1.8.10 at TimeStepDFSPH.cpp:35-41.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
particle-fluids/sph-water/src/main.cpp:122:constexpr float DFSPH_JACOBI_RELAX       = 0.5f;       // SPlisHSPlasH TimeStepDFSPH.cpp:606,:692
```

### Citation count by file extension (excluding `node_modules`, `build/`, `build-test-alembic/`, `references/`, `.venv`)

```text
   756 md
    20 glsl
     8 cpp
     4 py
```

// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
**Dominant format observed across stacks:** `Filename.ext:line` and `Filename.ext:line-line` (e.g., `TimeStepDFSPH.cpp:1370-1422`, `LeniaNDK.py:329-335`). The citation is bare — neither bracketed (`[file:line]`) nor URL-formatted — and appears inside `//` comments in C++/GLSL/TS and `#` comments in Python. In Markdown reports it appears wrapped in single backticks (e.g., ``` `main.cpp:1349` ```).

// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
// integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a
The same format is used for **upstream citations** (e.g., `SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:758-760`) and for **intra-repo cross-references** (e.g., `main.py:306-318`, `kernel_impl.py:631`). The toolkit's Cat 1 parser cannot distinguish the two from the literal alone — it must check whether the path resolves under a vendored reference root (`references/<lib>/`) or under the repo source tree.

### Markdown citation samples (selection from `docs/diagnostics/_audits/`)

```text
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
docs/diagnostics/_audits/phase11_5_commit1_landing_2026-05-14.md:22:the lambda body at `main.cpp:1349`. No other edits, no shader edits.
docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md:16:- `references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h:62-85`
docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md:17:- `particle-fluids/sph-water/shaders/density_alpha.comp.glsl:73-78`
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
docs/diagnostics/_audits/phase11_5_setup1_2026-05-14_setup1.md:183:| 1 | `density_solve.comp.glsl:8` | `TimeStepDFSPH.cpp` | 285 | `pressureSolve()` | factor scales 1/h² | `m_simulationData.getFactor(fluidModelIndex, i) *= invH2;` | **PLAUSIBLE_MATCH** |
```

In audit-style markdown the citation is `` `path:line` `` (backtick-wrapped), often inside table cells, and is structurally identical to the source-comment form.

---

## Section F: Existing upstream reference patterns

### SHA / tag mentions in `docs/`, sim docs, common docs

```text
docs/diagnostics/_audits/phase11_5_setup1_2026-05-14_setup1.md:14:The original anchor in `particle-fluids/sph-water/docs/load-bearing-decisions.md:8-9` was `SPlisHSPlasH 1.8.10 at SHA c254caf2705ebf5271408dd37a091aa379258a38`. Step 1 of the blocking probe (`phase11_5_setup1_2026-05-14_blocked.md`) established that **no `1.8` tag has ever existed upstream** [...]
docs/diagnostics/_audits/phase11_5_setup1_2026-05-14_setup1.md:51:# Anchored to tag 2.16.1 (SHA 6bff55a6eaf14083d34650f22a268ce156b62b54).
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md:514:| glfw | tag 3.4, GIT_SHALLOW | `deps.cmake:521-527` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md:515:| glm | tag 1.0.1, GIT_SHALLOW | `deps.cmake:535-540` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md:516:| spdlog | tag v1.14.1, GIT_SHALLOW | `deps.cmake:549-554` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md:517:| nlohmann_json | tag v3.11.3, GIT_SHALLOW | `deps.cmake:562-567` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md:518:| vma | tag v3.1.0, GIT_SHALLOW | `deps.cmake:572-577` |
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md:519:| shaderc | tag v2024.3, GIT_SHALLOW [...]
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md:520:| imgui | tag v1.91.5-docking, GIT_SHALLOW | `deps.cmake:606-611` |
```

Two parallel anchor patterns coexist in the repo:

1. **CMake FetchContent tags** for build-time C++ dependencies (`common/common-cpp/cmake/deps.cmake`) — `tag v1.14.1`, `tag 1.0.1`, etc. These are enforced by the build system; tag/SHA mismatches surface as link errors.
2. **Doc-anchored vendored upstreams** for ground-truth references — currently only `references/SPlisHSPlasH/` (tag `2.16.1`, SHA `6bff55a6eaf14083d34650f22a268ce156b62b54`), with the anchor recorded in `.gitignore` and in `docs/diagnostics/_audits/phase11_5_setup1_*.md`. **This anchor is enforced by nothing** — there is no CI check that the cloned `references/SPlisHSPlasH/.git` HEAD is the documented SHA.

### `upstream` / `pinned` / `anchor` mentions in `docs/`

// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
The `phase11_5_setup1` audits established the fabrication pattern: `particle-fluids/sph-water/docs/load-bearing-decisions.md` quoted "SPlisHSPlasH 1.8.10 at SHA c254caf2705ebf5271408dd37a091aa379258a38" — a fabricated tag (no `1.x` tag past `1.3.1` ever existed upstream) and a SHA copy-pasted from the file's Alembic line. The fix vendored `2.16.1` to `references/SPlisHSPlasH/`. The sim-local doc still says `1.8.10` as of probe time (per `setup1.md:408-410`).

### Per-sim load-bearing-decisions docs that cite upstream lines

```text
./closed-form/strange-attractors/docs/load-bearing-decisions.md
./continuous-ca/reaction-diffusion-2d/docs/load-bearing-decisions.md
./continuous-ca/reaction-diffusion-3d/docs/load-bearing-decisions.md
./continuous-ca/lenia-fft/docs/load-bearing-decisions.md
./particle-fluids/sph-water/docs/load-bearing-decisions.md
./closed-form/mandelbulb-explorer/docs/load-bearing-decisions.md
./agent-based/physarum/docs/load-bearing-decisions.md
./hybrid-particle-grid/mpm-multimaterial/docs/load-bearing-decisions.md
./volumetric-grid/eulerian-smoke/docs/load-bearing-decisions.md
```

Each shipped sim has a `docs/load-bearing-decisions.md`. The format varies. Header of the still-fabricated sph-water file:

```markdown:particle-fluids/sph-water/docs/load-bearing-decisions.md:1
# sph-water — Load-bearing decisions

This document is a sim-local quick reference. For the full reasoning, see
`docs/phase11_sph_water.md` § 2.

## DFSPH (not WCSPH, not PCISPH, not IISPH, not PBF)

Divergence-Free SPH per Bender-Koschier 2015 + 2017. Anchored to SPlisHSPlasH
1.8.10 at SHA `c254caf2705ebf5271408dd37a091aa379258a38` for every formula
citation. Five non-obvious upstream conventions encoded:
```

This is the same anchor whose fabrication motivated the toolkit. The "anchored to <project> <version> at SHA `<sha>`" sentence pattern at the top of a `docs/load-bearing-decisions.md` is the closest thing to a per-sim upstream-anchor *convention* — but it is prose, not structured, and (as setup-1 showed) not verified anywhere.

---

## Section G: Public API conventions

### Stack C public-header surface (`common/common-cpp/include/gpusims/`)

```text
common/common-cpp/include/gpusims/alembic_writer.hpp
common/common-cpp/include/gpusims/camera.hpp
common/common-cpp/include/gpusims/gpu_profiler.hpp
common/common-cpp/include/gpusims/hot_reload.hpp
common/common-cpp/include/gpusims/imgui_setup.hpp
common/common-cpp/include/gpusims/log.hpp
common/common-cpp/include/gpusims/state_reader.hpp
common/common-cpp/include/gpusims/state_writer.hpp
common/common-cpp/include/gpusims/vdb_writer.hpp
common/common-cpp/include/gpusims/vk/ (directory)
```

Per `common/common-cpp/CMakeLists.txt` header comment:

```text:common/common-cpp/CMakeLists.txt:18
# Includes are written as:
#     #include <gpusims/camera.hpp>
#     #include <gpusims/vk/context.hpp>
```

**Stack C "public" = headers under `common/common-cpp/include/gpusims/`.** Source files in `common/common-cpp/src/` are implementation. Per-sim public headers — if any — would live in the sim's own `include/` dir; the file list under `common/common-cpp/examples/hello/` (only `main.cpp` + shaders) suggests per-sim "public" surfaces are not a current pattern.

The recent Layer-2 audit `commoncpp_inventory_2026-05-14_architect2.md` is the authoritative inventory of the Stack C public-API surface at probe time.

### Stack B public-API surface (`common/common-web/src/`)

```text
common/common-web/src/types.ts
common/common-web/src/log.ts
common/common-web/src/paramPanel.ts
common/common-web/src/stateWriter.ts
common/common-web/src/index.ts
common/common-web/src/gpuProfiler.ts
common/common-web/src/camera.ts
common/common-web/src/viteWgslPlugin.ts
common/common-web/src/hotReload.ts
common/common-web/src/stateReader.ts
common/common-web/src/input.ts
common/common-web/src/webgpu/shaderModule.ts
common/common-web/src/webgpu/computePipeline.ts
common/common-web/src/webgpu/renderPipeline.ts
common/common-web/src/webgpu/renderer.ts
common/common-web/src/webgpu/texture.ts
common/common-web/src/webgpu/context.ts
common/common-web/src/webgpu/buffer.ts
```

Per `common/common-web/package.json` `exports`:

```json
"exports": {
  ".": { "types": "./src/index.ts", "import": "./src/index.ts" },
  "./vite-plugin": { "types": "./src/viteWgslPlugin.ts", "import": "./src/viteWgslPlugin.ts" }
}
```

**Stack B "public" = what `src/index.ts` re-exports.** The `exports` map deliberately excludes the rest. A toolkit Cat-2 contract-verification check for Stack B would need to read `src/index.ts` and compare against per-sim consumer imports.

### Stack D public-API surface (`common/common-py/gpusims_common/`)

```text
common/common-py/gpusims_common/__init__.py
common/common-py/gpusims_common/alembic_writer.py
common/common-py/gpusims_common/camera.py
common/common-py/gpusims_common/log.py
common/common-py/gpusims_common/param_panel.py
common/common-py/gpusims_common/state_reader.py
common/common-py/gpusims_common/state_writer.py
common/common-py/gpusims_common/vdb_writer.py
```

Per `.github/workflows/build-py.yml:50`:

```text
python -c "from gpusims_common import Camera, CameraMode, ParamPanel, StateWriter, StateReader, VdbWriter, AlembicWriter, ParticleFrame, log; print('ok')"
```

The smoke-imported names from `gpusims_common` are the *de facto* public surface today (smoke-checked but not formally contract-asserted). A Cat-2 check would compare what `__init__.py` actually re-exports against the documented surface.

---

## Section H: Existing convention markers

### Marker counts in repo source dirs (Stack C/D/web src + shaders), excluding `.venv`

```text
9 NOTE
2 WARN
```

(`TODO`, `FIXME`, `XXX`, `HACK`, `SAFETY` returned **no hits** in the constrained repo-source set — the inflated counts seen in a broader sweep are all from `common/common-py/.venv/lib/python3.12/site-packages/...` and from upstream `build/_deps/`. The repo source is unusually clean of TODO-style markers.)

Sample of the `NOTE` markers in source:

```text
common/common-py/tests/test_kernels.py:15:# NOTE: deliberately NO `from __future__ import annotations`. Same Taichi
common/common-py/examples/hello/hello/main.py:28:# NOTE: deliberately NO `from __future__ import annotations`. Taichi 1.7.4's
```

### `integrity-*` marker presence

```text
grep -rEohn 'integrity-[a-z]+' . (excluding node_modules, references/, build/, build-test-alembic/, .venv) → (no hits)
```

// integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a
No `integrity-` prefixed annotation is in use anywhere in the repo source today. A toolkit suppression annotation in the `integrity-allow:`/`integrity-skip:`/`integrity-todo:` family would not collide with any existing marker.

### Comment-style by language (observed)

// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
- C++ / GLSL / WGSL / TypeScript: `// NOTE: …` on its own line at the head of a block, or appended to a constant declaration (e.g., `main.cpp:114` quoted earlier).
- Python: `# NOTE: …` on its own line.
- The `NOTE:` form (uppercase + colon) is the dominant style.

---

## Section I: Build / language versions

### `.nvmrc` / `.python-version`

```text
cat .nvmrc            → (file does not exist, no output)
cat .python-version   → (file does not exist, no output)
```

Neither pinning file exists. Node and Python versions are pinned **only in CI**:

- `package.json:7-9`:
  ```json
  "engines": { "node": ">=22" }
  ```
- `.github/workflows/build-web.yml:28-29`: `node-version: '22'`
- `.github/workflows/deploy-pages.yml:40-41`: `node-version: '22'`
- `.github/workflows/build-py.yml:30-31`: `python-version: '3.11'`
- Stack D `pyproject.toml` files: `requires-python = ">=3.11"` and `target-version = "py311"`.

### `CMakeLists.txt` (top-level, repo root)

```cmake:CMakeLists.txt:1
cmake_minimum_required(VERSION 3.25)

project(gpu_sims
    VERSION     0.1.0
    DESCRIPTION "GPU-accelerated physics and emergence simulations"
    LANGUAGES   C CXX
)

# ----------------------------------------------------------------------------
# Project-wide configuration
# ----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_C_STANDARD            11)
set(CMAKE_C_STANDARD_REQUIRED   ON)

# Generate compile_commands.json for clangd / IDE indexers.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

### Vulkan API version (`common/common-cpp/CMakeLists.txt`)

```text:common/common-cpp/CMakeLists.txt:117
GPU_SIMS_VULKAN_API_VERSION=VK_API_VERSION_1_3
```

**Stack C pin: CMake ≥ 3.25, C++20, C11, Vulkan 1.3.**

### WebGPU type version

Every Stack B `package.json` (`common-web` and all six sim packages) carries:

```text
"@webgpu/types": "^0.1.50"
```

```text
common/common-web/src/webgpu/context.ts:60:        if (!('gpu' in navigator) || !navigator.gpu) {
common/common-web/src/webgpu/context.ts:67:        const adapter = await navigator.gpu.requestAdapter({
common/common-web/src/webgpu/context.ts:93:        const preferredFormat = navigator.gpu.getPreferredCanvasFormat();
```

**Stack B pin: Node 22, TypeScript ^5.6.0, Vite ^7.0.0, `@webgpu/types ^0.1.50`.**

### Taichi version pin (every Stack D `pyproject.toml`)

```text
"taichi>=1.7.4,<1.8"
```

with `requires-python = ">=3.11"` and `target-version = "py311"`. NumPy is pinned `>=1.26,<3`.

**Stack D pin: Python ≥ 3.11 (CI uses 3.11), Taichi 1.7.4 (sub-1.8 cap), numpy ≥ 1.26 < 3, pytest ^8, ruff ^0.6 < 1.0, mypy ^1.11 < 2.**

---

## Section J: Documentation conventions

### Docs directory layout

```text
docs
docs/diagnostics
docs/diagnostics/_audits
docs/retro
docs/sim-specs
```

### Top-level `docs/*.md` and second-level (selection)

```text
docs/conventions.md
docs/hardware.md
docs/overarching-spec.md
docs/retro/phase11.md
docs/root-context-distilled.md
docs/sim-specs/boids-3d.md
docs/sim-specs/eulerian-smoke.md
docs/sim-specs/ising-dwave.md
docs/sim-specs/lattice-boltzmann.md
docs/sim-specs/lenia-fft.md
docs/sim-specs/mandelbulb-explorer.md
docs/sim-specs/mpm-multimaterial.md
docs/sim-specs/neural-ca.md
docs/sim-specs/physarum.md
docs/sim-specs/pic-flip.md
docs/sim-specs/reaction-diffusion-2d.md
docs/sim-specs/reaction-diffusion-3d.md
docs/sim-specs/sph-water.md
docs/sim-specs/strange-attractors.md
docs/sim-specs/_template.md
docs/stack-decisions.md
docs/tier1-capture-format-reference.md
```

`docs/diagnostics/` currently contains a single subdirectory `_audits/` (this report's home). No diagnostics-meta-doc (overview, README, or runbook) exists at `docs/diagnostics/` today — only the per-audit reports under `_audits/`.

### `docs/overarching-spec.md` — opening (verbatim, lines 1-30)

```markdown:docs/overarching-spec.md:1
# gpu-sims — Overarching Specification

> **Status:** Living document. Authoritative source for cross-cutting decisions across all simulation projects in this repo.
> **Audience:** The architect/coordinator chat, individual per-sim implementer chats, and any future contributor.
> **Companion document:** `root-context-distilled.md` — captures the reasoning and rejected alternatives behind the decisions in this spec.

---

## 1. Project goal and design philosophy

`gpu-sims` is a portfolio of GPU-accelerated physics and emergence simulations, each pushed toward the maximum scale and visual quality the hardware can sustain. The unifying philosophy:

**Scientific amazement through physical correctness at maximum scale.** Every simulation should be a real implementation of its underlying mathematics — not a stylized approximation, not a fakery. The "wow" comes from seeing 4M particles obey Navier-Stokes correctly in real time, not from post-processing tricks. When approximations are made, they are documented and justified.

**Interactive prototype, cinematic export.** Every sim runs in real time on consumer hardware at moderate scale, with full interactivity (sliders, mouse interactions, hot-reload). Every sim also exports its state to industry-standard formats (OpenVDB, Alembic) so it can be re-rendered offline at maximum scale on HPC hardware with path-traced lighting. The interactive build is for iteration and demonstration; the offline build is for hero renders and showcase material.
```

`docs/overarching-spec.md` totals 342 lines. The toolkit's spec is plausibly an entry under `docs/` (sibling to `conventions.md`) or under `docs/diagnostics/` as a runbook companion to the audits. There is no `docs/tooling/` or `docs/checks/` directory at present.

### Existing audit front-matter style (precedents)

Three precedents for diagnostics-report front-matter live in `docs/diagnostics/_audits/`:

1. Architect-1 probe (`phase11_5_probe_2026-05-14_architect1.md:1-8`):
   ```yaml
   ---
   title: Phase 11.5 DFSPH Architecture Probe
   date: 2026-05-14
   author: architect1
   phase: 11.5
   status: probe
   scope: read-only
   ---
   ```
2. Setup-1 (`phase11_5_setup1_2026-05-14_setup1.md:1-8`): adds `phase`, `status: complete`, fuller `scope` description.
3. Architect-2 layer-2 audit (`commoncpp_inventory_2026-05-14_architect2.md:1-17`): adds `layer`, `sibling-layers`, `out-of-scope`, `cross_workstream` keys.

This report follows the architect-1 form and adds out-of-scope / audience keys for clarity.

### Markdown lint / link rules

Markdown is gated by two CI jobs (`.github/workflows/markdown.yml`):

- `DavidAnson/markdownlint-cli2-action@v16` — lint rules in `.markdownlint.json` (258 bytes, present at repo root).
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- `lycheeverse/lychee-action@v1` — `--offline --include-fragments` link check, config in `lychee.toml` (317 bytes, present at repo root). Per `markdown.yml:46`, `docs/sim-specs/_template.md` is excluded from link checks.

Any toolkit-emitted markdown report must lint clean under both.

---

## Section P: Incidental findings (relevant but not asked)

- **Two CMake build trees at repo root.** `build/` and `build-test-alembic/` both exist. Either may shadow the other if a toolkit script naively globs `build/**`. The "official" CI build directory is `build/` per `.github/workflows/build-native.yml:57`.
- **`common/common-py/.venv/` exists in the working tree.** This pollutes any naive recursive grep of Python source (NumPy / mypy site-packages contribute several thousand spurious citation-like matches). The toolkit must whitelist `common/common-py/.venv/`, `node_modules/`, `build*/_deps/`, `references/`, and `**/__pycache__/`.
- **`references/SPlisHSPlasH/.git/` is a live clone.** Any HEAD-vs-anchor check the toolkit performs must read `references/SPlisHSPlasH/.git/HEAD` (or `git -C references/SPlisHSPlasH rev-parse HEAD`) and compare against the documented SHA in `.gitignore:1-5`. There is no current verification step.
- **Per-sim load-bearing-decisions.md headers are prose, not structured.** The "Anchored to <upstream> <tag> at SHA `<sha>`" sentence at the top of `particle-fluids/sph-water/docs/load-bearing-decisions.md:7-9` is the only existing structured-ish anchor — and it is stale (still says `1.8.10`). The toolkit choosing a structured anchor format (a frontmatter block, a sidecar YAML, etc.) is a greenfield decision; whatever it picks should be importable into the existing prose paragraph.
- **CI path filters partition stacks.** `build-native.yml`, `build-py.yml`, `build-web.yml` each gate on different paths. A cross-stack integrity toolkit either (a) needs its own workflow with `paths: ['**']` (no filter), or (b) needs to be wired into all three plus `structure.yml`/`markdown.yml`. The `structure.yml` workflow already runs on every push without path filtering (`.github/workflows/structure.yml:3-6`) — that is the closest existing template for an always-on cross-stack gate.
- **Stack C has no test framework, but Stack D has `pytest tests/ -v` per package.** If the toolkit's Cat-3 (numerical correctness) tests live alongside source-of-truth tests, the Stack D side has a natural integration point and the Stack C side does not. Either Stack C grows a tests/ directory + GoogleTest/Catch2 wiring, or Cat-3 for Stack C runs as a standalone driver (host program + golden-blob check) outside the existing build.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- **lenia-fft already cites Chakazul/LeniaNDK without vendoring it.** `continuous-ca/lenia-fft/python/lenia_fft/presets.py:11` and `:79` cite `LeniaNDK.py:329-335` / `LeniaNDK.py:184-206` — same structural pattern as the fabricated sph-water anchor, except (per `sims_prioritization_2026-05-14_triage.md:78`) `LeniaNDK` is **not vendored**. Verifying these citations would require either vendoring `references/Chakazul-Lenia/` or accepting that the citation is permanently unverifiable from the working tree. This is the next test case for Cat-1 integrity once the toolkit lands.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- **`docs/sim-specs/_template.md` is the canonical per-sim spec template.** It is excluded from lychee link-checking (per `markdown.yml:46`) precisely because its `{{PLACEHOLDER}}` link targets are not yet substituted. The toolkit must treat placeholder-tokens (`{{...}}`) as non-citations.
- **Documentation gate today is structure-only.** `structure.yml` enforces "the right files and directories exist" but **not** "what's in them is internally consistent." Citation integrity, contract verification, and numerical correctness are all currently outside any CI gate.
- **`docs/conventions.md` already documents per-stack testing expectations** (`docs/conventions.md:15`: "Testing: Each sim includes at minimum a smoke test ..."), but the language is aspirational — there is no Stack C test in the repo today, and the Stack B smoke is the `tsc --noEmit && vite build` compile-check (not a runtime test). The toolkit's Cat-3 spec section may want to update `docs/conventions.md` § Testing to match what it formalizes.
- **Front-matter token reservation.** Existing audit front-matter uses `title`, `date`, `author`, `phase`, `status`, `scope`, `audience`, `out-of-scope`, `sibling-layers`, `cross_workstream`, `layer`. If the toolkit reads frontmatter, it should avoid colliding with these — pick `toolkit-*` or `integrity-*`-prefixed keys for any new tags.

---

## Report file

Absolute path: `/home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits/integrity_toolkit_probe_2026-05-14_architect1.md`
