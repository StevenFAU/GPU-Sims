import { mat4, vec3 } from 'gl-matrix';

import type { JsonObject } from './types.js';
import type { InputSnapshot } from './input.js';

export type CameraMode = 'free-fly' | 'arcball' | 'orbit';

/** Re-exported for convenience; identical to InputSnapshot. */
export type CameraInputState = InputSnapshot;

const PITCH_LIMIT_DEG = 89.0;

export class Camera {
    mode: CameraMode = 'free-fly';

    // Free-fly state
    private _position: vec3 = vec3.fromValues(0, 0, 5);
    yawDeg = -90.0;       // looking down -Z
    pitchDeg = 0.0;

    /**
     * Camera position in world space. Read returns the underlying vec3
     * (in-place writes via gl-matrix functions are fine). Write accepts any
     * array-like of length ≥3 (vec3, number[], readonly tuple) and copies
     * into the underlying vec3 — preserves identity for code that holds
     * references to the live vec3.
     */
    get position(): vec3 { return this._position; }
    set position(v: ArrayLike<number>) {
        vec3.set(this._position, Number(v[0]), Number(v[1]), Number(v[2]));
    }

    // Arcball / Orbit state
    target: vec3 = vec3.fromValues(0, 0, 0);
    orbitDistance = 5.0;
    orbitYaw = 0.0;
    orbitPitch = 20.0;
    orbitSpeedDegPerSec = 30.0;

    // Lens
    fovDeg = 60.0;
    aspect = 16.0 / 9.0;
    near = 0.01;
    far = 1000.0;

    // Tuning
    moveSpeed = 5.0;     // units/s
    lookSpeed = 0.2;     // deg/pixel
    boostMultiplier = 5.0;

    update(dt: number, input: CameraInputState): void {
        switch (this.mode) {
            case 'free-fly': this.updateFreeFly(dt, input); break;
            case 'arcball':  this.updateArcball(input);     break;
            case 'orbit':    this.updateOrbit(dt);          break;
        }
    }

    private updateFreeFly(dt: number, input: CameraInputState): void {
        if (input.mouseRight) {
            this.yawDeg += input.mouseDx * this.lookSpeed;
            this.pitchDeg -= input.mouseDy * this.lookSpeed;
            this.pitchDeg = clamp(this.pitchDeg, -PITCH_LIMIT_DEG, PITCH_LIMIT_DEG);
        }
        const speed = this.moveSpeed * (input.shiftHeld ? this.boostMultiplier : 1.0);
        const fwd = this.forward();
        const rt  = this.right();

        const vel = vec3.fromValues(0, 0, 0);
        if (input.keyW) vec3.add(vel, vel, fwd);
        if (input.keyS) vec3.sub(vel, vel, fwd);
        if (input.keyD) vec3.add(vel, vel, rt);
        if (input.keyA) vec3.sub(vel, vel, rt);
        if (input.keyE) vec3.add(vel, vel, vec3.fromValues(0, 1, 0));
        if (input.keyQ) vec3.sub(vel, vel, vec3.fromValues(0, 1, 0));

        const len = vec3.length(vel);
        if (len > 0) {
            vec3.scale(vel, vel, (speed * dt) / len);
            vec3.add(this.position, this.position, vel);
        }
    }

    private updateArcball(input: CameraInputState): void {
        if (input.mouseLeft) {
            this.orbitYaw += input.mouseDx * this.lookSpeed;
            this.orbitPitch -= input.mouseDy * this.lookSpeed;
            this.orbitPitch = clamp(this.orbitPitch, -PITCH_LIMIT_DEG, PITCH_LIMIT_DEG);
        }
        if (input.scrollDy !== 0) {
            this.orbitDistance *= Math.pow(1.1, -input.scrollDy);
            this.orbitDistance = Math.max(this.orbitDistance, 0.001);
        }
        this.recomputeOrbitPosition();
    }

    private updateOrbit(dt: number): void {
        this.orbitYaw += this.orbitSpeedDegPerSec * dt;
        if (this.orbitYaw > 360.0) this.orbitYaw -= 360.0;
        this.recomputeOrbitPosition();
    }

    private recomputeOrbitPosition(): void {
        const yaw   = degToRad(this.orbitYaw);
        const pitch = degToRad(this.orbitPitch);
        const r = this.orbitDistance;
        const dx = r * Math.cos(pitch) * Math.cos(yaw);
        const dy = r * Math.sin(pitch);
        const dz = r * Math.cos(pitch) * Math.sin(yaw);
        vec3.set(this.position,
            this.target[0] + dx,
            this.target[1] + dy,
            this.target[2] + dz);
    }

    forward(): vec3 {
        const out = vec3.create();
        if (this.mode === 'free-fly') {
            const yaw = degToRad(this.yawDeg);
            const pitch = degToRad(this.pitchDeg);
            vec3.set(out,
                Math.cos(pitch) * Math.cos(yaw),
                Math.sin(pitch),
                Math.cos(pitch) * Math.sin(yaw));
            return vec3.normalize(out, out);
        }
        vec3.sub(out, this.target, this.position);
        return vec3.normalize(out, out);
    }

    right(): vec3 {
        const out = vec3.create();
        vec3.cross(out, this.forward(), vec3.fromValues(0, 1, 0));
        return vec3.normalize(out, out);
    }

    up(): vec3 {
        const out = vec3.create();
        vec3.cross(out, this.right(), this.forward());
        return vec3.normalize(out, out);
    }

    /** Column-major view matrix. Mirrors gpusims::Camera::view(). */
    view(): mat4 {
        const out = mat4.create();
        if (this.mode === 'free-fly') {
            const center = vec3.create();
            vec3.add(center, this.position, this.forward());
            return mat4.lookAt(out, this.position, center, vec3.fromValues(0, 1, 0));
        }
        return mat4.lookAt(out, this.position, this.target, vec3.fromValues(0, 1, 0));
    }

    /**
     * Column-major perspective matrix in WebGPU clip space ([0,1] depth, Y up
     * in NDC because of the y-flip). Identical to gpusims::Camera::projection
     * — Vulkan and WebGPU share clip-space convention.
     */
    projection(): mat4 {
        const out = mat4.create();
        mat4.perspectiveZO(out, degToRad(this.fovDeg), this.aspect, this.near, this.far);
        // Y-flip for Vulkan/WebGPU clip-space (origin top-left).
        out[5] *= -1;
        return out;
    }

    viewProjection(): mat4 {
        const out = mat4.create();
        mat4.multiply(out, this.projection(), this.view());
        return out;
    }

    setOrientation(yawDeg: number, pitchDeg: number): void {
        this.yawDeg = yawDeg;
        this.pitchDeg = clamp(pitchDeg, -PITCH_LIMIT_DEG, PITCH_LIMIT_DEG);
    }

    /**
     * Aim the camera at a world-space target. In free-fly mode this updates
     * yaw/pitch from the position→target direction so the camera looks at
     * (x,y,z); in arcball/orbit modes this also updates the orbit target.
     */
    lookAt(x: number, y: number, z: number): void {
        vec3.set(this.target, x, y, z);
        const dir = vec3.create();
        vec3.sub(dir, this.target, this._position);
        const len = vec3.length(dir);
        if (len < 1e-8) return;       // degenerate — leave orientation alone
        vec3.scale(dir, dir, 1 / len);
        // dir = (cos(p)*cos(y), sin(p), cos(p)*sin(y))
        const dy = clamp(dir[1], -1, 1);
        this.pitchDeg = (Math.asin(dy) * 180) / Math.PI;
        this.yawDeg = (Math.atan2(dir[2], dir[0]) * 180) / Math.PI;
        this.pitchDeg = clamp(this.pitchDeg, -PITCH_LIMIT_DEG, PITCH_LIMIT_DEG);
    }

    resetArcball(): void {
        this.orbitYaw = 0.0;
        this.orbitPitch = 20.0;
    }

    /** JSON serialization compatible with gpusims::Camera::toJson. */
    toJson(): JsonObject {
        const modeNum: Record<CameraMode, number> = { 'free-fly': 0, 'arcball': 1, 'orbit': 2 };
        return {
            mode: modeNum[this.mode],
            position: [this.position[0], this.position[1], this.position[2]],
            yaw_deg: this.yawDeg,
            pitch_deg: this.pitchDeg,
            target: [this.target[0], this.target[1], this.target[2]],
            orbit: {
                distance: this.orbitDistance,
                yaw: this.orbitYaw,
                pitch: this.orbitPitch,
                speed: this.orbitSpeedDegPerSec,
            },
            lens: {
                fov_deg: this.fovDeg,
                aspect: this.aspect,
                near: this.near,
                far: this.far,
            },
            freefly: {
                move_speed: this.moveSpeed,
                look_speed: this.lookSpeed,
                boost_mul: this.boostMultiplier,
            },
        };
    }

    fromJson(j: JsonObject): void {
        const modeNumToStr: Record<number, CameraMode> = {
            0: 'free-fly', 1: 'arcball', 2: 'orbit',
        };
        if (typeof j['mode'] === 'number') {
            this.mode = modeNumToStr[j['mode']] ?? 'free-fly';
        }
        const pos = j['position'];
        if (Array.isArray(pos) && pos.length === 3) {
            vec3.set(this.position, Number(pos[0]), Number(pos[1]), Number(pos[2]));
        }
        if (typeof j['yaw_deg']   === 'number') this.yawDeg   = j['yaw_deg'];
        if (typeof j['pitch_deg'] === 'number') this.pitchDeg = j['pitch_deg'];
        const tgt = j['target'];
        if (Array.isArray(tgt) && tgt.length === 3) {
            vec3.set(this.target, Number(tgt[0]), Number(tgt[1]), Number(tgt[2]));
        }
        const o = j['orbit'];
        if (o && typeof o === 'object' && !Array.isArray(o)) {
            if (typeof o['distance'] === 'number') this.orbitDistance       = o['distance'];
            if (typeof o['yaw']      === 'number') this.orbitYaw            = o['yaw'];
            if (typeof o['pitch']    === 'number') this.orbitPitch          = o['pitch'];
            if (typeof o['speed']    === 'number') this.orbitSpeedDegPerSec = o['speed'];
        }
        const l = j['lens'];
        if (l && typeof l === 'object' && !Array.isArray(l)) {
            if (typeof l['fov_deg'] === 'number') this.fovDeg = l['fov_deg'];
            if (typeof l['aspect']  === 'number') this.aspect = l['aspect'];
            if (typeof l['near']    === 'number') this.near   = l['near'];
            if (typeof l['far']     === 'number') this.far    = l['far'];
        }
        const f = j['freefly'];
        if (f && typeof f === 'object' && !Array.isArray(f)) {
            if (typeof f['move_speed'] === 'number') this.moveSpeed       = f['move_speed'];
            if (typeof f['look_speed'] === 'number') this.lookSpeed       = f['look_speed'];
            if (typeof f['boost_mul']  === 'number') this.boostMultiplier = f['boost_mul'];
        }
    }
}

function degToRad(deg: number): number { return (deg * Math.PI) / 180; }
function clamp(x: number, a: number, b: number): number { return x < a ? a : x > b ? b : x; }
