// Shared types used across multiple modules.

// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
export type Vec2 = readonly [number, number];
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
export type Vec3 = readonly [number, number, number];
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
export type Vec4 = readonly [number, number, number, number];

// 4x4 matrix in column-major order (matches gl-matrix and WGSL's mat4x4f).
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
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
