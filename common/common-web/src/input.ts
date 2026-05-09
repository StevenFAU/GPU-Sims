// Input snapshot helper. Converts raw browser keyboard/pointer state into
// a single immutable struct that Camera::update consumes — same shape as
// gpusims::CameraInputState in common-cpp.

export interface InputSnapshot {
    keyW: boolean;
    keyA: boolean;
    keyS: boolean;
    keyD: boolean;
    keyQ: boolean;
    keyE: boolean;
    shiftHeld: boolean;

    mouseLeft: boolean;
    mouseRight: boolean;
    mouseMiddle: boolean;

    /** Mouse delta in pixels since the previous snapshot. */
    mouseDx: number;
    mouseDy: number;

    /** Wheel delta accumulated since the previous snapshot. */
    scrollDy: number;
}

const empty: InputSnapshot = {
    keyW: false, keyA: false, keyS: false, keyD: false, keyQ: false, keyE: false,
    shiftHeld: false,
    mouseLeft: false, mouseRight: false, mouseMiddle: false,
    mouseDx: 0, mouseDy: 0, scrollDy: 0,
};

/**
 * Bind to a canvas and yield a function that returns + clears per-frame deltas.
 *
 * Returned snapshot reflects accumulated state since the last call to it; mouse
 * and scroll deltas are reset to zero each call (so subsequent calls return only
 * new movement). Key states are absolute (true while held).
 *
 * Pointer-lock is *not* used by default — it's annoying for casual visitors.
 * Mouse-look semantics: drag with right button to look (matches free-fly mode).
 */
export function snapshotInput(canvas: HTMLCanvasElement): () => InputSnapshot {
    const keys = new Set<string>();
    let mouseLeft = false, mouseRight = false, mouseMiddle = false;
    let dx = 0, dy = 0, scrollDy = 0;
    let lastX = 0, lastY = 0, hasLast = false;

    canvas.addEventListener('keydown', (e) => { keys.add(e.code); });
    canvas.addEventListener('keyup',   (e) => { keys.delete(e.code); });
    // The canvas needs tabIndex to receive keyboard events.
    if (canvas.tabIndex < 0) canvas.tabIndex = 0;

    // Window-level keys catch the case where canvas isn't focused.
    window.addEventListener('keydown', (e) => { keys.add(e.code); });
    window.addEventListener('keyup', (e) => { keys.delete(e.code); });

    canvas.addEventListener('mousedown', (e) => {
        if (e.button === 0) mouseLeft = true;
        if (e.button === 1) mouseMiddle = true;
        if (e.button === 2) mouseRight = true;
        canvas.focus();
    });
    canvas.addEventListener('mouseup', (e) => {
        if (e.button === 0) mouseLeft = false;
        if (e.button === 1) mouseMiddle = false;
        if (e.button === 2) mouseRight = false;
    });
    // Suppress the right-click context menu so RMB drag works.
    canvas.addEventListener('contextmenu', (e) => e.preventDefault());

    canvas.addEventListener('mousemove', (e) => {
        if (!hasLast) {
            lastX = e.clientX;
            lastY = e.clientY;
            hasLast = true;
            return;
        }
        dx += e.clientX - lastX;
        dy += e.clientY - lastY;
        lastX = e.clientX;
        lastY = e.clientY;
    });

    canvas.addEventListener('wheel', (e) => {
        // deltaY is positive for scroll-down on most platforms.
        scrollDy += -e.deltaY / 100.0;
        e.preventDefault();
    }, { passive: false });

    return function readSnapshot(): InputSnapshot {
        const snap: InputSnapshot = {
            keyW: keys.has('KeyW'),
            keyA: keys.has('KeyA'),
            keyS: keys.has('KeyS'),
            keyD: keys.has('KeyD'),
            keyQ: keys.has('KeyQ'),
            keyE: keys.has('KeyE'),
            shiftHeld: keys.has('ShiftLeft') || keys.has('ShiftRight'),
            mouseLeft, mouseRight, mouseMiddle,
            mouseDx: dx, mouseDy: dy, scrollDy,
        };
        dx = 0;
        dy = 0;
        scrollDy = 0;
        return snap;
    };
}

export const emptyInput: InputSnapshot = empty;
