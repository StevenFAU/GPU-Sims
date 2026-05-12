"""StateReader — loose-directory state capture loader for Stack D.

Reads the cross-stack capture schema documented in
`docs/tier1-capture-format-reference.md`. Surfaces:
- find_latest(): for F9-load convenience
- find_by_frame(idx): explicit frame index
- load_meta(capture_dir): returns state.json contents
- load_buffer(capture_dir, name): returns flat numpy array
- load_buffer_reshaped(capture_dir, name): returns ndarray reshaped per buffer's `shape` field

The schema's per-buffer fields are `{name, file, count, stride, format, shape}`
(matches Stack B / Stack C 1:1). This reader maps `format` back to a numpy
dtype via the inverse of `state_writer._DTYPE_TO_FORMAT`.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

import numpy as np

from gpusims_common.log import log

_CAPTURE_NAME_RE = re.compile(r"^capture_(\d+)$")

# Inverse of state_writer._DTYPE_TO_FORMAT. Stack B's `rgba16float` is folded
# to the same dtype as Stack C's `rgba16f` per the tier1 reference's
# format-normalization table.
_FORMAT_TO_DTYPE: dict[str, np.dtype] = {
    "r32f":         np.dtype(np.float32),
    "r32i":         np.dtype(np.int32),
    "r32ui":        np.dtype(np.uint32),
    "rgba16f":      np.dtype(np.float16),
    "rgba16float":  np.dtype(np.float16),
}


class StateReader:
    def __init__(self, root_dir: str | Path) -> None:
        self._root: Path = Path(root_dir)

    def find_latest(self) -> Path | None:
        if not self._root.is_dir():
            return None
        best_idx = -1
        best_path: Path | None = None
        for child in self._root.iterdir():
            if not child.is_dir():
                continue
            m = _CAPTURE_NAME_RE.match(child.name)
            if not m:
                continue
            idx = int(m.group(1))
            if idx > best_idx:
                best_idx = idx
                best_path = child
        return best_path

    def find_by_frame(self, frame_idx: int) -> Path | None:
        candidate = self._root / f"capture_{frame_idx:04d}"
        return candidate if candidate.is_dir() else None

    def load_meta(self, capture_dir: Path) -> dict[str, Any]:
        path = capture_dir / "state.json"
        with open(path) as f:
            return json.load(f)  # type: ignore[no-any-return]

    def load_buffer(
        self,
        capture_dir: Path,
        name: str,
        dtype: np.dtype | str | None = None,
    ) -> np.ndarray:
        """Load a binary blob as a flat 1D ndarray.

        If `dtype` is None, derives from the buffer's `format` field via
        `_FORMAT_TO_DTYPE`. Returns a flat 1D array; caller is responsible for
        reshaping (or use `load_buffer_reshaped`).
        """
        meta = self.load_meta(capture_dir)
        entry = next((b for b in meta.get("buffers", []) if b.get("name") == name), None)
        if entry is None:
            raise KeyError(f"buffer '{name}' not in {capture_dir}/state.json")

        effective_dtype: np.dtype | str
        if dtype is not None:
            effective_dtype = dtype
        else:
            fmt = entry.get("format", "")
            if fmt in _FORMAT_TO_DTYPE:
                effective_dtype = _FORMAT_TO_DTYPE[fmt]
            else:
                log.warn(
                    "load_buffer(%s): format %r not in _FORMAT_TO_DTYPE; "
                    "defaulting to float32. Pass `dtype=` explicitly if wrong.",
                    name, fmt,
                )
                effective_dtype = np.dtype(np.float32)

        path = capture_dir / entry["file"]
        arr = np.fromfile(path, dtype=effective_dtype)
        expected_count = int(entry.get("count", arr.size))
        if arr.size != expected_count:
            log.warn(
                "load_buffer(%s): on-disk count %d mismatches state.json count %d",
                name, arr.size, expected_count,
            )
        return arr

    def load_buffer_reshaped(
        self,
        capture_dir: Path,
        name: str,
        dtype: np.dtype | str | None = None,
    ) -> np.ndarray:
        """Load a buffer and reshape via its `shape` field. If absent, returns flat."""
        arr = self.load_buffer(capture_dir, name, dtype)
        meta = self.load_meta(capture_dir)
        entry = next((b for b in meta.get("buffers", []) if b.get("name") == name), None)
        if entry is None:
            return arr
        shape = entry.get("shape")
        if shape is None:
            return arr
        try:
            return arr.reshape(shape)
        except ValueError as exc:
            log.warn("load_buffer_reshaped(%s): reshape to %s failed: %s", name, shape, exc)
            return arr
