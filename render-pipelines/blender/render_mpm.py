#!/usr/bin/env python3
"""Blender Cycles render script for MPM-multimaterial PLY particle sequences.

Renders a still or animation from PLY particle frames exported by
`hybrid-particle-grid/mpm-multimaterial/python/main.py`. Run headless:

    blender -b -P render_mpm.py -- --ply-input <path> --output <path> [...]

GPU device selection: OptiX -> HIP -> CUDA -> fail. CPU rendering is NOT
silently selected; the script exits with error code 1 if no GPU is detected
(passing `--device cpu` is an explicit opt-in).

Per-material visual treatment via Geometry Nodes per-vertex `material` int
channel: each particle is instanced as a small icosahedron with material slot
driven by the named-attribute. Three Cycles materials are constructed
programmatically via the bpy API (no `.blend` file dependency):

  - WATER (0): Principled BSDF, blue tint, IOR 1.33, transmission 1.0
  - JELLY (1): Principled BSDF, reddish, subsurface scattering enabled
  - SNOW  (2): Principled BSDF, near-white, roughness 0.7, slight emission

v1 deliverable: a single hero still at --frame 60. v1.1 produces an animation
once A100 access is available; the script supports `--frame-start` /
`--frame-end` for both modes from day 1.

Mirrors render_smoke.py (Phase 8) in shape: parse_args, select_gpu_device,
reset_scene, setup_world, setup_camera, setup_lighting, build_material(),
import_ply_sequence, main.
"""

import argparse
import os
import sys
from pathlib import Path

# Blender Python module — only available when running under `blender -P`.
import bpy

# ============================================================================
# CLI args (parsed AFTER the `--` separator Blender uses to delimit script
# args from Blender's own args).
# ============================================================================

def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []
    p = argparse.ArgumentParser(
        description="Render mpm-multimaterial PLY particle sequence via Blender Cycles.")
    p.add_argument("--ply-input", required=True,
                   help="Path to particles_NNNN.ply. For animation, use the lowest-numbered "
                        "file; the script looks up the sequence pattern from the filename.")
    p.add_argument("--output", required=True,
                   help="Output PNG path. For animation, this is the BASE; frame numbers append.")
    p.add_argument("--resolution", nargs=2, type=int, default=[1920, 1080],
                   metavar=("WIDTH", "HEIGHT"))
    p.add_argument("--samples", type=int, default=256,
                   help="Cycles samples per pixel (128-512 typical for particle scenes).")
    p.add_argument("--camera-pos", nargs=3, type=float, default=[1.8, 1.2, 1.8],
                   metavar=("X", "Y", "Z"))
    p.add_argument("--camera-target", nargs=3, type=float, default=[0.5, 0.3, 0.5],
                   metavar=("X", "Y", "Z"))
    p.add_argument("--fov-deg", type=float, default=50.0)
    p.add_argument("--frame", type=int, default=None,
                   help="Single-frame render. If set, overrides --frame-start / --frame-end.")
    p.add_argument("--frame-start", type=int, default=1,
                   help="First frame to render (default: 1 — still mode).")
    p.add_argument("--frame-end", type=int, default=1,
                   help="Last frame to render (default: same as start — still mode).")
    p.add_argument("--device", default="auto",
                   choices=["auto", "optix", "hip", "cuda", "cpu"],
                   help="GPU backend. 'auto' tries OptiX -> HIP -> CUDA and fails if none. "
                        "'cpu' is allowed only with explicit opt-in.")
    p.add_argument("--particle-radius", type=float, default=0.004,
                   help="Per-particle instanced-icosahedron radius in world units.")
    args = p.parse_args(argv)
    # Single-frame convenience: --frame implies start = end = frame
    if args.frame is not None:
        args.frame_start = args.frame
        args.frame_end = args.frame
    return args

# ============================================================================
# GPU device selection (Phase 8 inheritance; same shape as render_smoke.py).
# ============================================================================

def select_gpu_device(preferred: str) -> str:
    """Select a GPU backend, printing which was chosen.

    For 'auto': tries OptiX (NVIDIA) → HIP (AMD) → CUDA (NVIDIA legacy) → fail.
    For 'cpu': allowed only via explicit opt-in; never reached from 'auto'.
    For explicit choices: errors if requested backend has no devices.
    """
    prefs = bpy.context.preferences.addons["cycles"].preferences
    print(f"[render_mpm] requested device: {preferred}")

    def try_backend(name: str, prefs_name: str) -> bool:
        try:
            prefs.compute_device_type = prefs_name
            prefs.refresh_devices()
        except (TypeError, RuntimeError) as exc:
            print(f"[render_mpm] backend {name} unavailable: {exc}")
            return False
        gpu_devs = [d for d in prefs.devices if d.type != "CPU"]
        if not gpu_devs:
            print(f"[render_mpm] backend {name}: no GPU devices found")
            return False
        for d in prefs.devices:
            d.use = (d.type != "CPU")
        print(f"[render_mpm] selected {name}; GPUs: {[d.name for d in gpu_devs]}")
        return True

    if preferred == "cpu":
        prefs.compute_device_type = "NONE"
        bpy.context.scene.cycles.device = "CPU"
        print("[render_mpm] using CPU explicit")
        return "cpu"

    chain = (
        [("OPTIX", "OPTIX"), ("HIP", "HIP"), ("CUDA", "CUDA")]
        if preferred == "auto" else
        [(preferred.upper(), preferred.upper())]
    )
    for name, prefs_name in chain:
        if try_backend(name, prefs_name):
            bpy.context.scene.cycles.device = "GPU"
            return name.lower()

    print("[render_mpm] FATAL: no GPU device available")
    print("[render_mpm] re-run with `--device cpu` to force CPU rendering (slow)")
    sys.exit(1)

# ============================================================================
# Scene reset
# ============================================================================

def reset_scene() -> None:
    """Clear default cube / lamp / camera so we can build from scratch."""
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat, do_unlink=True)
    for mesh in list(bpy.data.meshes):
        bpy.data.meshes.remove(mesh, do_unlink=True)
    for ng in list(bpy.data.node_groups):
        bpy.data.node_groups.remove(ng, do_unlink=True)

# ============================================================================
# World / lighting / camera / ground
# ============================================================================

def setup_world(scene) -> None:
    world = bpy.data.worlds.new("MpmWorld")
    scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    bg.inputs["Color"].default_value = (0.04, 0.05, 0.07, 1.0)
    bg.inputs["Strength"].default_value = 0.3

def setup_camera(scene, pos, target, fov_deg, res_x, res_y):
    cam_data = bpy.data.cameras.new("MpmCam")
    cam_obj = bpy.data.objects.new("MpmCam", cam_data)
    bpy.context.collection.objects.link(cam_obj)
    cam_obj.location = pos
    # Aim camera at target
    direction = (
        target[0] - pos[0],
        target[1] - pos[1],
        target[2] - pos[2],
    )
    import math
    cam_obj.rotation_euler = _look_at_euler(direction)
    cam_data.lens_unit = "FOV"
    cam_data.angle = math.radians(fov_deg)
    scene.camera = cam_obj
    scene.render.resolution_x = res_x
    scene.render.resolution_y = res_y
    return cam_obj

def _look_at_euler(direction):
    """Compute XYZ Euler rotation so -Z axis points along `direction`."""
    import math
    dx, dy, dz = direction
    horizontal_len = math.sqrt(dx * dx + dz * dz) + 1e-12
    yaw = math.atan2(dx, -dz)
    pitch = math.atan2(dy, horizontal_len)
    return (pitch, 0.0, yaw)

def setup_lighting(scene) -> None:
    """3-point lighting tuned for translucent / refractive particle scenes."""
    # Key light: bright area light from upper-front
    key_data = bpy.data.lights.new("KeyLight", type="AREA")
    key_data.size = 2.0
    key_data.energy = 600.0
    key_obj = bpy.data.objects.new("KeyLight", key_data)
    bpy.context.collection.objects.link(key_obj)
    key_obj.location = (1.5, 2.5, 1.5)
    key_obj.rotation_euler = (-0.9, 0.2, 0.6)

    # Fill light: cooler, lower power from opposite side
    fill_data = bpy.data.lights.new("FillLight", type="AREA")
    fill_data.size = 3.0
    fill_data.energy = 150.0
    fill_data.color = (0.7, 0.8, 1.0)
    fill_obj = bpy.data.objects.new("FillLight", fill_data)
    bpy.context.collection.objects.link(fill_obj)
    fill_obj.location = (-1.5, 1.5, 1.5)
    fill_obj.rotation_euler = (-0.7, -0.3, -0.6)

    # Rim light: warm, behind subject for translucent silhouette
    rim_data = bpy.data.lights.new("RimLight", type="SPOT")
    rim_data.energy = 250.0
    rim_data.color = (1.0, 0.85, 0.6)
    rim_data.spot_size = 1.2
    rim_obj = bpy.data.objects.new("RimLight", rim_data)
    bpy.context.collection.objects.link(rim_obj)
    rim_obj.location = (0.5, 1.0, -1.0)
    rim_obj.rotation_euler = (-2.0, 0.0, 0.0)

def setup_ground(scene) -> None:
    """Large flat ground plane with neutral material for shadow catching."""
    bpy.ops.mesh.primitive_plane_add(size=20.0, location=(0.0, -0.001, 0.0))
    plane = bpy.context.active_object
    plane.name = "Ground"
    mat = bpy.data.materials.new("GroundMat")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.18, 0.18, 0.20, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.85
    plane.data.materials.append(mat)

# ============================================================================
# Per-material Cycles materials (constructed via bpy API; no .blend dep)
# ============================================================================

def build_material_water() -> bpy.types.Material:
    """Transparent blue water: Principled BSDF with transmission 1.0."""
    mat = bpy.data.materials.new("Water")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.10, 0.50, 0.85, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.05
    bsdf.inputs["IOR"].default_value = 1.33
    # Blender 4.x: Transmission Weight is the input name; pre-4.0 was "Transmission".
    if "Transmission Weight" in bsdf.inputs:
        bsdf.inputs["Transmission Weight"].default_value = 1.0
    elif "Transmission" in bsdf.inputs:
        bsdf.inputs["Transmission"].default_value = 1.0
    return mat

def build_material_jelly() -> bpy.types.Material:
    """Translucent red-orange jelly via SSS."""
    mat = bpy.data.materials.new("Jelly")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.93, 0.33, 0.23, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.20
    # Subsurface — Blender 4.x renamed inputs from "Subsurface" to "Subsurface Weight".
    if "Subsurface Weight" in bsdf.inputs:
        bsdf.inputs["Subsurface Weight"].default_value = 0.4
    elif "Subsurface" in bsdf.inputs:
        bsdf.inputs["Subsurface"].default_value = 0.4
    if "Subsurface Radius" in bsdf.inputs:
        bsdf.inputs["Subsurface Radius"].default_value = (0.04, 0.02, 0.015)
    return mat

def build_material_snow() -> bpy.types.Material:
    """Near-white snow: rough Lambertian with a hint of emission."""
    mat = bpy.data.materials.new("Snow")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.95, 0.95, 1.00, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.85
    bsdf.inputs["Emission Strength"].default_value = 0.05
    return mat

# ============================================================================
# Geometry Nodes per-material instancing on PLY-imported point cloud
# ============================================================================

def build_particle_geometry_nodes(host_obj, water_mat, jelly_mat, snow_mat,
                                  radius: float) -> None:
    """Attach a Geometry Nodes modifier that instances a small icosahedron per
    vertex, with material driven by the "material" named integer attribute.

    Node graph shape:
        Group Input (geometry from PLY mesh)
        → Mesh to Points (one point per vertex)
        → Instance on Points (icosahedron instance per point)
        → Named Attribute("material", "INT")
        → Compare nodes (== 0, == 1, == 2)
        → Three Set Material nodes
        → Group Output

    Each instance gets the matching Cycles material by branching on the
    attribute via a Switch node tree. The shape is verbose in the bpy API
    but the runtime cost is minimal.
    """
    mod = host_obj.modifiers.new("MpmInstancer", type="NODES")
    ng = bpy.data.node_groups.new("MpmInstancerNG", "GeometryNodeTree")
    mod.node_group = ng

    # Group interface (Blender 4.0+ uses the `interface` API)
    ng.interface.new_socket("Geometry", in_out="INPUT", socket_type="NodeSocketGeometry")
    ng.interface.new_socket("Geometry", in_out="OUTPUT", socket_type="NodeSocketGeometry")

    nodes = ng.nodes
    links = ng.links

    n_in = nodes.new("NodeGroupInput")
    n_in.location = (-1000, 0)
    n_out = nodes.new("NodeGroupOutput")
    n_out.location = (1200, 0)

    # Mesh to Points: convert PLY vertices to a point cloud
    n_m2p = nodes.new("GeometryNodeMeshToPoints")
    n_m2p.location = (-700, 0)
    n_m2p.inputs["Radius"].default_value = radius
    links.new(n_in.outputs["Geometry"], n_m2p.inputs["Mesh"])

    # Read the "material" named attribute (int)
    n_attr = nodes.new("GeometryNodeInputNamedAttribute")
    n_attr.data_type = "INT"
    n_attr.inputs["Name"].default_value = "material"
    n_attr.location = (-700, -300)

    # Instance: small icosahedron per point
    n_icos = nodes.new("GeometryNodeMeshIcoSphere")
    n_icos.location = (-400, -200)
    n_icos.inputs["Radius"].default_value = radius * 2.0
    n_icos.inputs["Subdivisions"].default_value = 1

    n_inst = nodes.new("GeometryNodeInstanceOnPoints")
    n_inst.location = (-100, 0)
    links.new(n_m2p.outputs["Points"], n_inst.inputs["Points"])
    links.new(n_icos.outputs["Mesh"], n_inst.inputs["Instance"])

    # Realize instances so material can be set per-instance
    n_real = nodes.new("GeometryNodeRealizeInstances")
    n_real.location = (200, 0)
    links.new(n_inst.outputs["Instances"], n_real.inputs["Geometry"])

    # Three Set Material nodes branching on attribute equality:
    # (a == 0 → Water), (a == 1 → Jelly), (a == 2 → Snow)
    def add_eq_then_set_material(input_geom_out, threshold_int, material, x_off):
        # Equality test. FunctionNodeCompare in Blender 4.4 has hidden sockets for
        # each data_type; with data_type=INT the enabled NodeSocketInt sockets are
        # at positional indices 2 and 3 (Blender names them "A" at [2] and "B" at
        # [3] respectively — verified against Blender 4.4.3 pre-ship). The EQUAL
        # operation is commutative so it doesn't matter which side carries the
        # constant vs. the linked attribute; if this is ever changed to a
        # non-commutative op (LESS_THAN, GREATER_THAN), the constant should land
        # on the side conceptually appropriate for the comparison direction.
        n_eq = nodes.new("FunctionNodeCompare")
        n_eq.data_type = "INT"
        n_eq.operation = "EQUAL"
        n_eq.location = (x_off, -300)
        n_eq.inputs[2].default_value = threshold_int  # Int A (constant; commutative with B for EQUAL)
        links.new(n_attr.outputs["Attribute"], n_eq.inputs[3])  # Int B (linked from named-attribute output)
        # Set Material (selection-driven)
        n_set = nodes.new("GeometryNodeSetMaterial")
        n_set.location = (x_off, 0)
        n_set.inputs["Material"].default_value = material
        links.new(input_geom_out, n_set.inputs["Geometry"])
        links.new(n_eq.outputs["Result"], n_set.inputs["Selection"])
        return n_set.outputs["Geometry"]

    out0 = add_eq_then_set_material(n_real.outputs["Geometry"], 0, water_mat, 450)
    out1 = add_eq_then_set_material(out0,                       1, jelly_mat, 700)
    out2 = add_eq_then_set_material(out1,                       2, snow_mat,  950)

    links.new(out2, n_out.inputs["Geometry"])

# ============================================================================
# PLY import (single frame or animated sequence)
# ============================================================================

def import_ply_frame(ply_path: str) -> bpy.types.Object:
    """Import a PLY file as a mesh object. Returns the imported object."""
    # Blender 4.x: bpy.ops.wm.ply_import; earlier: bpy.ops.import_mesh.ply
    if hasattr(bpy.ops.wm, "ply_import"):
        bpy.ops.wm.ply_import(filepath=ply_path)
    else:
        bpy.ops.import_mesh.ply(filepath=ply_path)
    obj = bpy.context.active_object
    if obj is None:
        print(f"[render_mpm] FATAL: PLY import did not produce an active object: {ply_path}")
        sys.exit(1)
    obj.name = "MpmParticles"
    return obj

def derive_ply_sequence_path(template_path: str, frame_idx: int) -> str:
    """Given a path like '.../particles_0001.ply', return path for `frame_idx`.

    Pads to 4 digits matching the mpm-multimaterial export convention.
    """
    p = Path(template_path)
    stem = p.stem
    # Find trailing digits in stem
    import re
    m = re.match(r"^(.*?)(\d+)$", stem)
    if not m:
        return str(p)
    prefix, _ = m.group(1), m.group(2)
    return str(p.with_name(f"{prefix}{frame_idx:04d}{p.suffix}"))

# ============================================================================
# Main
# ============================================================================

def main() -> None:
    args = parse_args()
    print(f"[render_mpm] args: {vars(args)}")

    select_gpu_device(args.device)

    reset_scene()
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"

    setup_world(scene)
    setup_camera(scene, args.camera_pos, args.camera_target,
                 args.fov_deg, args.resolution[0], args.resolution[1])
    setup_lighting(scene)
    setup_ground(scene)

    water_mat = build_material_water()
    jelly_mat = build_material_jelly()
    snow_mat  = build_material_snow()

    # Frame loop. v1 default: single still (--frame-start == --frame-end).
    for frame_idx in range(args.frame_start, args.frame_end + 1):
        frame_path = derive_ply_sequence_path(args.ply_input, frame_idx)
        if not os.path.isfile(frame_path):
            print(f"[render_mpm] WARN: frame {frame_idx} PLY missing: {frame_path}; skipping")
            continue

        # Remove the previous frame's particle object so the new PLY replaces it.
        prev = bpy.data.objects.get("MpmParticles")
        if prev is not None:
            bpy.data.objects.remove(prev, do_unlink=True)

        obj = import_ply_frame(frame_path)
        build_particle_geometry_nodes(obj, water_mat, jelly_mat, snow_mat,
                                       args.particle_radius)

        # Output path: still mode → exactly --output; animation → --output stem + _NNNN
        if args.frame_start == args.frame_end:
            out_path = args.output
        else:
            base = Path(args.output)
            out_path = str(base.with_name(f"{base.stem}_{frame_idx:04d}{base.suffix}"))
        scene.render.filepath = out_path

        scene.frame_set(frame_idx)
        print(f"[render_mpm] rendering frame {frame_idx} to {out_path} …")
        bpy.ops.render.render(write_still=True)

    print("[render_mpm] done")


if __name__ == "__main__":
    main()
