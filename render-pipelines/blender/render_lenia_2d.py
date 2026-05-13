#!/usr/bin/env python3
"""Blender Cycles compositor render for Lenia 2D PNG sequences.

Reads a PNG sequence from `continuous-ca/lenia-fft/python/frames_export/`
(output of `ti.tools.imwrite` per-Nth-frame in the sim), applies a Cycles
compositor pipeline (RGB Curves color-grade + saturation boost + vignette
via Mask multiply + optional Blur Time temporal smoothing), outputs MP4.

NO 3D geometry. The render is purely compositor-driven on an Image Sequence
input node. This is the Phase 10 first Cycles-compositor-without-3D-geometry
script in the repo.

Run headless:

    blender -b -P render_lenia_2d.py -- \\
        --frames-input <path>/frame_0000.png --output hero.mp4 [--frame-end N] [...]

GPU device selection: OptiX -> HIP -> CUDA -> fail. Same template as
render_smoke.py / render_mpm.py. CPU rendering is NOT silently selected;
exits 1 if no GPU is detected (passing `--device cpu` is an explicit opt-in).

Mirrors render_smoke.py / render_mpm.py shape: parse_args, select_gpu_device,
reset_scene, setup_compositor, build_image_sequence_node, main.
"""

import argparse
import os
import sys
from pathlib import Path

import bpy


def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []
    p = argparse.ArgumentParser(
        description="Render lenia-fft 2D PNG sequence via Blender Cycles compositor.")
    p.add_argument("--frames-input", required=True,
                   help="Path to frame_NNNN.png. Use the lowest-numbered file; "
                        "the script looks up the sequence pattern from the filename.")
    p.add_argument("--output", required=True,
                   help="Output MP4 path.")
    p.add_argument("--resolution", nargs=2, type=int, default=[1920, 1080],
                   metavar=("WIDTH", "HEIGHT"))
    p.add_argument("--frame-start", type=int, default=1)
    p.add_argument("--frame-end", type=int, default=300)
    p.add_argument("--device", default="auto",
                   choices=["auto", "optix", "hip", "cuda", "cpu"])
    p.add_argument("--saturation", type=float, default=1.3,
                   help="Saturation boost factor (1.0 = neutral).")
    p.add_argument("--vignette", type=float, default=0.4,
                   help="Vignette strength (0.0 = none, 1.0 = strong).")
    p.add_argument("--temporal-blur", type=int, default=0,
                   help="Frames of temporal blur (0 = none, 3 = mild motion smoothing).")
    return p.parse_args(argv)


def select_gpu_device(preferred: str) -> str:
    """Select a GPU backend, printing which was chosen. Mirrors render_smoke.py
    Phase 8 template. Exits with error code 1 if no GPU is available and
    `preferred != "cpu"`."""
    prefs = bpy.context.preferences
    cprefs = prefs.addons["cycles"].preferences

    def has_devices(dtype):
        try:
            cprefs.compute_device_type = dtype
            cprefs.refresh_devices()
            return any(d.type == dtype for d in cprefs.devices)
        except Exception as ex:
            print(f"  [gpu-probe] {dtype}: not available ({ex})")
            return False

    if preferred == "auto":
        for backend in ("OPTIX", "HIP", "CUDA"):
            if has_devices(backend):
                print(f"  [gpu] selected {backend}")
                return backend
        print("ERROR: no GPU backend available (OptiX/HIP/CUDA all absent).")
        print("Pass --device cpu to render on CPU (very slow).")
        sys.exit(1)
    if preferred == "cpu":
        return "CPU"
    chosen = preferred.upper()
    if not has_devices(chosen):
        print(f"ERROR: requested device {chosen} not available.")
        sys.exit(1)
    return chosen


def reset_scene():
    """Clear the default scene and start fresh."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    for collection in list(bpy.data.collections):
        bpy.data.collections.remove(collection)


def setup_compositor(args, gpu_device: str):
    """Build the Cycles compositor pipeline: Image Sequence → RGB Curves →
    Hue Saturation Value → Vignette (Mask × Multiply) → optional Blur Time
    → Output File."""
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "GPU" if gpu_device != "CPU" else "CPU"
    scene.render.resolution_x = args.resolution[0]
    scene.render.resolution_y = args.resolution[1]
    scene.frame_start = args.frame_start
    scene.frame_end = args.frame_end
    scene.render.image_settings.file_format = "FFMPEG"
    scene.render.ffmpeg.format = "MPEG4"
    scene.render.ffmpeg.codec = "H264"
    scene.render.filepath = args.output
    scene.use_nodes = True

    nt = scene.node_tree
    for node in list(nt.nodes):
        nt.nodes.remove(node)

    # Image Sequence input node.
    img_node = nt.nodes.new("CompositorNodeImage")
    seq_path = Path(args.frames_input)
    img_node.image = bpy.data.images.load(str(seq_path))
    img_node.image.source = "SEQUENCE"
    img_node.frame_duration = args.frame_end - args.frame_start + 1
    img_node.frame_start = args.frame_start
    img_node.frame_offset = 0
    img_node.location = (-800, 0)

    # RGB Curves for color grade.
    rgb_node = nt.nodes.new("CompositorNodeCurveRGB")
    rgb_node.location = (-600, 0)
    # Default identity curve; users may extend in v1.1 with config files.

    # Hue Saturation Value for saturation boost.
    hsv_node = nt.nodes.new("CompositorNodeHueSat")
    hsv_node.location = (-400, 0)
    hsv_node.inputs["Saturation"].default_value = args.saturation

    # Vignette: Ellipse mask × Multiply with main image.
    mask_node = nt.nodes.new("CompositorNodeEllipseMask")
    mask_node.location = (-400, -200)
    mask_node.width = 0.5
    mask_node.height = 0.5
    mask_node.mask_type = "MULTIPLY"

    # Invert the mask so it's dark in corners.
    invert_node = nt.nodes.new("CompositorNodeInvert")
    invert_node.location = (-200, -200)
    invert_node.inputs["Fac"].default_value = args.vignette

    # Mix the vignette over the image.
    mix_node = nt.nodes.new("CompositorNodeMixRGB")
    mix_node.location = (0, 0)
    mix_node.blend_type = "MULTIPLY"
    mix_node.inputs["Fac"].default_value = 1.0

    # Optional temporal blur.
    if args.temporal_blur > 0:
        blur_node = nt.nodes.new("CompositorNodeBlur")
        blur_node.location = (200, 0)
        blur_node.size_x = args.temporal_blur
        blur_node.size_y = args.temporal_blur

    # Output File node — write the final MP4.
    out_node = nt.nodes.new("CompositorNodeComposite")
    out_node.location = (400, 0)

    # Wire the graph.
    nt.links.new(img_node.outputs["Image"], rgb_node.inputs["Image"])
    nt.links.new(rgb_node.outputs["Image"], hsv_node.inputs["Image"])
    nt.links.new(hsv_node.outputs["Image"], mix_node.inputs[1])
    nt.links.new(mask_node.outputs["Mask"], invert_node.inputs["Color"])
    nt.links.new(invert_node.outputs["Color"], mix_node.inputs[2])

    if args.temporal_blur > 0:
        nt.links.new(mix_node.outputs["Image"], blur_node.inputs["Image"])
        nt.links.new(blur_node.outputs["Image"], out_node.inputs["Image"])
    else:
        nt.links.new(mix_node.outputs["Image"], out_node.inputs["Image"])


def main():
    args = parse_args()
    print(f"render_lenia_2d.py: frames={args.frames_input} output={args.output}")
    gpu_device = select_gpu_device(args.device)
    reset_scene()
    setup_compositor(args, gpu_device)
    # Render the full sequence to MP4.
    bpy.ops.render.render(animation=True)
    print(f"done: wrote {args.output}")


if __name__ == "__main__":
    main()
