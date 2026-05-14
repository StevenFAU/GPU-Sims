# Phase 11.5 Commit-1 Landing — 2026-05-14

Smallest-commit landing of the host-side gradient-kernel-norm fix. Single
file, single coefficient. No shader changes. No commit; diff staged for
user review.

Verifies against commit-2 verification probe Claim 1:
`docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md`.

---

## A. Change summary

The host-side `grad_kernel_norm_3d_value` lambda in
`particle-fluids/sph-water/src/main.cpp` was multiplying by `48.0f / (π · h⁴)`,
which produces a kernel-gradient normalization 6× larger than upstream
SPlisHSPlasH's `CubicKernel::gradW` (probe Claim 1, numerically confirmed
at two test points). The GPU shader bakes the `1/h` factor into the host
constant, so upstream's `m_l = 48 / (π · h³)` becomes the host-side
`8 / (π · h⁴)` (i.e. `48 / 6` to absorb the gradient polynomial's `1/6`
scaling that the shader applies on-GPU). Changed `48.0f` → `8.0f` inside
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
the lambda body at `main.cpp:1349`. No other edits, no shader edits.

---

## B. Verification

### B.1 — view of modified function

```
1347      auto grad_kernel_norm_3d_value = [&]() {
1348          float h = rt.supportRadius;
1349          return 8.0f / (float(M_PI) * h * h * h * h);
1350      };
```

### B.2 — `grep -n "48.0f / (float(M_PI)" particle-fluids/sph-water/src/main.cpp`

```
(no output — zero hits)
```

### B.3 — `grep -n "8.0f / (float(M_PI) \* h \* h \* h \* h)" particle-fluids/sph-water/src/main.cpp`

```
1349:        return 8.0f / (float(M_PI) * h * h * h * h);
```

Exactly one hit, inside `grad_kernel_norm_3d_value`.

### B.4 — `git diff --stat particle-fluids/sph-water/src/main.cpp`

```
 particle-fluids/sph-water/src/main.cpp | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
```

### B.5 — `git diff particle-fluids/sph-water/src/main.cpp`

```diff
diff --git a/particle-fluids/sph-water/src/main.cpp b/particle-fluids/sph-water/src/main.cpp
index 3568cb2..3ce4bc6 100644
--- a/particle-fluids/sph-water/src/main.cpp
+++ b/particle-fluids/sph-water/src/main.cpp
@@ -1346,7 +1346,7 @@ int main(int argc, char** argv) {
     };
     auto grad_kernel_norm_3d_value = [&]() {
         float h = rt.supportRadius;
-        return 48.0f / (float(M_PI) * h * h * h * h);
+        return 8.0f / (float(M_PI) * h * h * h * h);
     };

     auto pack_sort_uniform = [&]() {
```

---

## C. Behavioral expectations

User-visible behavioral change from this commit in isolation is expected
to be **zero or marginal**. The current pressure-solve stencil is a
placeholder (per the commit-2 spec) and does not exercise the gradient
in a way that surfaces the 6× scaling error as a visible defect — the
placeholder masks the gradient error. The structural correctness gate
this commit satisfies is for **commit 2's pressure-stencil rewrite**:
once the real DFSPH pressure-force stencil lands, it will multiply
density-error-driven pressures by this gradient-kernel norm, and only
then does the 6× error become a visible / numerically significant
defect. Landing the host-norm fix first lets commit 2 land cleanly
without bundling two correctness changes in one diff.

The user should run the dam-break preset post-edit and note whether
anything visibly shifted; if the run looks identical to pre-edit, that
is the expected outcome and not a regression.
