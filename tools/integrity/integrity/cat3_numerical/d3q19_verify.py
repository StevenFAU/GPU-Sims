"""Independent first-principles verifier for the D3Q19 algebraic ground truth.

Re-derives the 19 velocity vectors and 3 weight values from scratch, evaluates
the BGK equilibrium at the four canonical test points from d3q19.md, performs
the algebraic sanity checks from sections 3.3 and 4.2 of the derivation, and
emits the per-direction feq table to d3q19_equilibrium.expected.json.

Run-time output (stdout) is a short verification trace; on success the harness
exits 0 and the JSON blob is written. Any assertion failure stops execution
with a descriptive message and a nonzero exit.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from fractions import Fraction


HERE = Path(__file__).resolve().parent
EXPECTED_JSON = HERE / "d3q19_equilibrium.expected.json"

TOL_ABS = 1e-12


def load_expected_payload() -> dict:
    """Load the expected-values JSON payload.

    Returns the dict structure described in the docstring of main() --
    velocity_set, weights, opposite_index, test_points.
    """
    if not EXPECTED_JSON.is_file():
        raise FileNotFoundError(
            f"expected values JSON missing: {EXPECTED_JSON}; "
            f"run `python3 {Path(__file__).name}` to regenerate."
        )
    return json.loads(EXPECTED_JSON.read_text(encoding="utf-8"))


def verify_velocity_set(payload: dict) -> list[str]:
    """Verify the JSON payload's velocity_set matches the re-derivation."""
    expected = [tuple(c) for c in build_velocity_set()]
    got_raw = payload.get("velocity_set")
    if got_raw is None:
        return ["velocity_set: payload key missing"]
    got = [tuple(c) for c in got_raw]
    if got != expected:
        return [
            f"velocity_set mismatch: got {got}, want {expected}",
        ]
    return []


def verify_weights(payload: dict) -> list[str]:
    """Verify weights match the re-derivation: 1/3, 1/18 x6, 1/36 x12."""
    velocity_set = [tuple(c) for c in build_velocity_set()]
    expected_fracs = [weight_for(c) for c in velocity_set]
    expected = [float(w) for w in expected_fracs]
    got = payload.get("weights")
    if got is None:
        return ["weights: payload key missing"]
    if len(got) != len(expected):
        return [f"weights length mismatch: got {len(got)}, want {len(expected)}"]
    errs: list[str] = []
    for i, (g, e) in enumerate(zip(got, expected)):
        if abs(g - e) > TOL_ABS:
            errs.append(f"weights[{i}]: got {g!r}, want {e!r}, |diff|={abs(g-e):.3e}")
    # Sanity: sum should equal 1.0 exactly under float (rational sum gives 1).
    s = sum(got)
    if abs(s - 1.0) > TOL_ABS:
        errs.append(f"sum(weights) = {s!r}, want 1.0 (|diff|={abs(s-1.0):.3e})")
    return errs


def verify_equilibrium(payload: dict) -> list[str]:
    """Verify per-test-point feq tables match a fresh evaluation of feq()."""
    velocity_set = [tuple(c) for c in build_velocity_set()]
    weights = [float(weight_for(c)) for c in velocity_set]
    test_points = payload.get("test_points")
    if not test_points:
        return ["test_points: payload key missing or empty"]
    errs: list[str] = []
    for tp in test_points:
        name = tp.get("name", "<unnamed>")
        rho = tp["rho"]
        u = tp["u"]
        got_feq = tp.get("feq")
        if got_feq is None:
            errs.append(f"{name}: feq missing")
            continue
        recomputed = feq(rho, u[0], u[1], u[2], velocity_set, weights)
        if len(recomputed) != len(got_feq):
            errs.append(f"{name}: feq length mismatch: got {len(got_feq)}, want {len(recomputed)}")
            continue
        for i, (g, r) in enumerate(zip(got_feq, recomputed)):
            if abs(g - r) > TOL_ABS:
                errs.append(
                    f"{name}.feq[{i}]: got {g!r}, recomputed {r!r}, "
                    f"|diff|={abs(g-r):.3e} > {TOL_ABS:.0e}"
                )
    return errs


def build_velocity_set() -> list[tuple[int, int, int]]:
    """Enumerate {-1,0,1}^3 with squared L2 norm <= 2, ordered canonically."""
    candidates = [
        (cx, cy, cz)
        for cx in (-1, 0, 1)
        for cy in (-1, 0, 1)
        for cz in (-1, 0, 1)
        if cx * cx + cy * cy + cz * cz <= 2
    ]
    rest = [c for c in candidates if c == (0, 0, 0)]

    faces_by_axis_sign = []
    for axis in (0, 1, 2):
        for sign in (+1, -1):
            for c in candidates:
                if (
                    c[axis] == sign
                    and all(c[other] == 0 for other in (0, 1, 2) if other != axis)
                ):
                    faces_by_axis_sign.append(c)

    def edge_group(zero_axis: int) -> list[tuple[int, int, int]]:
        nonzero_axes = tuple(a for a in (0, 1, 2) if a != zero_axis)
        ordered_signs = [(+1, +1), (+1, -1), (-1, +1), (-1, -1)]
        out = []
        for s_a, s_b in ordered_signs:
            target = [0, 0, 0]
            target[nonzero_axes[0]] = s_a
            target[nonzero_axes[1]] = s_b
            out.append(tuple(target))
        return out

    xy_edges = edge_group(zero_axis=2)
    xz_edges = edge_group(zero_axis=1)
    yz_edges = edge_group(zero_axis=0)

    ordered = rest + faces_by_axis_sign + xy_edges + xz_edges + yz_edges
    assert len(ordered) == 19, f"expected 19 vectors, got {len(ordered)}"
    return ordered


EXPECTED_CANONICAL = [
    (0, 0, 0),
    (+1, 0, 0), (-1, 0, 0),
    (0, +1, 0), (0, -1, 0),
    (0, 0, +1), (0, 0, -1),
    (+1, +1, 0), (+1, -1, 0), (-1, +1, 0), (-1, -1, 0),
    (+1, 0, +1), (+1, 0, -1), (-1, 0, +1), (-1, 0, -1),
    (0, +1, +1), (0, +1, -1), (0, -1, +1), (0, -1, -1),
]

EXPECTED_OPPOSITE_PAIRS = [
    (0, 0),
    (1, 2), (3, 4), (5, 6),
    (7, 10), (8, 9),
    (11, 14), (12, 13),
    (15, 18), (16, 17),
]


def weight_for(c: tuple[int, int, int]) -> Fraction:
    n2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2]
    if n2 == 0:
        return Fraction(1, 3)
    if n2 == 1:
        return Fraction(1, 18)
    if n2 == 2:
        return Fraction(1, 36)
    raise ValueError(f"unexpected squared norm {n2} for {c}")


def feq(rho: float, ux: float, uy: float, uz: float,
        cs: list[tuple[int, int, int]], ws: list[float]) -> list[float]:
    u_dot_u = ux * ux + uy * uy + uz * uz
    out = []
    for c, w in zip(cs, ws):
        cu = c[0] * ux + c[1] * uy + c[2] * uz
        bracket = 1.0 + 3.0 * cu + 4.5 * cu * cu - 1.5 * u_dot_u
        out.append(w * rho * bracket)
    return out


# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
def opposite_index(i: int, cs: list[tuple[int, int, int]]) -> int:
    target = (-cs[i][0], -cs[i][1], -cs[i][2])
    return cs.index(target)


# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
def assert_close(label: str, got: float, want: float, tol: float = TOL_ABS) -> None:
    if abs(got - want) > tol:
        raise AssertionError(
            f"{label}: got {got!r}, want {want!r}, |diff|={abs(got-want):.3e} > {tol:.0e}"
        )


def main() -> int:
    print("=== D3Q19 algebraic verification harness ===")
    cs = build_velocity_set()
    print(f"  built {len(cs)} velocity vectors from {{-1,0,1}}^3 with |c|^2 <= 2")

    assert cs == EXPECTED_CANONICAL, (
        "canonical ordering mismatch\n"
        f"  got:  {cs}\n  want: {EXPECTED_CANONICAL}"
    )
    print("  canonical ordering matches d3q19.md section 2.2 table: OK")

    weights_frac = [weight_for(c) for c in cs]
    weights = [float(w) for w in weights_frac]
    norm = sum(weights_frac)
    assert norm == Fraction(1, 1), f"sum(omega_i) = {norm}, want 1"
    print(f"  sum(omega_i) = {norm} (exact rational): OK")

    for alpha in range(3):
        for beta in range(3):
            s = sum(
                w * Fraction(c[alpha] * c[beta])
                for w, c in zip(weights_frac, cs)
            )
            want = Fraction(1, 3) if alpha == beta else Fraction(0)
            assert s == want, (
                f"second moment ({alpha},{beta}): got {s}, want {want}"
            )
    print("  second-moment tensor exactly (1/3) delta_{alpha,beta}: OK")

    opp = [opposite_index(i, cs) for i in range(19)]
    derived_pairs = []
    seen: set[int] = set()
    for i in range(19):
        if i in seen:
            continue
        j = opp[i]
        if i == j:
            derived_pairs.append((i, i))
        else:
            lo, hi = (i, j) if i < j else (j, i)
            derived_pairs.append((lo, hi))
        seen.add(i)
        seen.add(j)
    derived_pairs.sort()
    expected_sorted = sorted(EXPECTED_OPPOSITE_PAIRS)
    assert derived_pairs == expected_sorted, (
        f"opposite-pair set mismatch\n  got:  {derived_pairs}\n  want: {expected_sorted}"
    )
    print(f"  opposite-direction involution matches: {derived_pairs}")

    test_points = [
        {"name": "tp1_zero_velocity", "rho": 1.0, "u": (0.0, 0.0, 0.0)},
        {"name": "tp2_uniform_x", "rho": 1.0, "u": (0.1, 0.0, 0.0)},
        {"name": "tp3_oblique_xy", "rho": 1.0, "u": (0.05, 0.05, 0.0)},
        {"name": "tp4_density_scaled", "rho": 2.5, "u": (0.05, 0.0, 0.0)},
    ]

    results = []
    for tp in test_points:
        rho = tp["rho"]
        ux, uy, uz = tp["u"]
        f = feq(rho, ux, uy, uz, cs, weights)

        sum_f = sum(f)
        sum_fx = sum(fi * c[0] for fi, c in zip(f, cs))
        sum_fy = sum(fi * c[1] for fi, c in zip(f, cs))
        sum_fz = sum(fi * c[2] for fi, c in zip(f, cs))

        assert_close(f"{tp['name']} mass", sum_f, rho)
        assert_close(f"{tp['name']} mom_x", sum_fx, rho * ux)
        assert_close(f"{tp['name']} mom_y", sum_fy, rho * uy)
        assert_close(f"{tp['name']} mom_z", sum_fz, rho * uz)

        results.append({
            "name": tp["name"],
            "rho": rho,
            "u": [ux, uy, uz],
            "feq": f,
            "sums": {
                "mass": sum_f,
                "mom_x": sum_fx,
                "mom_y": sum_fy,
                "mom_z": sum_fz,
            },
        })

    print()
    print("--- Hand-check: test point 2 (rho=1.0, u=(0.1, 0, 0)) ---")
    tp2 = next(r for r in results if r["name"] == "tp2_uniform_x")
    expected_tp2 = {
        0:  Fraction(1, 3)  * Fraction(985, 1000),
        1:  Fraction(1, 18) * Fraction(1330, 1000),
        2:  Fraction(1, 18) * Fraction(730, 1000),
        3:  Fraction(1, 18) * Fraction(985, 1000),
        4:  Fraction(1, 18) * Fraction(985, 1000),
        5:  Fraction(1, 18) * Fraction(985, 1000),
        6:  Fraction(1, 18) * Fraction(985, 1000),
        7:  Fraction(1, 36) * Fraction(1330, 1000),
        8:  Fraction(1, 36) * Fraction(1330, 1000),
        9:  Fraction(1, 36) * Fraction(730, 1000),
        10: Fraction(1, 36) * Fraction(730, 1000),
        11: Fraction(1, 36) * Fraction(1330, 1000),
        12: Fraction(1, 36) * Fraction(1330, 1000),
        13: Fraction(1, 36) * Fraction(730, 1000),
        14: Fraction(1, 36) * Fraction(730, 1000),
        15: Fraction(1, 36) * Fraction(985, 1000),
        16: Fraction(1, 36) * Fraction(985, 1000),
        17: Fraction(1, 36) * Fraction(985, 1000),
        18: Fraction(1, 36) * Fraction(985, 1000),
    }
    for i, fi in enumerate(tp2["feq"]):
        want = float(expected_tp2[i])
        assert_close(f"tp2 feq[{i}]", fi, want)
        print(f"  feq[{i:>2}] c={cs[i]!s:<14} = {fi:.15f}  (want {want:.15f})")
    print(f"  sum(feq)       = {tp2['sums']['mass']:.15f}  (want 1.0)")
    print(f"  sum(feq*c_x)   = {tp2['sums']['mom_x']:.15f}  (want 0.1)")
    print(f"  sum(feq*c_y)   = {tp2['sums']['mom_y']:.15f}  (want 0.0)")
    print(f"  sum(feq*c_z)   = {tp2['sums']['mom_z']:.15f}  (want 0.0)")

    print()
    for r in results:
        if r["name"] == "tp2_uniform_x":
            continue
        s = r["sums"]
        print(f"  {r['name']}: rho={r['rho']} u={tuple(r['u'])}"
              f" mass={s['mass']:.15f} mom=({s['mom_x']:.15f},"
              f" {s['mom_y']:.15f}, {s['mom_z']:.15f})")

    tp4 = next(r for r in results if r["name"] == "tp4_density_scaled")
    tp4_unit = feq(1.0, 0.05, 0.0, 0.0, cs, weights)
    for i, (a, b) in enumerate(zip(tp4["feq"], tp4_unit)):
        assert_close(f"tp4 linear scaling i={i}", a, 2.5 * b)
    print("  tp4 density linear-scaling check (feq(2.5,u) == 2.5*feq(1.0,u)): OK")

    payload = {
        "schema_version": 1,
        "source": "tools/integrity/integrity/cat3_numerical/d3q19_verify.py",
        "derivation": "tools/integrity/docs/algebraic/d3q19.md",
        "velocity_set": [list(c) for c in cs],
        "weights": weights,
        "opposite_index": opp,
        "test_points": [
            {
                "name": r["name"],
                "rho": r["rho"],
                "u": r["u"],
                "feq": r["feq"],
                "sums": r["sums"],
            }
            for r in results
        ],
    }
    EXPECTED_JSON.write_text(json.dumps(payload, indent=2) + "\n")
    print(f"\n  wrote {EXPECTED_JSON.relative_to(Path.cwd()) if EXPECTED_JSON.is_relative_to(Path.cwd()) else EXPECTED_JSON}")

    print("\n=== ALL CHECKS PASS ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
