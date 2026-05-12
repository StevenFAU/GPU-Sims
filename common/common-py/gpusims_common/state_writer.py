"""StateWriter — loose-directory state capture for Stack D.

Mirrors the cross-stack capture schema documented in
`docs/tier1-capture-format-reference.md` byte-for-byte. **This is a hard
cross-stack contract**: Tier-1 / Tier-3 diagnostics tooling reads the format
this writer produces and expects the schema as specified. Per-buffer meta
fields are `{name, file, count, stride, format, shape}` — not `{bytes, dtype}`.

Top-level `meta` uses **exactly one** sim-namespaced key per the convention in
§ 1 of the capture-format reference. The key is the activation signature for
the Tier-3 module that diagnoses this sim. Convention: camelCase, named after
the sim. The MPM consumer calls `set_meta("mpmMultimaterial", {...})`; the
hello-world example calls `set_meta("helloPy", {...})`.

Layout:

    <root>/capture_<NNNN>/
        state.json     -- metadata; describes each blob
        <name>.bin     -- one binary file per save_buffer() call

Schema (state.json) — matches Stack B / Stack C verbatim:

    {
        "frame": 42,
        "meta": {
            "mpmMultimaterial": {
                "camera": {...},
                "gravity": [0, -9.8, 0],
                "preset": "Water Snow Jelly",
                "tier_idx": 0,
                "n_particles": 250000,
                "n_grid": 96,
                ...
            }
        },
        "buffers": [
            { "name": "x",         "file": "x.bin",
              "count": 750000, "stride": 4, "format": "r32f",
              "shape": [250000, 3] },
            { "name": "materials", "file": "materials.bin",
              "count": 250000, "stride": 4, "format": "r32i",
              "shape": [250000] },
            ...
        ]
    }

Per-buffer field semantics (per tier1-capture-format-reference.md § 2-3):
  - `count`: total scalar elements (e.g. for a (N, 3) float32 array, count = N*3).
  - `stride`: bytes per scalar element (e.g. 4 for float32, 4 for int32, 2 for fp16).
  - `format`: WebGPU-style format string for the scalar element. Phase 9
    introduces `r32i` for int32 scalar buffers — the format-string set in the
    existing reference (§ 3) listed `r32f`, `rgba16f`, `rgba16float` as the four
    observed before Phase 9; `r32i` is a natural extension matching the
    WebGPU/Vulkan naming convention. Banked at the spec's load-bearing decisions.
  - `shape`: logical array shape (e.g. `[N, 3]` for N vec3 particles; `[G, G, G]`
    for a 3D grid). Absent if not applicable (packed-struct buffers — see
    boids-3d entities precedent in the reference).

Usage:

    writer = StateWriter("captures")
    writer.begin_frame(frame_idx)
    writer.set_meta("mpmMultimaterial", {
        "camera": camera.to_json(),
        "gravity": gravity,
        ...
    })
    writer.save_buffer("x", x_np, shape=[n_particles, 3])
    writer.save_buffer("materials", mat_np, shape=[n_particles])
    writer.end_frame()
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import numpy as np

from gpusims_common.log import log

# Mapping numpy dtype → WebGPU-style format string used by the capture schema.
# Only entries where dtype → format is unambiguous (1-channel scalar element).
# Multi-channel buffers (e.g. float16 vec4 for velocity grids) MUST pass
# `format=` explicitly at the save_buffer call site — there's no way to infer
# channel count from numpy dtype alone, and silently picking a 4-channel format
# for a scalar float16 array would emit a misleading format string that breaks
# Tier-1 / Tier-3 diagnostics. This mirrors Stack C eulerian-smoke's explicit
# call-site convention for velocity buffers (`format: "rgba16f"`).
# Banked extension: Phase 9 adds `r32i` for int32 scalar buffers — natural
# extension matching the WebGPU/Vulkan naming convention of the existing
# `r32f` / `rgba16f` / `rgba16float` set in `tier1-capture-format-reference.md` § 3.
_DTYPE_TO_FORMAT: dict[str, str] = {
    "float32": "r32f",
    "int32":   "r32i",      # Phase 9 addition; tracked in tier1 ref's format table
    "uint32":  "r32ui",     # banked for future use; no Phase 9 consumer
}


class StateWriter:
    """Save full simulation state to disk in the cross-stack loose-directory format.

    Designed for Python-friendly reload (matches Stack C / Stack B schema 1:1):

        import json, numpy as np
        meta = json.load(open("captures/capture_0001/state.json"))
        sim_meta = meta["meta"]["mpmMultimaterial"]   # sim-namespaced wrapper
        x_buf = next(b for b in meta["buffers"] if b["name"] == "x")
        x = np.fromfile("captures/capture_0001/x.bin", dtype=np.float32)
        x = x.reshape(x_buf["shape"])
    """

    def __init__(self, root_dir: str | Path) -> None:
        self._root: Path = Path(root_dir)
        self._root.mkdir(parents=True, exist_ok=True)
        self._current_dir: Path | None = None
        self._state: dict[str, Any] = {}
        self._buffers: list[dict[str, Any]] = []
        self._in_frame: bool = False

    def begin_frame(self, frame_idx: int) -> None:
        """Open a new capture directory at <root>/capture_<NNNN>/. Pads to 4 digits."""
        if self._in_frame:
            log.warn("begin_frame called while already in a frame; ending the previous one first.")
            self.end_frame()
        name = f"capture_{frame_idx:04d}"
        self._current_dir = self._root / name
        self._current_dir.mkdir(parents=True, exist_ok=True)
        self._state = {"frame": int(frame_idx), "meta": {}, "buffers": []}
        self._buffers = []
        self._in_frame = True

    def set_meta(self, key: str, value: Any) -> None:
        """Set a sim-namespaced JSON-serializable metadata blob.

        Per `tier1-capture-format-reference.md` § 1, every shipped sim writes
        **exactly one** sim-namespaced top-level key (e.g., `mpmMultimaterial`,
        `eulerianSmoke`, `physarum`). Consumer code should call this ONCE per
        frame with the sim's full runtime metadata wrapped under one key.
        Multiple calls with different keys are allowed but will produce a
        capture that no Tier-3 module recognizes as activating its diagnostics.
        """
        if not self._in_frame:
            log.error("set_meta called outside a frame; ignoring.")
            return
        if len(self._state["meta"]) >= 1 and key not in self._state["meta"]:
            log.warn(
                "set_meta(%r): this is the second top-level meta key in this frame; "
                "convention is exactly one sim-namespaced key per the tier1-capture-format "
                "reference. Consider wrapping multiple values under one sim key.",
                key,
            )
        self._state["meta"][key] = value

    def save_buffer(
        self,
        name: str,
        data: np.ndarray,
        count: int | None = None,
        stride: int | None = None,
        format: str | None = None,  # noqa: A002 (shadows builtin; matches schema field name)
        shape: list[int] | tuple[int, ...] | None = None,
    ) -> None:
        """Save a numpy array as a binary blob.

        Per-buffer schema fields written to state.json:
          - count: total scalar elements (inferred as data.size if not provided)
          - stride: bytes per scalar (inferred as data.itemsize if not provided)
          - format: WebGPU-style format string (inferred from dtype via _DTYPE_TO_FORMAT)
          - shape: logical array layout (inferred from data.shape if not provided)

        The on-disk bytes are written via `data.tofile(...)` — raw little-endian
        of the array's dtype, contiguous x-fastest. Hard-rule-11 from § 0 applies:
        no np.float64 anywhere; verify `data.dtype` matches a supported entry
        in _DTYPE_TO_FORMAT before calling (the writer warns and falls back to
        an empty format string if unrecognized).
        """
        if not self._in_frame or self._current_dir is None:
            log.error("save_buffer called outside a frame; ignoring.")
            return
        if not isinstance(data, np.ndarray):
            raise TypeError(f"save_buffer expects np.ndarray, got {type(data).__name__}")

        # Infer count: total scalar elements in the array.
        if count is None:
            count = int(data.size)

        # Infer stride: bytes per scalar element.
        if stride is None:
            stride = int(data.itemsize)

        # Infer format from dtype.
        if format is None:
            dtype_str = str(data.dtype)
            if dtype_str in _DTYPE_TO_FORMAT:
                format = _DTYPE_TO_FORMAT[dtype_str]
            else:
                log.warn(
                    "save_buffer(%s): dtype %s not in _DTYPE_TO_FORMAT; "
                    "writing empty format string (Tier-1 will treat as raw bytes).",
                    name, dtype_str,
                )
                format = ""

        # Infer shape from data.shape if not provided.
        if shape is None:
            if data.ndim >= 1:
                shape = list(data.shape)
            else:
                shape = [1]

        filename = f"{name}.bin"
        path = self._current_dir / filename
        data.tofile(path)

        entry: dict[str, Any] = {
            "name": name,
            "file": filename,
            "count": int(count),
            "stride": int(stride),
        }
        if format:
            entry["format"] = format
        if shape is not None:
            entry["shape"] = list(shape)

        self._buffers.append(entry)

    def end_frame(self) -> None:
        """Write state.json and close the frame."""
        if not self._in_frame or self._current_dir is None:
            log.warn("end_frame called with no active frame; ignoring.")
            return
        self._state["buffers"] = self._buffers
        path = self._current_dir / "state.json"
        with open(path, "w") as f:
            json.dump(self._state, f, indent=2)
        self._in_frame = False
        self._current_dir = None
        self._state = {}
        self._buffers = []

    def current_dir(self) -> Path | None:
        return self._current_dir
