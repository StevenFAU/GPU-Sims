#!/usr/bin/env python3
"""Blender Cycles volume scatter render for Lenia 3D VDB sequences.

Reads a VDB sequence from `continuous-ca/lenia-fft/python/vdb_export/density/`
(output of `gpusims_common.vdb_writer.write_float_frame` per-Nth-frame in
the 3D tier), loads into a Blender Volume domain, sets up a Principled
Volume shader with color-ramp transfer function + scatter coefficient +
mild emission, renders to PNG (still) or MP4 (animation).

Mirrors render_smoke.py (Phase 8) almost exactly — the only differences are:
(a) no temperature grid (Lenia is single-channel scalar density), so
    emission is driven by the density grid through a ColorRamp instead;
(b) default emission-strength is lower (Lenia's volumetric look should be
    creature-on-dark-background, not glowing-fire);
(c) default density-scale is higher (Lenia density values are in [0, 1]
    while smoke values may be much higher; we need to boost to compensate).

Run headless:

    blender -b -P render_lenia_3d.py -- \\
        --density-input <path>/density_0000.vdb --output hero.png [...]

GPU device selection: OptiX -> HIP -> CUDA -> fail. Same template as
render_smoke.py / render_mpm.py.
"""

import argparse
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
        description="Render lenia-fft 3D VDB sequence via Blender Cycles volume scatter.")
    p.add_argument("--density-input", required=True,
                   help="Path to density_NNNN.vdb. For animation, use the lowest-numbered file; "
                        "the script will look up the sequence pattern from the filename.")
    p.add_argument("--output", required=True,
                   help="Output PNG path (still) or base MP4 path (animation).")
    p.add_argument("--resolution", nargs=2, type=int, default=[1920, 1080],
                   metavar=("WIDTH", "HEIGHT"))
    p.add_argument("--samples", type=int, default=512)
    p.add_argument("--camera-pos", nargs=3, type=float, default=[-1.5, 1.0, -1.5],
                   metavar=("X", "Y", "Z"))
    p.add_argument("--camera-target", nargs=3, type=float, default=[0.5, 0.5, 0.5],
                   metavar=("X", "Y", "Z"))
    p.add_argument("--fov-deg", type=float, default=50.0)
    p.add_argument("--frame-start", type=int, default=1)
    p.add_argument("--frame-end", type=int, default=1)
    p.add_argument("--device", default="auto",
                   choices=["auto", "optix", "hip", "cuda", "cpu"])
    p.add_argument("--density-scale", type=float, default=10.0,
                   help="Multiplier on the Principled Volume density input. "
                        "Lenia values are in [0,1]; boost for visible scatter.")
    p.add_argument("--emission-strength", type=float, default=0.5,
                   help="Multiplier on emission from density-driven color-ramp.")
    return p.parse_args(argv)


def select_gpu_device(preferred: str) -> str:
    """Mirrors render_smoke.py exactly — same fallback chain (OptiX → HIP → CUDA → fail)."""
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
        sys.exit(1)
    if preferred == "cpu":
        return "CPU"
    chosen = preferred.upper()
    if not has_devices(chosen):
        print(f"ERROR: requested device {chosen} not available.")
        sys.exit(1)
    return chosen


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def setup_world():
    """Dark world background; volumetric scatter shows up best against dark."""
    world = bpy.data.worlds.new("World")
    bpy.context.scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    bg.inputs["Color"].default_value = (0.02, 0.02, 0.03, 1.0)
    bg.inputs["Strength"].default_value = 0.5


def setup_camera(args):
    """Camera with FOV-deg parameter (matches Phase 8 convention)."""
    bpy.ops.object.camera_add(location=args.camera_pos)
    cam = bpy.context.object
    cam.data.angle = args.fov_deg * 3.14159265 / 180.0
    # Aim at target via track-to constraint with an empty.
    bpy.ops.object.empty_add(location=args.camera_target)
    target = bpy.context.object
    tc = cam.constraints.new("TRACK_TO")
    tc.target = target
    tc.track_axis = "TRACK_NEGATIVE_Z"
    tc.up_axis = "UP_Y"
    bpy.context.scene.camera = cam


def setup_lighting():
    """Two-light key+fill setup matching render_smoke.py."""
    bpy.ops.object.light_add(type="AREA", location=(-3, 3, -3))
    key = bpy.context.object
    key.data.energy = 200
    key.data.size = 3.0
    bpy.ops.object.light_add(type="AREA", location=(2, 2, 2))
    fill = bpy.context.object
    fill.data.energy = 60
    fill.data.size = 5.0


def build_volume_material(args):
    """Principled Volume with density-driven ColorRamp emission. Single-grid
    (density only) — distinct from render_smoke.py's optional temperature grid.

    Architect-2: the color ramp's emission ramp here is a placeholder
    (white-to-blue gradient on density value). User may want to tune
    this per-render via CLI flag in v1.1.
    """
    mat = bpy.data.materials.new("LeniaVolume")
    mat.use_nodes = True
    nt = mat.node_tree
    for node in list(nt.nodes):
        nt.nodes.remove(node)

    out = nt.nodes.new("ShaderNodeOutputMaterial")
    out.location = (400, 0)
    pv = nt.nodes.new("ShaderNodeVolumePrincipled")
    pv.location = (200, 0)
    attr = nt.nodes.new("ShaderNodeAttribute")
    attr.location = (-400, 0)
    attr.attribute_name = "density"
    # Color ramp on density for the emission channel.
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.location = (-200, 100)
    # Default white-to-blue ramp; users tweak in v1.1.
    ramp.color_ramp.elements[0].color = (0.05, 0.1, 0.2, 1.0)
    ramp.color_ramp.elements[1].color = (0.9, 0.95, 1.0, 1.0)

    # Density scale via a Math node.
    scale = nt.nodes.new("ShaderNodeMath")
    scale.location = (-200, -100)
    scale.operation = "MULTIPLY"
    scale.inputs[1].default_value = args.density_scale

    # Wire: attribute (density) → scale → Principled Volume density;
    #       attribute → ramp → Principled Volume emission color.
    nt.links.new(attr.outputs["Fac"], ramp.inputs["Fac"])
    nt.links.new(attr.outputs["Fac"], scale.inputs[0])
    nt.links.new(scale.outputs["Value"], pv.inputs["Density"])
    nt.links.new(ramp.outputs["Color"], pv.inputs["Emission Color"])
    pv.inputs["Emission Strength"].default_value = args.emission_strength
    pv.inputs["Anisotropy"].default_value = 0.3
    nt.links.new(pv.outputs["Volume"], out.inputs["Volume"])
    return mat


def import_vdb_sequence(args, mat):
    """Import VDB density sequence as a Volume object. Phase 8 template."""
    seq_path = Path(args.density_input)
    bpy.ops.object.volume_import(
        filepath=str(seq_path),
        directory=str(seq_path.parent),
        files=[{"name": seq_path.name}],
    )
    vol_obj = bpy.context.object
    vol_obj.data.materials.append(mat)
    # Configure for sequence playback (frame-by-frame VDB load).
    vol_obj.data.is_sequence = True
    vol_obj.data.frame_start = args.frame_start
    vol_obj.data.frame_duration = args.frame_end - args.frame_start + 1
    return vol_obj


def main():
    args = parse_args()
    print(f"render_lenia_3d.py: density={args.density_input} output={args.output}")
    gpu_device = select_gpu_device(args.device)
    reset_scene()
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "GPU" if gpu_device != "CPU" else "CPU"
    scene.cycles.samples = args.samples
    scene.render.resolution_x = args.resolution[0]
    scene.render.resolution_y = args.resolution[1]
    scene.frame_start = args.frame_start
    scene.frame_end = args.frame_end
    scene.render.filepath = args.output

    setup_world()
    setup_camera(args)
    setup_lighting()
    mat = build_volume_material(args)
    import_vdb_sequence(args, mat)

    is_animation = args.frame_end > args.frame_start
    if is_animation:
        scene.render.image_settings.file_format = "FFMPEG"
        scene.render.ffmpeg.format = "MPEG4"
        scene.render.ffmpeg.codec = "H264"
    else:
        scene.render.image_settings.file_format = "PNG"

    bpy.ops.render.render(animation=is_animation, write_still=not is_animation)
    print(f"done: wrote {args.output}")


if __name__ == "__main__":
    main()
