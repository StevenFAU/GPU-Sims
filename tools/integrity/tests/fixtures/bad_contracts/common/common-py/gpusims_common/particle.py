"""ParticleFrame: deliberately mirrors the real ParticleFrame defect class."""

from dataclasses import dataclass, field


@dataclass
class ParticleFrame:
    positions: list[float] = field(default_factory=list)
    velocities: list[float] = field(default_factory=list)
    radii: list[float] = field(default_factory=list)
    ids: list[int] = field(default_factory=list)
