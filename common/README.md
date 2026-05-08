# common/

Shared infrastructure consumed by every native, web, and Python sim in this repo.

**Status:** Skeleton only. API surfaces and code are populated by Phase 1.

## Planned API surfaces

Phase 1 will add the following modules. Their precise APIs are specified in [`../docs/overarching-spec.md`](../docs/overarching-spec.md) §5 and finalized in the Phase 1 build document.

- **Camera** — free-fly, arcball, and orbit modes. Used by every native and web sim.
- **Hot-reload** — file-watcher-based shader hot-reload for GLSL, WGSL, and HLSL. Graceful fallback on shader compile errors keeps the sim running with the last good shader.
- **Profiling** — per-pass GPU timing wrappers, printable to an ImGui overlay and dumpable to CSV for offline analysis.
- **State capture** — `F5` save / `F9` load of full simulation state to disk. Simple format (JSON metadata + binary blobs) that's restorable across runs and inspectable from Python.
- **Export (VDB / Alembic)** — the cinematic-export path. Volumetric grid sims write OpenVDB; particle and mesh sims write Alembic. Designed to be called from any sim's record mode.

## Likely sub-structure (to be confirmed by Phase 1)

```
common/
├── common-cpp/        # Headers and source for native (C++) sims
│   ├── camera/
│   ├── hot-reload/
│   ├── profiling/
│   ├── state-capture/
│   ├── ui/
│   ├── export/
│   │   ├── vdb/
│   │   └── alembic/
│   └── cmake/
├── common-web/        # TypeScript package for web (WebGPU) sims
└── common-py/         # Python package for Stack D sims
```

The three-package split allows each stack's idiomatic dependencies (CMake / npm / pip) without cross-contamination. Phase 1 confirms or revises this.

## Consumption

- **Native sims** consume `common-cpp/` as a CMake module (`add_subdirectory(common/common-cpp)`) or as an installed package.
- **Web sims** import from `common-web/` as a workspace package.
- **Python sims** install `common-py/` from a local path in `requirements.txt` or `pyproject.toml`.

Per-sim chats consume `common/`'s API; they do not modify it without coordination through the architect chat. See [`../CONTRIBUTING.md`](../CONTRIBUTING.md) for the workflow.
