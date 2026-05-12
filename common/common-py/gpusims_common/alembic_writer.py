"""AlembicWriter — Stack D permanent stub.

This module is a **permanent stub** for Phase 9. The real Python Alembic
binding lands at the natural sph-water consumer phase (Decision #8 / rule-of-three
convention in `project-state.md` § 7).

Surface mirrors common-cpp `gpusims::abc::ParticleWriter` (alembic_writer.hpp)
so the future real impl drops in cleanly:

    ParticleFrame                  -- per-frame point cloud snapshot
    AlembicWriter.create(path, fps) -> AlembicWriter | None
    AlembicWriter.write_frame(frame) -> bool
    is_available() -> bool

The discriminator for "do we have real VFX Alembic (pyalembic) vs. SQLAlchemy
migration tool (PyPI `alembic`)" is `from alembic import AbcGeom`. SQLAlchemy
Alembic does NOT expose `AbcGeom`; VFX pyalembic does. Phase 9 ships with the
discriminator returning False unconditionally — the real check is deferred to
whichever phase decides to land pyalembic. When that happens, set _HAS_ALEMBIC
to the discriminator result and fill in `_RealParticleWriter` with the actual
ABC writer.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import numpy as np

from gpusims_common.log import log


@dataclass
class ParticleFrame:
    """Particle frame for streaming particle exports.

    Mirrors common-cpp `gpusims::abc::ParticleFrame`. `positions` is required;
    `velocities`, `radii`, `ids` are optional. `count` is the number of valid
    particles (positions array should have at least `count * 3` floats).
    """

    positions: np.ndarray                        # float32, shape (count, 3) or flat (count*3,)
    count: int                                   # number of particles
    velocities: np.ndarray | None = None         # float32, shape (count, 3); optional
    radii: np.ndarray | None = None              # float32, shape (count,); optional
    ids: np.ndarray | None = None                # uint64, shape (count,); optional


# Phase 9 ships with VFX Alembic unconditionally unavailable.
# When real Python Alembic lands, replace with a runtime discriminator:
#
#   try:
#       import alembic
#       from alembic import AbcGeom   # SQLAlchemy alembic does NOT expose this
#       _HAS_ALEMBIC = True
#   except ImportError:
#       _HAS_ALEMBIC = False
_HAS_ALEMBIC: bool = False

_warned_unavailable: bool = False


def _warn_unavailable_once() -> None:
    """Log a one-time warning that Alembic is in permanent-stub mode for Phase 9."""
    global _warned_unavailable
    if _warned_unavailable:
        return
    log.warn(
        "Alembic Python binding not available; AlembicWriter running in STUB mode. "
        "VFX-Alembic Python bindings are not pip-installable (PyPI `alembic` is "
        "SQLAlchemy migrations — different library). Real Python Alembic support "
        "is banked for the natural sph-water Stack C consumer (Phase 11+). "
        "Use PLY export (`ti.tools.PLYWriter`) for the Phase 9 hero-render path."
    )
    _warned_unavailable = True


def is_available() -> bool:
    """True if this process has a real VFX-Alembic binding (always False in Phase 9)."""
    return _HAS_ALEMBIC


class AlembicWriter:
    """Streaming particle writer. One instance per .abc file.

    Phase 9 is permanent-stub: `create(...)` always returns None and logs a
    one-time warning. The shape is preserved so consumer code (e.g.,
    `mpm-multimaterial/main.py`) can be written against the real-or-stub API
    today without conditional branching at every call site.

    Idiomatic consumer pattern (works in both stub and real modes):

        writer = AlembicWriter.create("particles.abc", fps=24.0)
        if writer is not None:
            writer.write_frame(ParticleFrame(positions=x_np, count=n))
        else:
            # Real-mode unavailable — fall back to PLY or skip Alembic export.
            ...
    """

    def __init__(self, path: Path, fps: float) -> None:
        # Phase 9: this constructor is never actually called from create() because
        # create() returns None. Kept for symmetry with the future real impl.
        self._path: Path = path
        self._fps: float = fps

    @classmethod
    def create(cls, path: str | Path, fps: float = 24.0) -> AlembicWriter | None:
        """Construct a streaming Alembic particle writer, or return None if unavailable.

        Phase 9: always returns None (permanent stub). Consumer code MUST handle
        the None return cleanly.
        """
        if not _HAS_ALEMBIC:
            _warn_unavailable_once()
            return None
        # When real impl lands, replace with: return _RealParticleWriter(Path(path), fps)
        return None  # pragma: no cover

    def write_frame(self, frame: ParticleFrame) -> bool:
        """Write one particle frame to the .abc file. Stub: returns False."""
        if not _HAS_ALEMBIC:
            _warn_unavailable_once()
            return False
        return False  # pragma: no cover
