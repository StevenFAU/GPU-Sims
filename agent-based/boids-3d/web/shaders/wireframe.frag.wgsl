// wireframe.frag.wgsl — fixed gray color for the box wireframe.
// Color matches § 2.7's aquarium specification: vec3(0.35, 0.35, 0.4) —
// visible against the gradient background but not stark.

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(0.35, 0.35, 0.4, 1.0);
}
