"""Canonical paths per stack per spec § 7.2."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class StackPaths:
    name: str
    public_surface_dir: Path
    implementation_dir: Path


def stack_paths(root: Path) -> dict[str, StackPaths]:
    """Return the per-stack public/impl path map rooted at `root`."""
    return {
        "c": StackPaths(
            name="c",
            public_surface_dir=root / "common" / "common-cpp" / "include" / "gpusims",
            implementation_dir=root / "common" / "common-cpp" / "src",
        ),
        "b": StackPaths(
            name="b",
            public_surface_dir=root / "common" / "common-web" / "src",
            implementation_dir=root / "common" / "common-web" / "src",
        ),
        "d": StackPaths(
            name="d",
            public_surface_dir=root / "common" / "common-py" / "gpusims_common",
            implementation_dir=root / "common" / "common-py" / "gpusims_common",
        ),
    }
