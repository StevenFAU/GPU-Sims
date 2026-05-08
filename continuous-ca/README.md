# Continuous cellular automata

Cellular automata with continuous state (rather than discrete cells like Conway's Life). Includes reaction-diffusion systems, Lenia's continuous-state CA, and learned cellular automata implemented as small neural networks per cell.

## Sims in this category

- [`reaction-diffusion-2d/`](reaction-diffusion-2d/) — Gray-Scott pattern explorer with parameter sliders. **Stack A (Shadertoy).**
- [`lenia-fft/`](lenia-fft/) — 2048² real-time Lenia via FFT convolution; automated parameter search for stable creatures. **Stack D (Python / Taichi)** for research; **Stack B (WebGPU)** for deployment.
- [`neural-ca/`](neural-ca/) — Neural CA trained to grow a target image from a seed pixel; interactive damage-and-regenerate demo. **Stack D (Python / PyTorch)** for training; **Stack B (WebGPU)** for deployment.

For category rationale see [`../docs/overarching-spec.md`](../docs/overarching-spec.md) §5–§6.
