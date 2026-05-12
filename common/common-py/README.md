# common-py — Stack D shared infrastructure (Python / Taichi)

Stack D's shared Python package for GPU-Sims. Mirrors the conceptual surface of
[`common-cpp`](../common-cpp/) and [`common-web`](../common-web/) where applicable;
designed slim and demand-driven (per `project-state.md` § 7's rule-of-three convention).

**Status:** Phase 9 — implemented. First consumer: `hybrid-particle-grid/mpm-multimaterial/`.

## Layout

```
common-py/
├── pyproject.toml             package metadata, ruff/mypy/pytest config
├── README.md                  this file
├── gpusims_common/            public API (importable as `import gpusims_common`)
│   ├── __init__.py            barrel exports
│   ├── log.py                 stdlib logging wrapper ("gpusims" logger)
│   ├── camera.py              Camera (wraps ti.ui.Camera; modes, view/proj, JSON)
│   ├── state_writer.py        StateWriter (loose-dir JSON + .bin, Decision #16)
│   ├── state_reader.py        StateReader
│   ├── param_panel.py         ParamPanel (Taichi GGUI sub_window wrapper)
│   ├── vdb_writer.py          VDB writer; real-or-stub gated on `import pyopenvdb`
│   └── alembic_writer.py      Alembic permanent-stub; real impl banked to sph-water phase
├── examples/hello/            reference application; copy to start a new sim
│   ├── pyproject.toml
│   ├── main.py                exercises every module
│   └── README.md
└── tests/
    └── test_kernels.py        Taichi-CPU-backend kernel-compile smoke for CI
```

## Per-sim consumption

A new Stack D sim's `pyproject.toml`:

```toml
[project]
name = "gpusims-<sim>-py"
version = "0.1.0"
requires-python = ">=3.11"
dependencies = [
    "gpusims-common-py @ file://../../../common/common-py",  # path-relative editable install
    "taichi>=1.7.4,<1.8",
    "numpy>=1.26,<3",
]
```

Per-sim Python:

```python
import taichi as ti
from gpusims_common import Camera, StateWriter, StateReader, ParamPanel, VdbWriter, log

ti.init(arch=ti.gpu)  # picks CUDA when available, else Vulkan
# ... build the sim ...
```

## Build dependencies

Required (Ubuntu 24.04):

```bash
# Python 3.11+ from the default repos
sudo apt install python3 python3-venv python3-pip

# Vulkan runtime (for AMD desktop + Taichi Vulkan backend)
sudo apt install libvulkan1 vulkan-tools mesa-vulkan-drivers
```

Optional (enable as needed):

```bash
# OpenVDB Python bindings — Ubuntu apt
sudo apt install python3-openvdb

# CUDA + cuDNN — for the lab PC (NVIDIA) path; per the upstream NVIDIA Ubuntu repo
# https://developer.nvidia.com/cuda-downloads
```

Setup:

```bash
cd common/common-py
python3 -m venv .venv
source .venv/bin/activate
pip install -e .[dev]
```

## Hello-world

The example at `examples/hello/` exercises every Phase 9 module end-to-end (Taichi GGUI
window + free-fly camera + state save/load + param panel + VDB stub-or-real export +
Alembic permanent-stub call). Run with:

```bash
cd common/common-py/examples/hello
pip install -e .
python main.py
```

Controls: WASDQE to move, hold right mouse to look. F5 saves a state capture
(creates `captures/capture_NNNN/state.json` + `*.bin`); F9 loads the most recent.
Pressing the "export vdb" button writes `vdb_export/density_NNNN.vdb` (or logs a stub
warning if `pyopenvdb` is not installed). The Alembic export button always logs a
stub-mode warning — Alembic Python support is banked for the natural sph-water
consumer phase (see `project-state.md` § 7 rule-of-three banking).

## Hot-reload

Stack D has no in-process kernel hot-reload — Taichi's `@ti.kernel` decoration captures
the function's Python AST at definition time, so editing kernel source requires a fresh
Python process. The dev workflow is **Ctrl+C, edit, re-run**. This is documented in
`project-state.md` § 7 "Stack D has no HotReloader" as expected behavior, not a missing
feature; a future Stack D sim demanding hot-reload would design a `watchfiles`-based
process-restart wrapper at that point.

## Backend selection

`ti.init(arch=ti.gpu)` picks CUDA when available, else Vulkan, else CPU. Force a specific
backend via `ti.init(arch=ti.cuda)` / `ti.init(arch=ti.vulkan)` / `ti.init(arch=ti.cpu)`.
The AMD RX 6800 XT dev desktop runs Vulkan; the RTX 2080 Ti lab PC runs CUDA;
both are first-class targets for GPU-Sims Stack D sims.
