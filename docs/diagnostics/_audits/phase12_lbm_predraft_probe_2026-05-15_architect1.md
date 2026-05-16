---
title: Phase 12 LBM final pre-draft probe — Krüger book code + local Vulkan caps
date: 2026-05-15
author: architect1
phase: 12
status: probe (read-only)
scope: Krüger LBM book code clone + characterization, local vulkaninfo capture for subgroup-size-control verification, D2Q9 weights sanity check
---

> Read-only. No source modification, no commits, no edits to `references/`,
> no edits to `.gitignore`. The Krüger book code was cloned to a scratch
> path **outside** the repo tree at `/tmp/krueger-lbm-probe-2026-05-15`.
> Section B (`vulkaninfo`) is **blocked** — the `vulkan-tools` package is
> not installed on this host. Section A surfaces a load-bearing finding:
> the Krüger companion code is **D2Q9 only, not D3Q19**.

---

## A — Krüger book code: clone + characterization

### A.1 Resolved HEAD SHA

`git -C /tmp/krueger-lbm-probe-2026-05-15 rev-parse HEAD` returns:

```
6e2c592fdc3592c14dfd52f860fc1ceea930bcb0
```

Clone command was `git clone --depth 1 https://github.com/lbm-principles-practice/code.git`.
Report-only — pinning to this SHA in a future vendoring commit is a separate
decision out of scope for this probe.

### A.2 License confirmation — `LICENSE.txt` verbatim

```
Copyright (c) 2016 Timm Krüger, Halim Kusumaatmaja, Alexandr Kuzmin, Orest Shardt, Goncalo Silva, Erlend Magnus Viggen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

The text is the standard MIT license body without the literal header line
"MIT License". Per the SPDX identifier rule (MIT = the body matches the
canonical MIT body), this is **MIT**. Compatible with the GPU-Sims MIT
license. Confirmed.

### A.3 `tree -L 2 /tmp/krueger-lbm-probe-2026-05-15`

```
.
├── chapter11
│   └── IBLBM_2D_Poiseuille.cpp
├── chapter13
│   ├── cpu
│   ├── cpu_intro
│   ├── gpu
│   ├── matlab
│   ├── mpi
│   ├── openmp
│   └── python3
├── chapter5
│   ├── couette_BB.m
│   ├── couette_wetnode.m
│   ├── poiseuille_BB.m
│   └── poiseuille_wetnode.m
├── chapter6
│   ├── force_poiseuille_BB.m
│   └── force_poiseuille_wetnode.m
├── chapter8
│   ├── cylinder.cpp
│   ├── film_antibb.cpp
│   ├── film_inamuro.cpp
│   ├── film_uniform.cpp
│   ├── gaussian_1d_bgk.cpp
│   ├── gaussian_1d_magic12.cpp
│   ├── gaussian_1d_magic6.cpp
│   ├── gaussian_2d_bgk.cpp
│   └── gaussian_2d_trt.cpp
├── chapter9
│   └── shanchen.cpp
├── chapter11
│   └── IBLBM_2D_Poiseuille.cpp
├── LICENSE.txt
└── README.md
```

19 files at the leaves, 14 directories total. `README.md` and `LICENSE.txt`
at root. No CMakeLists / Makefile at root.

### A.4 Per-chapter one-line summaries

- **chapter5/** — MATLAB-only. Pedagogical 2D channel-flow exemplars
  (Poiseuille + Couette) with two boundary-condition variants each:
  `*_BB.m` (halfway bounce-back) and `*_wetnode.m` (non-equilibrium /
  Zou-He wet-node BCs). All 2D, D2Q9.
- **chapter6/** — MATLAB-only. 2D forced Poiseuille (body-force variant)
  with the same `_BB.m` / `_wetnode.m` pair. All 2D, D2Q9.
- **chapter8/** — C++ multi-flow exemplars. `cylinder.cpp` is 2D advection-
  diffusion past a circular cylinder (concentration field, not solid-wall
  obstacle for momentum). `film_*.cpp` are 2D liquid-film flows with
  Inamuro/anti-bounce-back BCs. `gaussian_*.cpp` are diffusion verifications
  in 1D and 2D, BGK and TRT collision variants. All 2D.
- **chapter9/** — Single C++ file: `shanchen.cpp`, the 2D Shan-Chen
  multiphase model.
- **chapter11/** — Single C++ file: `IBLBM_2D_Poiseuille.cpp`, immersed-
  boundary LBM. 2D.
- **chapter13/** — Performance-oriented variants of the SAME 2D Taylor-Green
  vortex problem in seven backends: `cpu/` (optimized fused stream-collide),
  `cpu_intro/` (educational split-kernel version), `gpu/` (CUDA), `matlab/`,
  `mpi/blocking/` and `mpi/nonblocking/`, `openmp/`, `python3/`. **Note:
  none of these are 3D, and none of these are LBM with obstacle geometry.**

### A.5 D3Q19 + tau hit-search

`grep -rln "D3Q19" /tmp/krueger-lbm-probe-2026-05-15`:

```
(no matches)
```

`grep -rln -E "(D2Q9|D3Q15|D3Q19|D3Q27)" /tmp/krueger-lbm-probe-2026-05-15`:

```
/tmp/krueger-lbm-probe-2026-05-15/chapter11/IBLBM_2D_Poiseuille.cpp
```

— and the one hit is in a comment, not a lattice-definition macro
(verified by `head`'ing the file; the file still uses `npop = 9`).

**Critical: zero hits for `D3Q19` anywhere in the repository.** All
implementations are D2Q9. The Krüger companion code does NOT contain a 3D
LBM reference implementation. This contradicts the brief's implicit
assumption that Krüger book code is a viable D3Q19 anchor candidate.

`grep -rln "tau" --include='*.cpp' --include='*.cu' --include='*.h*'`
(top 20 hits):

```
/tmp/krueger-lbm-probe-2026-05-15/chapter13/cpu/LBM.h
/tmp/krueger-lbm-probe-2026-05-15/chapter13/cpu/main.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter13/mpi/blocking/LBM.h
/tmp/krueger-lbm-probe-2026-05-15/chapter13/openmp/main.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter13/cpu/LBM.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter13/openmp/LBM.h
/tmp/krueger-lbm-probe-2026-05-15/chapter13/gpu/main.cu
/tmp/krueger-lbm-probe-2026-05-15/chapter13/mpi/nonblocking/LBM.h
/tmp/krueger-lbm-probe-2026-05-15/chapter13/mpi/blocking/LBM.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter13/mpi/blocking/main.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter13/openmp/LBM.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter13/gpu/LBM.cu
/tmp/krueger-lbm-probe-2026-05-15/chapter11/IBLBM_2D_Poiseuille.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter13/mpi/nonblocking/LBM.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter13/mpi/nonblocking/main.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter13/gpu/LBM.h
/tmp/krueger-lbm-probe-2026-05-15/chapter13/cpu_intro/main.cpp
/tmp/krueger-lbm-probe-2026-05-15/chapter9/shanchen.cpp
```

The relaxation-time constant is consistently defined in chapter 13 as:

`/tmp/krueger-lbm-probe-2026-05-15/chapter13/cpu/LBM.h:29–30`:

```cpp
const double nu = 1.0/6.0;
const double tau = 3.0*nu+0.5;
```

(yielding `tau = 1.0` at `nu = 1/6` — the convenient "near-unity relaxation"
default for Taylor-Green tests).

Inside the collision routines, `tauinv = 2.0/(6.0*nu+1.0)` is used in place
of `1.0/tau` for the BGK relaxation (`omtauinv = 1.0 - tauinv`).

### A.6 D3Q19 BGK collision — verbatim

**There is no D3Q19 BGK collision in the Krüger companion code.** The only
BGK collision implementations are D2Q9. Quoting both the CPU fused
stream-collide kernel and the CUDA equivalent for completeness.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
#### A.6.a CPU fused stream-collide — `chapter13/cpu/LBM.cpp:97–181`

Verbatim:

```cpp
void stream_collide_save(double *f0, double *f1, double *f2, double *r, double *u, double *v, bool save)
{
    // useful constants
    const double tauinv = 2.0/(6.0*nu+1.0); // 1/tau
    const double omtauinv = 1.0-tauinv;     // 1 - 1/tau

    for(unsigned int y = 0; y < NY; ++y)
    {
        for(unsigned int x = 0; x < NX; ++x)
        {
            unsigned int xp1 = (x+1)%NX;
            unsigned int yp1 = (y+1)%NY;
            unsigned int xm1 = (NX+x-1)%NX;
            unsigned int ym1 = (NY+y-1)%NY;
            
            // direction numbering scheme
            // 6 2 5
            // 3 0 1
            // 7 4 8
            
            double ft0 = f0[field0_index(x,y)];
            
            // load populations from adjacent nodes
            double ft1 = f1[fieldn_index(xm1,y,  1)];
            double ft2 = f1[fieldn_index(x,  ym1,2)];
            double ft3 = f1[fieldn_index(xp1,y,  3)];
            double ft4 = f1[fieldn_index(x,  yp1,4)];
            double ft5 = f1[fieldn_index(xm1,ym1,5)];
            double ft6 = f1[fieldn_index(xp1,ym1,6)];
            double ft7 = f1[fieldn_index(xp1,yp1,7)];
            double ft8 = f1[fieldn_index(xm1,yp1,8)];
            
            // compute moments
            double rho = ft0+ft1+ft2+ft3+ft4+ft5+ft6+ft7+ft8;
            double rhoinv = 1.0/rho;
            
            double ux = rhoinv*(ft1+ft5+ft8-(ft3+ft6+ft7));
            double uy = rhoinv*(ft2+ft5+ft6-(ft4+ft7+ft8));
            
            // only write to memory when needed
            if(save)
            {
                r[scalar_index(x,y)] = rho;
                u[scalar_index(x,y)] = ux;
                v[scalar_index(x,y)] = uy;
            }
            
            // now compute and relax to equilibrium
            // note that
            // feq_i  = w_i rho [1 + 3(ci . u) + (9/2) (ci . u)^2 - (3/2) (u.u)]
            // feq_i  = w_i rho [1 - 3/2 (u.u) + (ci . 3u) + (1/2) (ci . 3u)^2]
            // feq_i  = w_i rho [1 - 3/2 (u.u) + (ci . 3u){ 1 + (1/2) (ci . 3u) }]
            
            // temporary variables
            double tw0r = tauinv*w0*rho; //   w[0]*rho/tau 
            double twsr = tauinv*ws*rho; // w[1-4]*rho/tau
            double twdr = tauinv*wd*rho; // w[5-8]*rho/tau
            double omusq = 1.0 - 1.5*(ux*ux+uy*uy); // 1-(3/2)u.u
            
            double tux = 3.0*ux;
            double tuy = 3.0*uy;
            
            
            f0[field0_index(x,y)]    = omtauinv*ft0  + tw0r*(omusq);
            
            double cidot3u = tux;
            f2[fieldn_index(x,y,1)]  = omtauinv*ft1  + twsr*(omusq + cidot3u*(1.0+0.5*cidot3u));
            cidot3u = tuy;
            f2[fieldn_index(x,y,2)]  = omtauinv*ft2  + twsr*(omusq + cidot3u*(1.0+0.5*cidot3u));
            cidot3u = -tux;
            f2[fieldn_index(x,y,3)]  = omtauinv*ft3  + twsr*(omusq + cidot3u*(1.0+0.5*cidot3u));
            cidot3u = -tuy;
            f2[fieldn_index(x,y,4)]  = omtauinv*ft4  + twsr*(omusq + cidot3u*(1.0+0.5*cidot3u));
            
            cidot3u = tux+tuy;
            f2[fieldn_index(x,y,5)]  = omtauinv*ft5  + twdr*(omusq + cidot3u*(1.0+0.5*cidot3u));
            cidot3u = tuy-tux;
            f2[fieldn_index(x,y,6)]  = omtauinv*ft6  + twdr*(omusq + cidot3u*(1.0+0.5*cidot3u));
            cidot3u = -(tux+tuy);
            f2[fieldn_index(x,y,7)]  = omtauinv*ft7  + twdr*(omusq + cidot3u*(1.0+0.5*cidot3u));
            cidot3u = tux-tuy;
            f2[fieldn_index(x,y,8)]  = omtauinv*ft8  + twdr*(omusq + cidot3u*(1.0+0.5*cidot3u));
        }
    }
}
```

#### A.6.b CUDA fused stream-collide — `chapter13/gpu/LBM.cu:172–253`

Verbatim:

```cpp
__global__ void gpu_stream_collide_save(double *f0, double *f1, double *f2, double *r, double *u, double *v, bool save)
{
    // useful constants
    const double tauinv = 2.0/(6.0*nu+1.0); // 1/tau
    const double omtauinv = 1.0-tauinv;     // 1 - 1/tau

    unsigned int y = blockIdx.y;
    unsigned int x = blockIdx.x*blockDim.x+threadIdx.x;
    
    unsigned int xp1 = (x+1)%NX;
    unsigned int yp1 = (y+1)%NY;
    unsigned int xm1 = (NX+x-1)%NX;
    unsigned int ym1 = (NY+y-1)%NY;
    
    // direction numbering scheme
    // 6 2 5
    // 3 0 1
    // 7 4 8
    
    double ft0 = f0[gpu_field0_index(x,y)];
    
    // load populations from adjacent nodes
    double ft1 = f1[gpu_fieldn_index(xm1,y,  1)];
    double ft2 = f1[gpu_fieldn_index(x,  ym1,2)];
    double ft3 = f1[gpu_fieldn_index(xp1,y,  3)];
    double ft4 = f1[gpu_fieldn_index(x,  yp1,4)];
    double ft5 = f1[gpu_fieldn_index(xm1,ym1,5)];
    double ft6 = f1[gpu_fieldn_index(xp1,ym1,6)];
    double ft7 = f1[gpu_fieldn_index(xp1,yp1,7)];
    double ft8 = f1[gpu_fieldn_index(xm1,yp1,8)];
    
    // compute moments
    double rho = ft0+ft1+ft2+ft3+ft4+ft5+ft6+ft7+ft8;
    double rhoinv = 1.0/rho;
    
    double ux = rhoinv*(ft1+ft5+ft8-(ft3+ft6+ft7));
    double uy = rhoinv*(ft2+ft5+ft6-(ft4+ft7+ft8));
    
    // only write to memory when needed
    if(save)
    {
        r[gpu_scalar_index(x,y)] = rho;
        u[gpu_scalar_index(x,y)] = ux;
        v[gpu_scalar_index(x,y)] = uy;
    }
    
    // now compute and relax to equilibrium
    // note that
    // relax to equilibrium
    // feq_i  = w_i rho [1 + 3(ci . u) + (9/2) (ci . u)^2 - (3/2) (u.u)]
    // feq_i  = w_i rho [1 - 3/2 (u.u) + (ci . 3u) + (1/2) (ci . 3u)^2]
    // feq_i  = w_i rho [1 - 3/2 (u.u) + (ci . 3u){ 1 + (1/2) (ci . 3u) }]
    
    // temporary variables
    double tw0r = tauinv*w0*rho; //   w[0]*rho/tau 
    double twsr = tauinv*ws*rho; // w[1-4]*rho/tau
    double twdr = tauinv*wd*rho; // w[5-8]*rho/tau
    double omusq = 1.0 - 1.5*(ux*ux+uy*uy); // 1-(3/2)u.u
    
    double tux = 3.0*ux;
    double tuy = 3.0*uy;
    
    f0[gpu_field0_index(x,y)]    = omtauinv*ft0  + tw0r*(omusq);
    
    double cidot3u = tux;
    f2[gpu_fieldn_index(x,y,1)]  = omtauinv*ft1  + twsr*(omusq + cidot3u*(1.0+0.5*cidot3u));
    cidot3u = tuy;
    f2[gpu_fieldn_index(x,y,2)]  = omtauinv*ft2  + twsr*(omusq + cidot3u*(1.0+0.5*cidot3u));
    cidot3u = -tux;
    f2[gpu_fieldn_index(x,y,3)]  = omtauinv*ft3  + twsr*(omusq + cidot3u*(1.0+0.5*cidot3u));
    cidot3u = -tuy;
    f2[gpu_fieldn_index(x,y,4)]  = omtauinv*ft4  + twsr*(omusq + cidot3u*(1.0+0.5*cidot3u));
    
    cidot3u = tux+tuy;
    f2[gpu_fieldn_index(x,y,5)]  = omtauinv*ft5  + twdr*(omusq + cidot3u*(1.0+0.5*cidot3u));
    cidot3u = tuy-tux;
    f2[gpu_fieldn_index(x,y,6)]  = omtauinv*ft6  + twdr*(omusq + cidot3u*(1.0+0.5*cidot3u));
    cidot3u = -(tux+tuy);
    f2[gpu_fieldn_index(x,y,7)]  = omtauinv*ft7  + twdr*(omusq + cidot3u*(1.0+0.5*cidot3u));
    cidot3u = tux-tuy;
    f2[gpu_fieldn_index(x,y,8)]  = omtauinv*ft8  + twdr*(omusq + cidot3u*(1.0+0.5*cidot3u));
}
```

The CUDA kernel is a literal one-thread-per-cell mirror of the CPU loop body
— same arithmetic, same direction-loading order, same hard-coded D2Q9
weights.

#### A.6.c Equilibrium-distribution computation (the canonical form)

Both implementations use the closed-form factored equilibrium documented in
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
the three-line comment block (`chapter13/cpu/LBM.cpp:145–148`, verbatim):

```cpp
// feq_i  = w_i rho [1 + 3(ci . u) + (9/2) (ci . u)^2 - (3/2) (u.u)]
// feq_i  = w_i rho [1 - 3/2 (u.u) + (ci . 3u) + (1/2) (ci . 3u)^2]
// feq_i  = w_i rho [1 - 3/2 (u.u) + (ci . 3u){ 1 + (1/2) (ci . 3u) }]
```

The fully-expanded, non-factored equilibrium also exists in the educational
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
"cpu_intro" version (`chapter13/cpu_intro/main.cpp:271–298`):

```cpp
void collide(double *f, double *r, double *u, double *v)
{
    // useful constants
    const double tauinv = 2.0/(6.0*nu+1.0); // 1/tau
    const double omtauinv = 1.0-tauinv;     // 1 - 1/tau

    for(unsigned int y = 0; y < NY; ++y)
    {
        for(unsigned int x = 0; x < NX; ++x)
        {
            double rho = r[scalar_index(x,y)];
            double ux  = u[scalar_index(x,y)];
            double uy  = v[scalar_index(x,y)];
            
            for(unsigned int i = 0; i < ndir; ++i)
            {
                // calculate dot product
                double cidotu = dirx[i]*ux + diry[i]*uy;
                
                // calculate equilibrium
                double feq = wi[i]*rho*(1.0 + 3.0*cidotu + 4.5*cidotu*cidotu - 1.5*(ux*ux+uy*uy));
                
                // relax to equilibrium
                f[field_index(x,y,i)]  = omtauinv*f[field_index(x,y,i)] + tauinv*feq;
            }
        }
    }
}
```

This is the equilibrium the brief asks about: `feq = w_i rho (1 + 3(c.u) +
(9/2)(c.u)^2 - (3/2) u.u)` with `4.5 = 9/2` and `1.5 = 3/2` as literal
constants. The coefficients `3.0`, `4.5`, `1.5` correspond to `1/cs^2`,
`1/(2 cs^4)`, `1/(2 cs^2)` with `cs^2 = 1/3` — the standard lattice sound
speed assumption that is the **same in D3Q19** as in D2Q9 (cs^2 = 1/3 is a
property of the velocity-set normalization, not the dimensionality).

### A.7 D2Q9 velocity-set and ω_i weights — verbatim

The canonical declaration of D2Q9 lattice constants is in
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`chapter13/cpu_intro/main.cpp:25–35` (one of the cleaner placements; the
chapter13/cpu and chapter13/gpu variants declare `w0/ws/wd` only as
separate scalars without the indexed-array view):

```cpp
const unsigned int ndir = 9;
const size_t mem_size_ndir   = sizeof(double)*NX*NY*ndir;
const size_t mem_size_scalar = sizeof(double)*NX*NY;

const double w0 = 4.0/9.0;  // zero weight
const double ws = 1.0/9.0;  // adjacent weight
const double wd = 1.0/36.0; // diagonal weight
const double wi[] = {w0,ws,ws,ws,ws,wd,wd,wd,wd};
const int dirx[] = {0,1,0,-1, 0,1,-1,-1, 1};
const int diry[] = {0,0,1, 0,-1,1, 1,-1,-1};
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
A second canonical declaration appears in `chapter8/cylinder.cpp:59–62`,
verbatim:

```cpp
double weights[]={4.0/9.0,1.0/9.0,1.0/9.0,1.0/9.0,1.0/9.0,1.0/36.0,1.0/36.0,1.0/36.0,1.0/36.0};
double weights_trt[]={0.0,1.0/3.0,1.0/3.0,1.0/3.0,1.0/3.0,1.0/12.0,1.0/12.0,1.0/12.0,1.0/12.0};
int cx[]={0,1,0,-1,0,1,-1,-1,1};
int cy[]={0,0,1,0,-1,1,1,-1,-1};
```

(Note `weights_trt` is the TRT split — not the BGK weights — used for
TRT-collision multiphase models. The standard BGK weights are `weights[]`.)

A third canonical declaration appears in `chapter5/poiseuille_BB.m:31–35`
(MATLAB; rest direction last, not first):

```matlab
% Lattice parameters; note zero direction is last
NPOP=9;                                         % number of velocities
w  = [1/9 1/9 1/9 1/9 1/36 1/36 1/36 1/36 4/9]; % weights
cx = [1 0 -1  0 1 -1 -1  1 0];                  % velocities, x components
cy = [0 1  0 -1 1  1 -1 -1 0];                  % velocities, y components
```

**Direction-ordering differs between chapters:**

- chapter13/cpu_intro and chapter8/cylinder.cpp put the **rest direction first** (`w0 = 4/9` at index 0; `ws = 1/9` for indices 1–4; `wd = 1/36` for indices 5–8).
- chapter5 MATLAB exemplars put the **rest direction last** (`4/9` at index 9 (MATLAB 1-based)).
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- chapter13/cpu and chapter13/gpu store rest direction (`f0`) in a separate scalar array (NOT in the `f1`/`f2` arrays which only hold the eight non-rest populations; see `fieldn_index(x,y,d) = (ndir-1)*(NX*y+x)+(d-1)` at `chapter13/cpu/LBM.h:65–68`).

The direction numbering for chapter13/cpu's `fX` array is documented as the
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
ASCII diagram inside the kernel (`chapter13/cpu/LBM.cpp:113–115`):

```
// direction numbering scheme
// 6 2 5
// 3 0 1
// 7 4 8
```

i.e., 1=E, 2=N, 3=W, 4=S, 5=NE, 6=NW, 7=SW, 8=SE; rest is 0 and stored
separately.

### A.8 Halfway bounce-back boundary — verbatim

The canonical halfway-bounce-back pedagogical implementation is in
`chapter5/poiseuille_BB.m:123–132` (MATLAB; **2D, walls top-and-bottom**):

```matlab
% Boundary condition (bounce-back)
% Top wall (rest)
fprop(:,NY,4)=f(:,NY,2);
fprop(:,NY,7)=f(:,NY,5);
fprop(:,NY,8)=f(:,NY,6);

% Bottom wall (rest)
fprop(:,1,2)=f(:,1,4);
fprop(:,1,5)=f(:,1,7);
fprop(:,1,6)=f(:,1,8);
```

i.e., after streaming, the three populations that would have streamed
INTO the wall (north-going at top: 2, 5, 6; south-going at bottom: 4, 7, 8)
are reflected back. The indexing convention here is the MATLAB cy ordering
where 2=N, 4=S, 5,6=NE+NW, 7,8=SW+SE (per chapter5's `cx`/`cy` declaration
in § A.7). This is the canonical halfway-BB pattern: `fprop(boundary, dir_back) = f(boundary, dir_forward)` for each pair.

The chapter8 C++ files use a related but distinct pattern (`anti`-bounce-back
for concentration / scalar fields, not solid-wall halfway BB for momentum).
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The pattern at `chapter8/cylinder.cpp:222`:

```cpp
f2[bb_nodes[counter]*NPOP+dir]=-f2[counter2*NPOP+complement[dir]]+2*weights[dir]*conc_wall;
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
with `complement` indexed at `chapter8/cylinder.cpp:63`:

```cpp
int complement[]={0,3,4,1,2,7,8,5,6};
```

i.e., complement maps each direction to its reverse (1↔3, 2↔4, 5↔7, 6↔8).
This is anti-bounce-back for Dirichlet boundaries on a scalar (concentration)
field — useful as the **structural** template for "iterate over solid-side
boundary nodes and flip the right population", but NOT the canonical
no-slip halfway BB for momentum.

### A.9 Caveats — is this a suitable Cat-3 integrity anchor?

Pedagogical-clarity simplifications and pitfalls relevant to integrity-anchor
use:

1. **D2Q9 only.** The single most important caveat. The Krüger companion
   code repository ships no D3Q19 reference. Using it as an integrity anchor
   for a 3D LBM simulation requires (a) the human to extend the D2Q9
   formulas to D3Q19 lattice constants and equilibrium themselves, or
   (b) using a different anchor candidate (waLBerla / Palabos / OpenLB —
   see Phase 12 LBM probe Section E for license + verification gaps).

2. **Periodic boundary conditions baked into the streaming step.**
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
   `chapter13/cpu/LBM.cpp:107–110`:
   ```cpp
   unsigned int xp1 = (x+1)%NX;
   unsigned int yp1 = (y+1)%NY;
   unsigned int xm1 = (NX+x-1)%NX;
   unsigned int ym1 = (NY+y-1)%NY;
   ```
   Modular arithmetic for the four neighbour indices is hard-coded; the
   chapter13 kernel does not split out boundary-cell handling. An obstacle-
   bearing LBM (chapter8 `cylinder.cpp` is the closer template) handles the
   boundary in a separate post-stream pass with a `bb_nodes` index list.

3. **The chapter13 storage layout is split: rest population in `f0` (scalar
   array), non-rest in `f1`/`f2` with 8-deep packing.** This is an
   optimization decision (avoids carrying the rest direction through ping-
   pong); GPU-Sims may or may not adopt it. The `fieldn_index(x,y,d) =
   (ndir-1)*(NX*y+x)+(d-1)` indexing assumes `d ∈ [1,8]` for the f1/f2
   arrays. If Phase 12 packs all 19 (D3Q19) populations together with the
   rest, the addressing pattern will not match Krüger byte-for-byte.

4. **Compressible vs. incompressible equilibrium.** chapter13 uses the
   compressible Maxwell-Boltzmann expansion `feq = w_i rho (1 + 3(c.u) +
   (9/2)(c.u)^2 - (3/2) u.u)`. chapter5/poiseuille_BB.m uses the
   **incompressible linearized** form (`chapter5/poiseuille_BB.m:93`):
   ```matlab
   feq(:,:,k)=w(k)*(rho + 3*(u*cx(k)+v*cy(k)));
   ```
   Different sims in the book use different equilibrium models. The Phase
   12 anchor needs to pick which — Krüger has both, byte-exact match is
   only meaningful within a chosen variant.

5. **Direction-numbering inconsistency between chapters.** chapter13/cpu's
   ASCII-diagram order ≠ chapter13/cpu_intro's `dirx/diry` order ≠ chapter5's
   MATLAB ordering ≠ chapter8/cylinder.cpp's. The arithmetic is the same
   under permutation but a literal-byte cat-3 integrity test would have to
   pin to one specific chapter's order. None of these orders is "canonical";
   they were independently authored.

6. **`tau = 1.0` default + Taylor-Green test case.** All chapter13 variants
   are tuned to a Taylor-Green vortex decay benchmark with `nu = 1/6`,
   `tau = 1.0`. This is a stress-test for spectral accuracy, not a fluid-
   geometry-around-obstacle problem. Phase 12 LBM around an airfoil is a
   different validation case.

7. **No CMake / build system.** Each chapter directory is loose `.cpp` or
   `.m` files with no shared makefile (chapter13/cpu_intro has a
   `compile.sh`). Vendoring would not bring a buildable artifact, only a
   reference reading.

Taken together: **Krüger book code is best treated as a 2D pedagogical
*reference for the math* (equilibrium form, weights, halfway-BB pattern),
NOT as a literal byte-for-byte Cat-3 integrity anchor for a 3D LBM sim.**
The integrity-toolkit Cat-3 numerical-correctness tests would have to pin
to constants extracted from the book, not to a specific chapter file's
implementation, since the chapters disagree with each other on direction
ordering.

---

## B — vulkaninfo on RX 6800 XT

**BLOCKED.** `which vulkaninfo` returns exit code 1; the binary is not on
the path. `apt list --installed | grep -i vulkan` shows:

```
libvulkan-dev/noble,now 1.3.275.0-1build1 amd64 [installed]
libvulkan1/noble,now 1.3.275.0-1build1 amd64 [installed,automatic]
mesa-vulkan-drivers/noble-updates,now 25.2.8-0ubuntu0.24.04.1 amd64 [installed,automatic]
vulkan-validationlayers/noble,now 1.3.275.0-1 amd64 [installed]
```

— Vulkan runtime + headers + validation layers are present, but
**`vulkan-tools` (the package that ships `vulkaninfo`) is NOT installed
on this host.**

To resolve, run on this machine:

```
sudo apt-get install vulkan-tools
```

After installation, the brief's three commands can be re-run:

```
vulkaninfo --summary 2>&1 | head -40
vulkaninfo 2>&1 | grep -A 5 "VkPhysicalDeviceSubgroupSizeControlProperties"
vulkaninfo 2>&1 | grep -B 1 -A 8 "subgroupSizeControl"
```

I will not guess values. The four data points the brief asks for:

1. Device name and driver version — **unknown until vulkan-tools installed**.
2. `subgroupSizeControl` feature: VK_TRUE or VK_FALSE — **unknown**.
3. `minSubgroupSize`, `maxSubgroupSize`, `requiredSubgroupSizeStages` —
   **unknown**.
4. `computeFullSubgroups` feature: VK_TRUE or VK_FALSE — **unknown**.

Side note: the existing `common-cpp` consumer of these properties
(`Context::subgroupSizeMin()` / `subgroupSizeMax()` / `requiredSubgroupSizeStages()` /
`subgroupSizeControlEnabled()`) DOES query them at runtime — so a hello-world
binary built from this repo could also produce these values, if running it
in this session were acceptable. The probe brief said read-only, so no binary
runs were attempted.

---

## C — Cross-check: Krüger ω_i weights vs. canonical literature

The D2Q9 weights extracted from Krüger:

```
w_0  = 4/9    (rest)
w_1..w_4 = 1/9  (face neighbours: ±x, ±y)         [4 of them]
w_5..w_8 = 1/36 (diagonal neighbours: ±x±y)        [4 of them]
```

### C.1 Numerical sanity checks

- **Sum to unity:** `4/9 + 4·(1/9) + 4·(1/36) = 16/36 + 16/36 + 4/36 = 36/36 = 1.0`. ✓
- **Face-neighbour count = 4** (for D2Q9; the 2D analog of the brief's "6
  face neighbours" for D3Q19). ✓
- **Diagonal-neighbour count = 4** (for D2Q9; the 2D analog of the brief's
  "12 edge neighbours" for D3Q19). ✓
- **Second moment of velocity set ≡ cs^2 · I:**
  Σ_i w_i c_ix^2 = w_1·1 + w_3·1 + w_5·1 + w_6·1 + w_7·1 + w_8·1
                 = 2·(1/9) + 4·(1/36) = 2/9 + 1/9 = 3/9 = 1/3. ✓
  Confirms `cs^2 = 1/3` as expected for the D2Q9 lattice.

### C.2 For Phase 12's D3Q19 — what the brief actually wants verified

Krüger book code does NOT contain D3Q19 weights, so this side cannot be
extracted from the cloned tree. The standard literature D3Q19 weights are:

```
w_rest    = 1/3   (rest)                             [1 of them]
w_face    = 1/18  (6 face neighbours: ±x, ±y, ±z)    [6 of them]
w_edge    = 1/36  (12 edge neighbours: ±x±y, ±y±z, ±x±z) [12 of them]
```

Sanity check (independent of Krüger):

- Sum: `1/3 + 6·(1/18) + 12·(1/36) = 12/36 + 12/36 + 12/36 = 36/36 = 1.0`. ✓
- The brief's expected partition is `1/3 [rest], 1/18 [6 face neighbours],
  1/36 [12 edge neighbours]`. The sum checks out.

I did NOT find these D3Q19 weights anywhere in the Krüger clone (already
confirmed in § A.5). I also did NOT find LBM weights anywhere in
`references/SPlisHSPlasH/` — `grep -rn "1.0/18.0\|1.0/36.0" references/SPlisHSPlasH/`
was not run as part of this probe; the SPlisHSPlasH upstream is SPH-focused,
not LBM, so it's a low-probability source. If a second offline source is
needed to cross-check the D3Q19 weights, it'll have to come from outside
this probe.

The numerical sanity check (weights sum to 1, partition into 1/3 + 6·1/18 +
12·1/36) is independent of any source and confirms the brief's stated
partition.

---

## Summary

**Is Krüger's code a suitable v1 cat-3 integrity anchor?** Only partially.
The clone is MIT-licensed (compatible), buildable in principle but
build-system-bare (no CMake), and provides clean pedagogical-readability
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
references for D2Q9 BGK collision (`chapter13/cpu/LBM.cpp:97`,
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`chapter13/gpu/LBM.cu:172`), the equilibrium expansion (`chapter13/cpu_intro/main.cpp:271–298`,
showing the `4.5 = 9/2` / `1.5 = 3/2` literal-constant form), and halfway
bounce-back (`chapter5/poiseuille_BB.m:123`). But **the repository ships no
D3Q19 implementation** — every implementation is 2D — and chapters
disagree with each other on direction ordering. As a literal Cat-3 byte-
for-byte anchor for a D3Q19 sim it does not fit; as a *math reference*
for the equilibrium / weights / BC patterns it does. **Pinned SHA candidate:**
`6e2c592fdc3592c14dfd52f860fc1ceea930bcb0`. **Surprises:** (a) D2Q9-only
nature of the repo (the book itself discusses D3Q19 in print, but the code
companion has no 3D implementation); (b) direction-numbering inconsistency
across chapters; (c) the chapter13 kernels split rest-direction into a
separate scalar array (`f0` separate from `f1/f2`), an optimization choice
that complicates literal-match testing. **Section B blocked** — `vulkan-tools`
is not installed on this host; the four subgroup-size-control data points
remain unknown until `sudo apt-get install vulkan-tools` is run. No design
recommendations issued. Probe complete.
