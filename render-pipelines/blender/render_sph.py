"""
render_sph.py — Blender Cycles hero render for sph-water (Phase 11).

Consumes an Alembic .abc particle archive written by particle-fluids/sph-water
during interactive use. Instances each particle as a small icosphere via
Geometry Nodes, applies a Principled BSDF water material, renders via Cycles
with GPU device fallback chain OptiX -> HIP -> CUDA -> fail-loud-no-CPU.

Structurally mirrors render-pipelines/blender/render_mpm.py (Phase 9), with two
material differences:
  - PLY sequence load -> bpy.ops.wm.alembic_import
  - PLY-stored velocity reconstruction -> native Alembic OPoints velocity attr

CLI:
  blender --background --python render_sph.py -- \
      --abc-path /path/to/sph_water.abc \
      --output /path/to/output.png \
      --samples 256 \
      --resolution 1920 1080 \
      [--frame N] \
      [--frame-start S --frame-end E]
"""

from __future__ import annotations

import argparse
import math
import os
import sys
from typing import Optional, Tuple

import bpy  # type: ignore  # provided by Blender's embedded Python


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    """Parse args from the post-`--` slice."""
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser(description="Render an sph-water Alembic archive via Blender Cycles.")
    parser.add_argument("--abc-path", required=True, help="Path to the .abc particle archive.")
    parser.add_argument("--output",   required=True, help="Path to the output image (or .mp4 for sequences).")
    parser.add_argument("--samples",  type=int,   default=256,  help="Cycles render samples.")
    parser.add_argument("--resolution", type=int, nargs=2, default=[1920, 1080],
                        help="Render resolution (width height).")
    parser.add_argument("--frame",       type=int, default=None, help="Single frame to render.")
    parser.add_argument("--frame-start", type=int, default=None, help="Animation start frame.")
    parser.add_argument("--frame-end",   type=int, default=None, help="Animation end frame.")
    parser.add_argument("--particle-radius", type=float, default=0.01,
                        help="Particle radius (m), used as instance icosphere radius scale.")
    parser.add_argument("--tint", type=float, nargs=3, default=[0.10, 0.30, 0.50],
                        help="Water Base Color RGB (0..1).")
    parser.add_argument("--roughness", type=float, default=0.0, help="Water surface roughness.")
    return parser.parse_args(argv)


# ---------------------------------------------------------------------------
# GPU fallback chain (OptiX -> HIP -> CUDA -> fail-loud-no-CPU)
# Pattern mirrors render_mpm.py / render_smoke.py exactly.
# ---------------------------------------------------------------------------

def configure_cycles_gpu() -> str:
    """Configure Cycles for GPU rendering. Tries OptiX, then HIP, then CUDA.
    Raises RuntimeError if none of the three is available (we do NOT fall
    back to CPU; loud failure is the convention)."""
    prefs = bpy.context.preferences
    cycles_prefs = prefs.addons["cycles"].preferences

    candidates = ["OPTIX", "HIP", "CUDA"]
    chosen: Optional[str] = None
    for backend in candidates:
        try:
            cycles_prefs.compute_device_type = backend
        except TypeError:
            continue
        cycles_prefs.get_devices()
        gpu_devices = [d for d in cycles_prefs.devices if d.type == backend]
        if gpu_devices:
            chosen = backend
            for d in cycles_prefs.devices:
                d.use = (d.type == backend)
            break

    if chosen is None:
        raise RuntimeError(
            "No GPU compute device available (tried OPTIX, HIP, CUDA). "
            "render_sph.py refuses to fall back to CPU; install GPU drivers."
        )

    bpy.context.scene.cycles.device = "GPU"
    return chosen


# ---------------------------------------------------------------------------
# Scene setup
# ---------------------------------------------------------------------------

def setup_scene(samples: int, resolution: Tuple[int, int]) -> None:
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False

    scene.cycles.samples = samples
    scene.cycles.use_denoising = True

    # Motion blur via Alembic per-point velocity attribute.
    scene.render.use_motion_blur = True
    scene.cycles.motion_blur_position = "CENTER"
    scene.render.motion_blur_shutter = 0.5

    # Image output format.
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "16"


def setup_lighting() -> None:
    """Three-point lighting + HDRI environment. Identical structure to render_mpm.py."""
    # Sun key light.
    bpy.ops.object.light_add(type="SUN", location=(5, 5, 8))
    sun = bpy.context.active_object
    sun.data.energy = 5.0
    sun.data.angle = math.radians(2.0)
    sun.rotation_euler = (math.radians(50), math.radians(20), math.radians(30))

    # Soft fill area light.
    bpy.ops.object.light_add(type="AREA", location=(-4, -2, 4))
    fill = bpy.context.active_object
    fill.data.energy = 200.0
    fill.data.size = 4.0

    # World gradient — simple two-stop blue sky.
    world = bpy.context.scene.world
    world.use_nodes = True
    bg_node = world.node_tree.nodes.get("Background")
    if bg_node is not None:
        bg_node.inputs["Color"].default_value = (0.7, 0.85, 1.0, 1.0)
        bg_node.inputs["Strength"].default_value = 0.6


def setup_camera() -> None:
    bpy.ops.object.camera_add(location=(0, -4.5, 1.5))
    cam = bpy.context.active_object
    cam.rotation_euler = (math.radians(80), 0.0, 0.0)
    cam.data.lens = 35.0
    bpy.context.scene.camera = cam


# ---------------------------------------------------------------------------
# Alembic import + Geometry Nodes instancing
# ---------------------------------------------------------------------------

def import_alembic_particles(abc_path: str) -> bpy.types.Object:
    """Import the Alembic archive and return the imported particle object.

    Alembic OPoints schema (which is what gpusims::abc::ParticleWriter writes)
    surfaces as a Blender point cloud / mesh-with-vertices object. The per-point
    velocity attribute named "velocities" is preserved and read by Cycles when
    motion blur is enabled (no manual finite-difference reconstruction needed).
    """
    if not os.path.isfile(abc_path):
        raise FileNotFoundError(f"Alembic archive not found: {abc_path}")

    existing = set(bpy.data.objects.keys())
    bpy.ops.wm.alembic_import(
        filepath=abc_path,
        scale=1.0,
        set_frame_range=True,
        is_sequence=False,
        validate_meshes=False,
        always_add_cache_reader=True,
    )
    new_objs = set(bpy.data.objects.keys()) - existing
    if not new_objs:
        raise RuntimeError(f"Alembic import produced no new objects from {abc_path}")
    return bpy.data.objects[next(iter(new_objs))]


def attach_instancer(particle_obj: bpy.types.Object, particle_radius: float) -> None:
    """Geometry Nodes graph: instance an icosphere at each input point.

    Each point in the Alembic archive becomes one icosphere of radius
    1.5 * particle_radius. The "velocities" attribute is carried through
    automatically by Cycles motion blur."""
    gn_mod = particle_obj.modifiers.new(name="ParticleInstancer", type="NODES")
    group = bpy.data.node_groups.new(name="SPHParticles", type="GeometryNodeTree")
    gn_mod.node_group = group

    # Interface (Blender 4.x style).
    if hasattr(group, "interface"):
        group.interface.new_socket("Geometry", in_out="INPUT",  socket_type="NodeSocketGeometry")
        group.interface.new_socket("Geometry", in_out="OUTPUT", socket_type="NodeSocketGeometry")
    else:
        group.inputs.new("NodeSocketGeometry",  "Geometry")
        group.outputs.new("NodeSocketGeometry", "Geometry")

    nodes = group.nodes
    links = group.links
    node_in  = nodes.new(type="NodeGroupInput")
    node_out = nodes.new(type="NodeGroupOutput")
    node_in.location  = (-600, 0)
    node_out.location = ( 600, 0)

    icosphere = nodes.new(type="GeometryNodeMeshIcoSphere")
    icosphere.inputs["Radius"].default_value      = 1.5 * particle_radius
    icosphere.inputs["Subdivisions"].default_value = 1
    icosphere.location = (-300, -200)

    instance = nodes.new(type="GeometryNodeInstanceOnPoints")
    instance.location = (0, 0)

    realize = nodes.new(type="GeometryNodeRealizeInstances")
    realize.location = (300, 0)

    links.new(node_in.outputs["Geometry"],     instance.inputs["Points"])
    links.new(icosphere.outputs["Mesh"],       instance.inputs["Instance"])
    links.new(instance.outputs["Instances"],   realize.inputs["Geometry"])
    links.new(realize.outputs["Geometry"],     node_out.inputs["Geometry"])


def setup_water_material(obj: bpy.types.Object,
                         tint_rgb: Tuple[float, float, float],
                         roughness: float) -> bpy.types.Material:
    mat = bpy.data.materials.new(name=f"WaterMat_{obj.name}")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*tint_rgb, 1.0)
    bsdf.inputs["Roughness"].default_value  = roughness
    bsdf.inputs["IOR"].default_value        = 1.33

    # Transmission socket name changed between Blender 3.x and 4.0+.
    if "Transmission Weight" in bsdf.inputs:
        bsdf.inputs["Transmission Weight"].default_value = 1.0
    elif "Transmission" in bsdf.inputs:
        bsdf.inputs["Transmission"].default_value = 1.0

    if "Alpha" in bsdf.inputs:
        bsdf.inputs["Alpha"].default_value = 0.9

    if obj.data is not None:
        if hasattr(obj.data, "materials"):
            if obj.data.materials:
                obj.data.materials[0] = mat
            else:
                obj.data.materials.append(mat)
    return mat


# ---------------------------------------------------------------------------
# Frame range + render dispatch
# ---------------------------------------------------------------------------

def configure_frames(args: argparse.Namespace) -> None:
    scene = bpy.context.scene
    if args.frame is not None:
        scene.frame_start = args.frame
        scene.frame_end   = args.frame
        scene.frame_current = args.frame
    elif args.frame_start is not None and args.frame_end is not None:
        scene.frame_start = args.frame_start
        scene.frame_end   = args.frame_end
        scene.frame_current = args.frame_start
    # else: leave whatever Alembic import's set_frame_range produced.


def render_single(output_path: str) -> None:
    bpy.context.scene.render.filepath = output_path
    bpy.ops.render.render(write_still=True)


def render_animation(output_dir: str) -> None:
    bpy.context.scene.render.filepath = output_dir
    bpy.ops.render.render(animation=True)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    args = parse_args()
    print(f"[render_sph] abc-path: {args.abc_path}")
    print(f"[render_sph] output:    {args.output}")

    # Reset scene to a known clean state.
    bpy.ops.wm.read_factory_settings(use_empty=True)

    backend = configure_cycles_gpu()
    print(f"[render_sph] Cycles GPU backend: {backend}")

    setup_scene(samples=args.samples, resolution=tuple(args.resolution))
    setup_lighting()
    setup_camera()

    particle_obj = import_alembic_particles(args.abc_path)
    print(f"[render_sph] Imported particle object: {particle_obj.name}")

    attach_instancer(particle_obj, args.particle_radius)
    setup_water_material(particle_obj, tuple(args.tint), args.roughness)

    configure_frames(args)

    is_anim = (args.frame is None and
               args.frame_start is not None and
               args.frame_end is not None)
    if is_anim:
        render_animation(args.output)
    else:
        render_single(args.output)

    print("[render_sph] Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
