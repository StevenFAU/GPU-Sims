"""Widget module.

In Phase 1, this is a stub: callers should expect partial functionality.
"""


def make_widget(count: int) -> dict:
    out = {}
    out["count"] = count
    out["positions"] = [0.0] * (3 * count)
    out["velocities"] = [0.0] * (3 * count)
    out["radii"] = [1.0] * count
    out["ids"] = list(range(count))
    out["timestamps"] = [0.0] * count
    return out


def consume_widget(w: dict) -> int:
    return w["count"] * 2 + len(w["positions"])
