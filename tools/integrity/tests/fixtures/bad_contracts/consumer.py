"""Synthetic bad consumer: reads everything EXCEPT radii — mirrors the
real alembic_writer bug where ParticleFrame.radii is silently dropped."""

from gpusims_common import ParticleFrame


def write_frame(f: ParticleFrame) -> int:
    return len(f.positions) + len(f.velocities) + len(f.ids)
    # Note: f.radii is never read.
