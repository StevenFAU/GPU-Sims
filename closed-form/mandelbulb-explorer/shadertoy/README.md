# Stack A — Shadertoy reference

Self-contained single-file fragment shader that paste-runs on
[shadertoy.com/new](https://www.shadertoy.com/new). MIT licensed (the file
header carries the license + author + reference attribution).

This artifact serves two purposes:

1. **Documentation of the port flow.** The Stack B port at `../web/` is a
   real port of this shader: same DE math, same default parameters, same
   look. Having both halves visible in the repo makes the A → B convention
   concrete for future port phases (e.g., `reaction-diffusion-2d`).
2. **Iteration medium.** Shadertoy.com is the fastest place to play with
   DE raymarchers. If the Stack B side ever needs a math change, prototype
   it here first.

## To run

1. Open <https://www.shadertoy.com/new>.
2. Paste the contents of `mandelbulb.glsl` into the Image tab.
3. Save. (No buffers, no common tab, no other channels needed.)

## Controls

- **Hold left mouse and drag:** orbit the camera around the origin.
- **No mouse pressed:** slow auto-orbit at ~23°/s.

## Port mapping (Stack A → Stack B)

| Stack A (this file)                          | Stack B (`../web/shaders/raymarch.frag.wgsl`) |
|----------------------------------------------|-----------------------------------------------|
| `iResolution`, `iTime`, `iMouse`             | Uniform buffer (camera + DE params + coloring) |
| `mainImage(out vec4, in vec2)`               | `@fragment fn fs_main(...) -> @location(0) vec4<f32>` |
| Hardcoded constants at top of file           | Tunable via lil-gui sliders                   |
| Mouse drives camera orbit                    | WASDQE + RMB-drag via common-web `Camera` class |
| Direct write to fragColor                    | Write to offscreen `rgba16float` RT, second pass tonemaps |
| GLSL `pow` / `sin` / `cos` / `length`        | WGSL `pow` / `sin` / `cos` / `length` (identical) |
| GLSL `mix(a, b, t)`                          | WGSL `mix(a, b, t)` (identical)                |
| `mat3 * vec3`                                | WGSL `mat3x3<f32> * vec3<f32>`                 |
| Single point-at-origin orbit trap            | Three trap presets selectable in lil-gui      |

The math is intentionally identical so a screenshot from Shadertoy with
default parameters and a screenshot from the Stack B sim with default
parameters and matching camera position should look the same.

## License of consulted references

No code lifted from any third-party shader. References consulted (per the
strange-attractors README precedent for documenting reference review):

- **Daniel White**, "The Mandelbulb: First 'true' 3D image of the famous
  fractal" (2009 white paper / blog post). Math, no code.
- **Paul Nylander**, <https://www.bugman123.com/Hypercomplex/>. Math and
  early reference renders, no code lifted.
- **Inigo Quilez**, "Distance estimation" essay
  (<https://iquilezles.org/articles/distfunctions/>). General DE-raymarching
  technique discussion, no specific shader code lifted. iq's website
  defaults to CC BY-NC-SA, so any verbatim copy would create a license
  problem; this file is independent.
