"""Cube-volume preset definitions for mpm-multimaterial.

Each preset is a list of CubeVolume entries assigned proportionally to total
particle count by `main.init_volumes`. Single source of truth.
"""

from __future__ import annotations

from dataclasses import dataclass

from kernels import JELLY, SNOW, WATER


@dataclass(frozen=True)
class CubeVolume:
    """A cube-shaped volume of particles in the unit-cube sim domain."""

    minimum: tuple[float, float, float]
    size:    tuple[float, float, float]
    material: int

    @property
    def volume(self) -> float:
        return self.size[0] * self.size[1] * self.size[2]


def build_presets() -> list[tuple[str, list[CubeVolume]]]:
    """Return the canonical preset list as (name, cube-list) pairs."""
    return [
        (
            "Single Dam Break",
            [
                CubeVolume(minimum=(0.55, 0.05, 0.55), size=(0.40, 0.40, 0.40), material=WATER),
            ],
        ),
        (
            "Double Dam Break",
            [
                CubeVolume(minimum=(0.05, 0.05, 0.05), size=(0.30, 0.40, 0.30), material=WATER),
                CubeVolume(minimum=(0.65, 0.05, 0.65), size=(0.30, 0.40, 0.30), material=WATER),
            ],
        ),
        (
            "Water Snow Jelly",
            [
                CubeVolume(minimum=(0.60, 0.05, 0.60), size=(0.25, 0.25, 0.25), material=WATER),
                CubeVolume(minimum=(0.35, 0.35, 0.35), size=(0.25, 0.25, 0.25), material=SNOW),
                CubeVolume(minimum=(0.05, 0.60, 0.05), size=(0.25, 0.25, 0.25), material=JELLY),
            ],
        ),
        (
            "Mixed Sandbox",
            [
                # Water pond at the bottom
                CubeVolume(minimum=(0.10, 0.05, 0.10), size=(0.80, 0.10, 0.80), material=WATER),
                # Jelly column on one side
                CubeVolume(minimum=(0.15, 0.20, 0.15), size=(0.15, 0.45, 0.15), material=JELLY),
                # Snow cube floating above (will fall and splash)
                CubeVolume(minimum=(0.40, 0.55, 0.40), size=(0.20, 0.20, 0.20), material=SNOW),
                # Small jelly ball on the other side
                CubeVolume(minimum=(0.65, 0.30, 0.65), size=(0.15, 0.15, 0.15), material=JELLY),
            ],
        ),
    ]
