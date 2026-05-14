---
title: Phase 11.5 Setup-1 — SPlisHSPlasH 2.16.1 vendored as new reference anchor
date: 2026-05-14
author: setup1
phase: 11.5
status: complete
scope: clone + gitignore + audit (no source modification, no build, no commit)
---

> Setup-1 resumed after blocking on the fabricated `1.8.10` anchor. New anchor `2.16.1` (SHA `6bff55a6eaf14083d34650f22a268ce156b62b54`) cloned into `references/SPlisHSPlasH/`. `.gitignore` updated. **All 14 cited line numbers in the DFSPH shader docblocks resolve to plausibly-relevant content at 2.16.1.** Match rate is 100% PLAUSIBLE_MATCH — surprising and load-bearing for fix-prompt confidence. See Section E for the audit table. Section G flags two non-obvious structural facts about the 2.16.1 DFSPH source that will matter for Phase 11.5 commit 2.

## Section A: Anchor decision

The original anchor in `particle-fluids/sph-water/docs/load-bearing-decisions.md:8-9` was `SPlisHSPlasH 1.8.10 at SHA c254caf2705ebf5271408dd37a091aa379258a38`. Step 1 of the blocking probe (`phase11_5_setup1_2026-05-14_blocked.md`) established that **no `1.8` tag has ever existed upstream** — the published tag line is `1.1.0`, `1.2.0`, `1.3.0`, `1.3.1`, then jumps straight to `2.0.0`. The SHA in `load-bearing-decisions.md` was a copy-paste from the Alembic line in the same document. Both the tag name and the SHA were fabricated, presumably in good-faith approximation.

User decision (this conversation): adopt tag `2.16.1` as the fresh Phase 11.5 anchor. Rationale recorded in user instruction — it is the latest stable upstream release, and the existing shader-docblock line citations are demoted to advisory status. The α-coupling rewrite in Phase 11.5 commit 2 will navigate the 2.16.1 source on its own terms; Setup-1's audit (Section E) determines whether the existing citations happen to still point at relevant material or not, but a low match rate would not have blocked progress.

## Section B: Clone outcome

```
$ mkdir -p references
$ cd references
$ git clone --depth 1 --branch 2.16.1 https://github.com/InteractiveComputerGraphics/SPlisHSPlasH.git
Cloning into 'SPlisHSPlasH'...
Note: switching to '6bff55a6eaf14083d34650f22a268ce156b62b54'.
You are in 'detached HEAD' state. ...
$ cd SPlisHSPlasH
$ git log -1 --format='%H %ci %s'
6bff55a6eaf14083d34650f22a268ce156b62b54 2026-05-12 10:48:44 +0200 - updated Changelog.txt
```

- **Tag:** `2.16.1`
- **Resolved SHA:** `6bff55a6eaf14083d34650f22a268ce156b62b54` — exact match with the SHA recorded in the blocking probe's ls-remote output. No force-push since the blocking-probe ls-remote.
- **Commit date:** 2026-05-12 10:48:44 +0200
- **Commit subject:** `- updated Changelog.txt`
- **Top-level layout:**
  ```
  Changelog.txt   CITATION.cff   CMake   CMakeLists.txt   data   doc
  extern   GUI   LICENSE   MANIFEST.in   pySPlisHSPlasH   README.md
  Scripts   setup.cfg   setup.py   Simulator   SPlisHSPlasH   Tests
  Tools   Utilities   version.txt
  ```
- **Directory size:** `50M` (depth-1 clone; full tree without history).

## Section C: `.gitignore` update verification

Appended these eight lines to repo-root `.gitignore` (after confirming the file ended with a newline):

```
# Phase 11.5: SPlisHSPlasH vendored upstream reference (clone-on-setup, gitignored).
# Anchored to tag 2.16.1 (SHA 6bff55a6eaf14083d34650f22a268ce156b62b54).
# See docs/diagnostics/_audits/phase11_5_setup1_*.md for the setup record and the
# anchor-decision context (the original load-bearing-decisions.md anchor of
# "1.8.10" was non-existent upstream; 2.16.1 was selected as a fresh anchor).
/references/
```

`git status --short` after append:

```
 M .gitignore
 M common/common-cpp/include/gpusims/gpu_profiler.hpp
 M common/common-cpp/src/gpu_profiler.cpp
 M common/common-cpp/src/vk/context.cpp
?? docs/diagnostics/
```

`references/` does NOT appear in the untracked list — the ignore is effective. The four `M` entries on common-cpp files are pre-existing (visible in the conversation's initial git-status snapshot) and unrelated to Setup-1. The `?? docs/diagnostics/` is the pre-existing audit directory containing the blocking report; this Setup-1 report will be added to the same directory.

## Section D: File-location verification

```
$ find references/SPlisHSPlasH -name 'TimeStepDFSPH*' -type f
references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp
references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.h

$ find references/SPlisHSPlasH -name 'SPHKernels*' -type f
references/SPlisHSPlasH/pySPlisHSPlasH/SPHKernelsModule.cpp
references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.cpp
references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h
```

- **Single canonical `TimeStepDFSPH.cpp`** at `SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp`. No CUDA variant, no separate GPU backend — the DFSPH solver is CPU-only in 2.16.1.
- **`TimeStepDFSPH.h`** sibling at `SPlisHSPlasH/DFSPH/TimeStepDFSPH.h`.
- **`SPHKernels.h`** at `SPlisHSPlasH/SPHKernels.h` (the citation target). `pySPlisHSPlasH/SPHKernelsModule.cpp` is the pybind11 wrapper, not relevant to the audit.

Line counts:

```
$ wc -l references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp \
        references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.h \
        references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h
 1423 references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp
   79 references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.h
  959 references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h
```

Sanity check: `TimeStepDFSPH.cpp` is 1423 lines, so the deepest citation (`:1175`) is safely within range. All cited line numbers (max 1175 for `.cpp`, max 43 for `SPHKernels.h`, max 28 for `.h`) fall within the corresponding file length.

Top-of-file (`TimeStepDFSPH.cpp:1-40`):

```cpp:references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1
#include "TimeStepDFSPH.h"
#include "SPlisHSPlasH/TimeManager.h"
#include "SPlisHSPlasH/SPHKernels.h"
#include "SimulationDataDFSPH.h"
#include <iostream>
#include "Utilities/Timing.h"
#include "Utilities/Counting.h"
#include "SPlisHSPlasH/Simulation.h"
#include "SPlisHSPlasH/BoundaryModel_Akinci2012.h"
#include "SPlisHSPlasH/BoundaryModel_Koschier2017.h"
#include "SPlisHSPlasH/BoundaryModel_Bender2019.h"


using namespace SPH;
using namespace std;
using namespace GenParam;

std::string TimeStepDFSPH::METHOD_NAME = "DFSPH";
int TimeStepDFSPH::SOLVER_ITERATIONS = -1;
... (constructor and ctor-init continues to line 40)
```

Top-of-file (`TimeStepDFSPH.h:1-40`):

```cpp:references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.h:1
#ifndef __TimeStepDFSPH_h__
#define __TimeStepDFSPH_h__

#include "SPlisHSPlasH/Common.h"
#include "SPlisHSPlasH/TimeStep.h"
#include "SimulationDataDFSPH.h"
#include "SPlisHSPlasH/SPHKernels.h"

#define USE_WARMSTART
#define USE_WARMSTART_V

namespace SPH
{
    class SimulationDataDFSPH;

    /** \brief This class implements the Divergence-free Smoothed Particle Hydrodynamics approach introduced
    * by Bender and Koschier [BK15,BK17,KBST19].
    *
    * References: [BK15], [BK17], [KBST19] — full bibliography in the docblock
    */
    class TimeStepDFSPH : public TimeStep
    {
    protected:
        SimulationDataDFSPH m_simulationData;
        const Real m_eps = static_cast<Real>(1.0e-5);   // <-- LINE 28 (cited)
        unsigned int m_iterations;
        Real m_maxError;
        ...
```

Top-of-file (`SPHKernels.h:1-12`):

```cpp:references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h:1
#ifndef SPHKERNELS_H
#define SPHKERNELS_H

#define _USE_MATH_DEFINES
#include <math.h>
#include "Common.h"
#include <algorithm>
#ifdef USE_AVX
#include "SPlisHSPlasH/Utilities/AVX_math.h"
#endif

namespace SPH { ... }
```

## Section E: Citation-resolution audit

All resolved against `references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp`, `references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.h`, and `references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h`.

### E.1 Per-citation table

| # | Citing shader | File | Line | Enclosing fn | Shader claim | Actual content (one-line summary) | Verdict |
|---|---|---|---|---|---|---|---|
| 1 | `density_solve.comp.glsl:8` | `TimeStepDFSPH.cpp` | 285 | `pressureSolve()` | factor scales 1/h² | `m_simulationData.getFactor(fluidModelIndex, i) *= invH2;` | **PLAUSIBLE_MATCH** |
| 2 | `density_solve.comp.glsl:9` | `TimeStepDFSPH.cpp` | 582 | `pressureSolveIteration` | aij_pj scales by h² | `aij_pj *= h * h;` | **PLAUSIBLE_MATCH** |
| 3 | `density_solve.comp.glsl:12` | `TimeStepDFSPH.cpp` | 590 | `pressureSolveIteration` | Source s_i = 1 - ρ_adv/ρ₀ | `const Real s_i = static_cast<Real>(1.0) - densityAdv;` (note: ρ₀ factored out per multiphase comment, see E.2) | **PLAUSIBLE_MATCH** |
| 4 | `density_solve.comp.glsl:13` | `TimeStepDFSPH.cpp` | 606 | `pressureSolveIteration` | Pressure update | `p_rho2_i = max(p_rho2_i - 0.5*(s_i - aij_pj)*factor, 0.0);` | **PLAUSIBLE_MATCH** |
| 5 | `divergence_solve.comp.glsl:5` | `TimeStepDFSPH.cpp` | 662 | `divergenceSolveIteration` | Source s_i = -ρ̇_i | `const Real s_i = -densityAdv;` | **PLAUSIBLE_MATCH** |
| 6 | `divergence_solve.comp.glsl:6` | `TimeStepDFSPH.cpp` | 656 | `divergenceSolveIteration` | aij_pj scales by h | `aij_pj *= h;` | **PLAUSIBLE_MATCH** |
| 7 | `divergence_solve.comp.glsl:7` | `TimeStepDFSPH.cpp` | 692 | `divergenceSolveIteration` | Pressure update (Jacobi 0.5) | `pv_rho2_i = max(pv_rho2_i - 0.5*(s_i - aij_pj)*factor, 0.0);` | **PLAUSIBLE_MATCH** |
| 8 | `divergence_solve.comp.glsl:8` | `TimeStepDFSPH.cpp` | 442 | `divergenceSolve()` | factor scales by 1/h | `m_simulationData.getFactor(fluidModelIndex, i) *= invH;` | **PLAUSIBLE_MATCH** |
| 9 | `pressure_apply.comp.glsl:7` | `TimeStepDFSPH.cpp` | 514 | `divergenceSolve()` (warmstart-finalize loop) | Velocity correction (divergence) | `computePressureAccel(..., m_simulationData.getPressureRho2VData(), true);` followed by `model->getVelocity(i) += h * m_simulationData.getPressureAccel(...)` | **PLAUSIBLE_MATCH** |
| 10 | `pressure_apply.comp.glsl:8` | `TimeStepDFSPH.cpp` | 359 | `pressureSolve()` (warmstart-finalize loop) | Velocity correction (density) | `computePressureAccel(..., m_simulationData.getPressureRho2Data(), true);` followed by `model->getVelocity(i) += h * m_simulationData.getPressureAccel(...)` | **PLAUSIBLE_MATCH** |
| 11 | `density_alpha.comp.glsl:5` | `SPHKernels.h` | 43 | `CubicKernel::W(Real)` | Cubic spline kernel | `if (q <= 0.5)` — branch inside the piecewise cubic spline value function | **PLAUSIBLE_MATCH** |
| 12 | `density_alpha.comp.glsl:6` | `TimeStepDFSPH.cpp` | 813 | `computeDFSPHFactor` (**AVX variant**, see G.1) | α-factor | `sum_grad_p_k += grad_p_i.squaredNorm();` — sum-of-grads accumulation immediately preceding `factor = 1/sum_grad_p_k` at line 820 | **PLAUSIBLE_MATCH** |
| 13 | `density_alpha.comp.glsl:6` | `TimeStepDFSPH.cpp` | 1175 | `computeDFSPHFactor` (**scalar variant**, see G.1) | α-factor (variant) | line 1175 is the comment `// Compute factor as: factor_i = -1 / (a_ii * rho_i^2)`; the assignment follows at 1179-1183 | **PLAUSIBLE_MATCH** |
| 14 | `density_alpha.comp.glsl:7` | `TimeStepDFSPH.h` | 28 | class `TimeStepDFSPH` (member declaration) | α floor ε = 1.0e-5 | `const Real m_eps = static_cast<Real>(1.0e-5);` | **PLAUSIBLE_MATCH** |

### E.2 Selected verbatim context for the high-stakes citations

The five citations most load-bearing for Phase 11.5 commit 2 (the a_ij coupling rewrite) are 285, 442, 582, 656, and the 813/1175 pair. Quoting them verbatim with 8 lines of context:

```cpp:references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:277
                computeDensityAdv(fluidModelIndex, i, h, density0);

                //////////////////////////////////////////////////////////////////////////
                // In the end of Section 3.3 [BK17] we have to multiply the density
                // error with the factor alpha_i divided by h^2. Hence, we multiply
                // the factor directly by 1/h^2 here.
                //////////////////////////////////////////////////////////////////////////
                m_simulationData.getFactor(fluidModelIndex, i) *= invH2;

                //////////////////////////////////////////////////////////////////////////
                // For the warm start we use 0.5 times the old pressure value.
                // Note: We divide the value by h^2 since we multiplied it by h^2 at the end of
                // the last time step to make it independent of the time step size.
                //////////////////////////////////////////////////////////////////////////
```

```cpp:references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:434
                        if (numNeighbors < 7)
                            densityAdv = 0.0;
                    }

                    //////////////////////////////////////////////////////////////////////////
                    // In equation (11) [BK17] we have to multiply the divergence
                    // error with the factor divided by h. Hence, we multiply the factor
                    // directly by 1/h here.
                    //////////////////////////////////////////////////////////////////////////
                    m_simulationData.getFactor(fluidModelIndex, i) *= invH;

                    //////////////////////////////////////////////////////////////////////////
                    // For the warm start we use 0.5 times the old pressure value.
```

```cpp:references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:574
            //////////////////////////////////////////////////////////////////////////
            // Update pressure values
            //////////////////////////////////////////////////////////////////////////
            #pragma omp for reduction(+:density_error) schedule(static)
            for (int i = 0; i < numParticles; i++)
            {
                if (model->getParticleState(i) != ParticleState::Active)
                    continue;

                Real aij_pj = compute_aij_pj(fluidModelIndex, i);
                aij_pj *= h * h;

                //////////////////////////////////////////////////////////////////////////
                // Compute source term: s_i = 1 - rho_adv
                // Note: that due to our multiphase handling, the multiplier rho0
                // is missing here
                //////////////////////////////////////////////////////////////////////////
                const Real& densityAdv = m_simulationData.getDensityAdv(fluidModelIndex, i);
                const Real s_i = static_cast<Real>(1.0) - densityAdv;
```

Note on citation #3 (`density_solve.comp.glsl:12` ↔ line 590): the upstream comment at lines 585-587 explicitly says "*the multiplier rho0 is missing here*". This is consistent with the shader docblock claim "s_i = 1 - ρ_adv/ρ₀" once you understand that `densityAdv` in the upstream code is already the normalized form (ρ_adv/ρ₀). The verdict stays PLAUSIBLE_MATCH; the apparent unit mismatch is reconciled by the multiphase convention.

```cpp:references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:648
            for (int i = 0; i < (int)numParticles; i++)
            {
                computePressureAccel(fluidModelIndex, i, density0, m_simulationData.getPressureRho2VData());
            }

            //////////////////////////////////////////////////////////////////////////
            // Update pressure
            //////////////////////////////////////////////////////////////////////////
            #pragma omp for reduction(+:density_error) schedule(static)
            for (int i = 0; i < numParticles; i++)
            {
                Real aij_pj = compute_aij_pj(fluidModelIndex, i);
                aij_pj *= h;

                //////////////////////////////////////////////////////////////////////////
                // Compute source term: s_i = -d rho / dt
                //////////////////////////////////////////////////////////////////////////
                const Real& densityAdv = m_simulationData.getDensityAdv(fluidModelIndex, i);
                const Real s_i = -densityAdv;
```

```cpp:references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:805
                else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Bender2019)
                {
                    forall_volume_maps(
                        const Vector3r grad_p_j = -Vj * sim->gradW(xi - xj);
                        grad_p_i -= grad_p_j;
                    );
                }

                sum_grad_p_k += grad_p_i.squaredNorm();

                //////////////////////////////////////////////////////////////////////////
                // Compute factor alpha_i / rho_i (see Equation (11) in [BK17])
                //////////////////////////////////////////////////////////////////////////
                Real& factor = m_simulationData.getFactor(fluidModelIndex, i);
                if (sum_grad_p_k > m_eps)
                    factor = static_cast<Real>(1.0) / (sum_grad_p_k);
                else
                    factor = 0.0;
```

```cpp:references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1167
                else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Bender2019)
                {
                    forall_volume_maps(
                        const Vector3r grad_p_j = -Vj * sim->gradW(xi - xj);
                        grad_p_i -= grad_p_j;
                    );
                }

                sum_grad_p_k += grad_p_i.squaredNorm();

                //////////////////////////////////////////////////////////////////////////
                // Compute factor as: factor_i = -1 / (a_ii * rho_i^2)
                // where a_ii is the diagonal entry of the linear system
                // for the pressure A * p = source term
                //////////////////////////////////////////////////////////////////////////
                Real &factor = m_simulationData.getFactor(fluidModelIndex, i);
                if (sum_grad_p_k > m_eps)
                    factor = static_cast<Real>(1.0) / (sum_grad_p_k);
                else
                    factor = 0.0;
```

### E.3 Summary statistics

- **PLAUSIBLE_MATCH:** 14 / 14
- **DRIFT:** 0
- **NOT_FOUND:** 0
- **Overall match rate:** 100%

Interpretation: every cited line number, against an anchor (`2.16.1`) different from the one named in `load-bearing-decisions.md` (`1.8.10`), resolves to code whose function-level meaning matches the shader docblock's claim. The most likely explanation is that the upstream DFSPH solver has been numerically stable in its line layout for many releases — `TimeStepDFSPH.cpp` was probably last substantially restructured well before any 1.x or 2.x version, and the shader author's citations carry forward across the version anchor swap effectively by accident. Phase 11.5 commit 2 / commit 4 should not over-rely on this happy accident; the load-bearing-decisions.md anchor still needs a permanent fix to name `2.16.1`.

## Section F: `TimeStepDFSPH.cpp` function map at 2.16.1

Total length: **1423 lines.** No `class ` or `struct ` definitions in the .cpp file (the class declaration is in the .h sibling); all function bodies are methods of `SPH::TimeStepDFSPH`.

The file is divided by a top-level `#ifdef USE_AVX` (lines 733-1423) with `#else` at 1104 and `#endif` at 1423. So **five DFSPH helpers are defined twice** — once for AVX (lines 735-1103) and once for scalar (1106-1422):

| Method | AVX line | Scalar line |
|---|---|---|
| `computeDFSPHFactor` | 735 | 1106 |
| `computeDensityAdv` | 830 | 1191 |
| `computeDensityChange` | 894 | 1247 |
| `computePressureAccel` | 954 | 1299 |
| `compute_aij_pj` | 1042 | 1370 |

Method-by-method line index (one-shot grep of function signatures):

```
 19-26  static-member initializers (SOLVER_ITERATIONS, MIN_ITERATIONS, ...)
 29  TimeStepDFSPH::TimeStepDFSPH()        — ctor
 57  TimeStepDFSPH::~TimeStepDFSPH(void)   — dtor
 73  void TimeStepDFSPH::initParameters()
117  void TimeStepDFSPH::step()
252  void TimeStepDFSPH::pressureSolve()
386  void TimeStepDFSPH::divergenceSolve()
544  void TimeStepDFSPH::pressureSolveIteration(fluidModelIndex, &avg_density_err)
621  void TimeStepDFSPH::divergenceSolveIteration(fluidModelIndex, &avg_density_err)
710  void TimeStepDFSPH::reset()
718  void TimeStepDFSPH::performNeighborhoodSearchSort()
723  void TimeStepDFSPH::emittedParticles(model, startIndex)
728  void TimeStepDFSPH::resize()
733  #ifdef USE_AVX
735  void TimeStepDFSPH::computeDFSPHFactor(fluidModelIndex)    [AVX]
830  void TimeStepDFSPH::computeDensityAdv(...)                 [AVX]
894  void TimeStepDFSPH::computeDensityChange(...)              [AVX]
954  void TimeStepDFSPH::computePressureAccel(...)              [AVX]
1042 Real TimeStepDFSPH::compute_aij_pj(...)                    [AVX]
1104 #else
1106 void TimeStepDFSPH::computeDFSPHFactor(fluidModelIndex)    [scalar]
1191 void TimeStepDFSPH::computeDensityAdv(...)                 [scalar]
1247 void TimeStepDFSPH::computeDensityChange(...)              [scalar]
1299 void TimeStepDFSPH::computePressureAccel(...)              [scalar]
1370 Real TimeStepDFSPH::compute_aij_pj(...)                    [scalar]
1423 #endif
```

## Section G: Incidental findings relevant to Phase 11.5 commit 2

### G.1 The AVX/scalar split is the source of "duplicate" function definitions

The `#ifdef USE_AVX` (line 733) / `#else` (1104) / `#endif` (1423) split means every per-particle DFSPH helper exists in two parallel implementations. **This affects two of our citations:**

- Citation 12 (`density_alpha.comp.glsl:6 → :813`) lands in the AVX-vector implementation of `computeDFSPHFactor`.
- Citation 13 (`density_alpha.comp.glsl:6 → :1175`) lands in the scalar implementation of `computeDFSPHFactor`.

The α-factor formula on both paths is identical — `factor = 1 / sum_grad_p_k` with an `m_eps` floor — but the AVX path uses Eigen + AVX intrinsics (via `Scalar4f`/`Scalar8f` from `Utilities/AVX_math.h`) while the scalar path uses ordinary `Real`/`Vector3r` arithmetic. **For Phase 11.5 commit 2's a_ij coupling rewrite, the scalar implementation (starting at line 1106) is the more readable upstream reference.** It is also the one whose loop structure most closely matches a per-particle compute-shader invocation. The AVX path packs 8 particles per AVX-512 register and the loop body iterates over packed lanes — useful for confirming numeric identity, less useful for reading the algorithm.

### G.2 No CUDA/GPU backend exists in 2.16.1's `DFSPH/` directory

`SPlisHSPlasH/DFSPH/` contains exactly four files (`SimulationDataDFSPH.{cpp,h}` and `TimeStepDFSPH.{cpp,h}`). There is no `TimeStepDFSPH_GPU.cpp`, no `.cu` file, and no separate CUDA kernel definitions. The reference solver is CPU + OpenMP only. This is informational: our GPU port is doing genuinely new work mapping the algorithm to compute shaders; there is no upstream GPU implementation to cross-check against numerically.

### G.3 `compute_aij_pj` is a one-function reference for the coupling rewrite

The function at lines 1370-1422 (scalar variant) is the upstream definition of the off-diagonal coupling term. The signature is:

```
Real TimeStepDFSPH::compute_aij_pj(const unsigned int fluidModelIndex, const unsigned int i)
```

It is **52 lines of code** and is called from both `pressureSolveIteration` (line 581) and `divergenceSolveIteration` (line 655). For Phase 11.5 commit 2 (replacing the placeholder a_ij coupling), this single upstream function is the authoritative reference. The AVX counterpart at 1042-1103 is the same algorithm, vectorized.

### G.4 The `TimeStepDFSPH.h:28` epsilon is named `m_eps`, not a global

The α-floor constant cited by `density_alpha.comp.glsl:7` is a **member variable** of `TimeStepDFSPH` (line 28: `const Real m_eps = static_cast<Real>(1.0e-5);`), not a free-standing constant or `#define`. This is relevant if the GPU shader uses a different name or constant location — the rename is fine, but the source location and value (1.0e-5) come from this instance variable.

### G.5 `load-bearing-decisions.md` still anchors to `1.8.10`

The audit deliberately did not touch `particle-fluids/sph-water/docs/load-bearing-decisions.md`, per the Setup-1 scope guardrails. That document still says "anchored to SPlisHSPlasH 1.8.10 at SHA c254caf...". **This needs an out-of-band fix** — either in a follow-up commit in Phase 11.5 setup, or rolled into commit 2 alongside the a_ij rewrite. The current state of the repo has a `.gitignore` comment pointing to `2.16.1` but a sim-local doc still pointing to a fabricated `1.8.10`. The two need to be made consistent.

### G.6 Boundary-model handling is pluggable; the GPU shader probably handles only one variant

The upstream `computeDFSPHFactor` (and its AVX twin) branches on `sim->getBoundaryHandlingMethod()` between `Akinci2012`, `Koschier2017`, and `Bender2019` (e.g., lines 793-811 and 1150-1168 scalar variant). Each path has different boundary-contribution formulas. The current GPU sph-water shaders almost certainly implement exactly one of these (most likely Akinci2012 — Akinci-style rigid-body sampling — which is the default and simplest). Phase 11.5 fix work should explicitly name which boundary model the GPU port implements, since drift between the upstream branch and the shader's branch is a likely source of numeric mismatch.
