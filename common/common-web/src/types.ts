// Shared types used across multiple modules.

export type Vec2 = readonly [number, number];
export type Vec3 = readonly [number, number, number];
export type Vec4 = readonly [number, number, number, number];

// 4x4 matrix in column-major order (matches gl-matrix and WGSL's mat4x4f).
export type Mat4 = Float32Array;

// JSON-serializable plain object. Used by StateWriter / StateReader.
export type JsonValue =
    | string
    | number
    | boolean
    | null
    | JsonValue[]
    | { [k: string]: JsonValue };

export type JsonObject = { [k: string]: JsonValue };
