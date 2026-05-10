import { mat4, vec4 } from 'gl-matrix';
import type { Camera } from '@gpusims/common-web';

export interface GroundPlaneHit {
    position: [number, number, number];
    hit: boolean;
}

/**
 * Convert canvas-pixel coordinates to a world-space ray from the camera,
 * intersect with the y=0 ground plane, and return the hit world position.
 *
 * Sign convention notes:
 *   - WebGPU framebuffer y-origin is at the top (cy = 0 → top of screen).
 *   - WebGPU clip-space Y points UP: NDC y = +1 is at the top of the
 *     framebuffer, NDC y = -1 is at the bottom (matches OpenGL convention).
 *   - Therefore: yClip = 1 - 2 * (canvasY / canvasHeight) maps canvasY = 0
 *     (top of screen) to yClip = +1 (top of NDC). This is the standard
 *     WebGPU / OpenGL picking formula.
 */
export function unprojectToGroundPlane(
    canvasX: number,
    canvasY: number,
    canvasWidth: number,
    canvasHeight: number,
    camera: Camera,
): GroundPlaneHit {
    const w = Math.max(canvasWidth, 1);
    const h = Math.max(canvasHeight, 1);
    const xClip = (canvasX / w) * 2 - 1;
    const yClip = 1 - (canvasY / h) * 2;

    const vp = camera.viewProjection();
    const invVp = mat4.create();
    mat4.invert(invVp, vp);

    // Two clip-space points on the picking ray (near and far planes).
    const nearClip = vec4.fromValues(xClip, yClip, 0, 1);
    const farClip  = vec4.fromValues(xClip, yClip, 1, 1);
    const nearWorld = vec4.create();
    const farWorld  = vec4.create();
    vec4.transformMat4(nearWorld, nearClip, invVp);
    vec4.transformMat4(farWorld,  farClip,  invVp);

    // Manual perspective divide — gl-matrix does NOT divide by w automatically.
    const nx = nearWorld[0] / nearWorld[3];
    const ny = nearWorld[1] / nearWorld[3];
    const nz = nearWorld[2] / nearWorld[3];
    const fx = farWorld[0]  / farWorld[3];
    const fy = farWorld[1]  / farWorld[3];
    const fz = farWorld[2]  / farWorld[3];

    // Ray direction (not necessarily normalized; t is in parametric space).
    const dx = fx - nx;
    const dy = fy - ny;
    const dz = fz - nz;

    // Ray-plane intersection with y=0: ny + t*dy = 0  =>  t = -ny / dy.
    if (Math.abs(dy) < 1e-6) return { position: [0, 0, 0], hit: false };
    const t = -ny / dy;
    if (t < 0) return { position: [0, 0, 0], hit: false };  // plane is behind camera

    return { position: [nx + t * dx, 0, nz + t * dz], hit: true };
}
