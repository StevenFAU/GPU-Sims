# SPH Water — Specification

> **Status:** Specification pending — not yet drafted by the architect chat
> **Category:** Particle fluids
> **Primary stack:** C (Native C++)
> **Secondary stack(s):** —
> **Target machine:** Desktop interactive, A100 hero
> **Folder:** [`particle-fluids/sph-water`](../../particle-fluids/sph-water/)

---

## 1. Goal and audience

What does this sim demonstrate? Who is the imagined viewer? What feeling should it produce?

## 2. Mathematical formulation

The actual equations being solved, with citations to canonical papers. Approximations explicitly listed and justified.

## 3. Stack assignment and rationale

Which of A / B / C / D, and why this one over the alternatives. Rejected alternatives noted.

## 4. Data structures and memory layout

The buffers, textures, atlas formats, alignment requirements. Estimated VRAM consumption at each scale tier. (90% of GPU performance lives here.)

## 5. Per-frame compute pipeline

Sequence of GPU dispatches. Synchronization points. Read/write hazards. For complex pipelines, a diagram.

## 6. Interactive rendering approach

What's drawn each frame, with what shader pipeline. Camera type, UI elements, sliders, mouse interactions, hotkeys.

## 7. Offline export path

How state is captured to VDB or Alembic. The offline render pipeline (Blender? Houdini? OptiX standalone?). Targeted hero renders.

## 8. Scale tiers

- **Laptop iteration scale:** what runs at 60fps on integrated graphics or low-end discrete.
- **Desktop flagship scale:** what runs at 60fps on the RX 6800 XT (or 2080 Ti).
- **HPC hero scale:** what's possible on an A100 batch run, even if not real-time.

## 9. Stretch goals

Optional improvements that would push performance, quality, or interactivity further. Effort estimates.

## 10. Engineering risks

What's likely to be hard or slow. Where bugs typically hide. What needs profiling.

## 11. References

Canonical papers, reference implementations, prior-art demos. License check on any reference code consulted.
