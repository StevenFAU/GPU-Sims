"""VdbWriter — real-or-stub VDB grid writer for Stack D.

Real-mode requires the `pyopenvdb` Python package (ships via Ubuntu apt
`python3-openvdb` or conda-forge `openvdb`; NOT pip-installable). When the import
fails, all functions log a single one-time warning and return False — same
posture as Stack C's `GPU_SIMS_USE_OPENVDB` compile-time flag.

Surface mirrors common-cpp `gpusims::vdb` (vdb_writer.hpp):

    write_float_grid(path, data, dims, voxel_size, origin, grid_name)
    write_float_frame(base, frame_idx, data, dims, voxel_size, origin, grid_name)

Data convention (must match Stack C):
  - Grid array is shape (X, Y, Z) — x-fastest, y, then z. numpy row-major.
  - voxel_size: world units per cell.
  - origin: world-space position of voxel (0, 0, 0) corner.

The class `VdbWriter` is exposed for parity with `AlembicWriter` (object-shape
streaming writer) but isn't strictly needed — the per-frame `write_float_frame`
free function is the canonical sim-side call site.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

from gpusims_common.log import log

# Try-import pyopenvdb. If absent, this module operates in stub-only mode.
try:
    import pyopenvdb as _vdb
    _HAS_VDB: bool = True
except ImportError:
    _vdb = None
    _HAS_VDB = False

_warned_unavailable: bool = False


def _warn_unavailable_once() -> None:
    """Log a one-time warning that pyopenvdb is unavailable."""
    global _warned_unavailable
    if _warned_unavailable:
        return
    log.warn(
        "pyopenvdb not available; VDB writer running in STUB mode. "
        "Install via `sudo apt install python3-openvdb` (Ubuntu) "
        "or `conda install -c conda-forge openvdb` to enable real VDB export."
    )
    _warned_unavailable = True


def is_available() -> bool:
    """True if this process has pyopenvdb importable (real mode); False if stub-only."""
    return _HAS_VDB


# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
def write_float_grid(
    path: str | Path,
    data: np.ndarray,
    dims: tuple[int, int, int],
    voxel_size: float,
    origin: tuple[float, float, float] = (0.0, 0.0, 0.0),
    grid_name: str = "density",
) -> bool:
    """Write a single dense float grid to a .vdb file.

    Returns True on success, False if pyopenvdb is unavailable or write fails.
    """
    if not _HAS_VDB:
        _warn_unavailable_once()
        return False
    arr = _coerce_to_float32_dense_3d(data, dims)
    if arr is None:
        return False
    grid = _vdb.FloatGrid()
    grid.copyFromArray(arr)
    grid.name = grid_name
    grid.gridClass = _vdb.GridClass.FOG_VOLUME
    grid.transform = _vdb.createLinearTransform(voxelSize=voxel_size)
    # Origin offset, applied as a post-transform translation.
    if origin != (0.0, 0.0, 0.0):
        # Pre-multiply translation into the transform
        grid.transform.translate(origin)
    try:
        _vdb.write(str(path), grids=[grid])
        return True
    except Exception as exc:
        log.error("vdb.write(%s) failed: %s", path, exc)
        return False


# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
def write_float_frame(
    base: str | Path,
    frame_idx: int,
    data: np.ndarray,
    dims: tuple[int, int, int],
    voxel_size: float,
    origin: tuple[float, float, float] = (0.0, 0.0, 0.0),
    grid_name: str = "density",
) -> bool:
    """Write a sequence frame. Output path is `<base>_<NNNN>.vdb` (4-digit pad).

    Convenience for per-frame export inside a sim loop.
    """
    base_path = Path(base)
    out = base_path.with_name(f"{base_path.name}_{frame_idx:04d}.vdb")
    return write_float_grid(out, data, dims, voxel_size, origin, grid_name)


class VdbWriter:
    """Object-shape wrapper around `write_float_frame`.

    Provides a `write_frame(data, frame_idx)` method for sites that prefer the
    object idiom over free functions. Exposed for API parity with
    `AlembicWriter`'s `ParticleWriter`-shape construction.
    """

    def __init__(
        self,
        base: str | Path,
        dims: tuple[int, int, int],
        voxel_size: float,
        origin: tuple[float, float, float] = (0.0, 0.0, 0.0),
        grid_name: str = "density",
    ) -> None:
        self._base: Path = Path(base)
        self._dims: tuple[int, int, int] = dims
        self._voxel_size: float = voxel_size
        self._origin: tuple[float, float, float] = origin
        self._grid_name: str = grid_name
        if not _HAS_VDB:
            _warn_unavailable_once()

    def write_frame(self, frame_idx: int, data: np.ndarray) -> bool:
        return write_float_frame(
            self._base,
            frame_idx,
            data,
            self._dims,
            self._voxel_size,
            self._origin,
            self._grid_name,
        )

    @staticmethod
    def is_available() -> bool:
        return _HAS_VDB


# ----------------------------------------------------------------------
# Internals
# ----------------------------------------------------------------------

def _coerce_to_float32_dense_3d(
    data: np.ndarray,
    dims: tuple[int, int, int],
) -> np.ndarray | None:
    """Return a float32 (X, Y, Z) ndarray suitable for `grid.copyFromArray`.

    Accepts either a 3D array of matching shape, or a 1D flat array of length
    X*Y*Z which is reshaped to (X, Y, Z). Logs and returns None on shape mismatch.
    """
    expected_count = dims[0] * dims[1] * dims[2]
    if data.ndim == 3:
        if data.shape != dims:
            log.error("write_float_grid: data.shape %s != dims %s", data.shape, dims)
            return None
        arr = data
    elif data.ndim == 1:
        if data.size != expected_count:
            log.error(
                "write_float_grid: data.size %d != X*Y*Z (%d)",
                data.size, expected_count,
            )
            return None
        arr = data.reshape(dims)
    else:
        log.error("write_float_grid: data must be 1D or 3D, got ndim=%d", data.ndim)
        return None
    if arr.dtype != np.float32:
        arr = arr.astype(np.float32, copy=False)
    # OpenVDB's copyFromArray expects contiguous memory.
    if not arr.flags["C_CONTIGUOUS"]:
        arr = np.ascontiguousarray(arr)
    return arr
