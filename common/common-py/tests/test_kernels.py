"""Taichi CPU-backend kernel-compile smoke test for common-py.

Banked: Taichi's @ti.kernel AST inspection requires functions to live in real
files on disk; `python -c '...'` string code fails with "Cannot find source
code for Object." This test file MUST stay on disk to exercise the Taichi
compile path under CI.

Run with:
    pytest tests/test_kernels.py -v

Or directly:
    python tests/test_kernels.py
"""

from __future__ import annotations

import numpy as np
import taichi as ti


def test_taichi_imports() -> None:
    """Taichi imports + version sanity check."""
    assert ti.__version__ >= (1, 7, 0), f"Taichi {ti.__version__} too old; need >=1.7"


def test_cpu_kernel_compile_and_run() -> None:
    """Compile + run a small kernel exercising the Stack D-relevant API surface.

    Covers: Vector.field, Matrix.field, ti.svd, ti.grouped, ti.static, basic
    arithmetic on field elements. If any drifts in Taichi 1.7.x's API, this
    catches it at CI time.
    """
    ti.init(arch=ti.cpu, default_fp=ti.f32)

    n = 64
    dim = 3
    x = ti.Vector.field(dim, ti.f32, n)
    F = ti.Matrix.field(dim, dim, ti.f32, n)
    out_sig = ti.field(ti.f32, n)

    @ti.kernel
    def init():
        for i in range(n):
            x[i] = ti.Vector([ti.random(), ti.random(), ti.random()])
            F[i] = ti.Matrix.identity(ti.f32, dim)

    @ti.kernel
    def svd_smoke():
        for I in ti.grouped(x):
            # Exercise the SVD path used by MPM plasticity.
            # U / V intentionally unused — this smoke test only verifies sig.
            _U, sig, _V = ti.svd(F[I], ti.f32)
            # Capture trace(sig) into a scalar field.
            s = 0.0
            for d in ti.static(range(dim)):
                s += sig[d, d]
            out_sig[I] = s

    init()
    svd_smoke()

    # For identity matrices, sig = identity, so trace = dim.
    sig_np = out_sig.to_numpy()
    assert sig_np.shape == (n,)
    assert np.allclose(sig_np, float(dim), atol=1e-5), (
        f"Expected SVD(I).trace == {dim} for all particles, got mean={sig_np.mean()}"
    )


def test_field_to_numpy_roundtrip() -> None:
    """Verify field.to_numpy() / .from_numpy() round-trip used by StateWriter."""
    n = 100
    x = ti.Vector.field(3, ti.f32, n)

    @ti.kernel
    def fill():
        for i in range(n):
            x[i] = ti.Vector([float(i), float(i) * 2.0, float(i) * 3.0])

    fill()
    arr = x.to_numpy()
    assert arr.shape == (n, 3)
    assert arr.dtype == np.float32

    # Round-trip
    arr2 = arr * 2.0
    x.from_numpy(arr2.astype(np.float32))
    arr3 = x.to_numpy()
    assert np.allclose(arr3, arr2, atol=1e-6)


def test_ply_writer_surface() -> None:
    """Verify ti.tools.PLYWriter signature used for MPM particle export."""
    import os
    import tempfile

    n = 50
    pos = np.random.rand(n, 3).astype(np.float32)
    material = np.random.randint(0, 3, size=n, dtype=np.int32)

    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "smoke.ply")
        writer = ti.tools.PLYWriter(num_vertices=n)
        writer.add_vertex_pos(pos[:, 0], pos[:, 1], pos[:, 2])
        writer.add_vertex_channel("material", "int", material)
        writer.export(path)
        assert os.path.getsize(path) > 0, "PLY file empty"


if __name__ == "__main__":
    test_taichi_imports()
    test_cpu_kernel_compile_and_run()
    test_field_to_numpy_roundtrip()
    test_ply_writer_surface()
    print("ok — all kernel smoke tests pass")
