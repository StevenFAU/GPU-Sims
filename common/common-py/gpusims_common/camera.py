"""Camera — Stack D wrapper around ti.ui.Camera.

Public surface mirrors `gpusims::Camera` (common-cpp/include/gpusims/camera.hpp):
modes (FreeFly / Arcball / Orbit), per-frame `update(dt, input)` (for non-FreeFly
modes; FreeFly delegates to ti.ui.Camera.track_user_inputs), view / projection /
view_projection getters, lens parameters, and to_json / from_json for state capture.

Implementation wraps `ti.ui.Camera` rather than porting Stack C's manual yaw/pitch
math — ti.ui.Camera already implements WASDQE + RMB-look free-fly cleanly.
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from enum import Enum
from typing import Any, Final

import numpy as np
import taichi as ti

from gpusims_common.log import log


class CameraMode(Enum):
    """Camera control modes. Mirrors gpusims::Camera::Mode (common-cpp camera.hpp)."""

    FreeFly = "FreeFly"  # FPS-like; WASDQE + RMB-drag look (delegates to ti.ui.Camera)
    Arcball = "Arcball"  # Orbit around target with LMB drag, zoom with scroll
    Orbit = "Orbit"      # Animated orbit around target (no input needed)


@dataclass
# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
class CameraInputState:
    """Per-frame raw input snapshot.

    Mirrors gpusims::CameraInputState (common-cpp camera.hpp). For FreeFly mode
    this struct is unused — ti.ui.Camera.track_user_inputs reads input directly
    from the window. For Arcball / Orbit modes the caller populates this from
    `window.is_pressed(...)`, `window.get_cursor_pos()`, etc.
    """

# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
    key_w: bool = False
# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
    key_a: bool = False
# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
    key_s: bool = False
# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
    key_d: bool = False
# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
    key_q: bool = False  # down (world space)
# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
    key_e: bool = False  # up (world space)
# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
    shift_held: bool = False  # boost / fine-control toggle

    mouse_left: bool = False
# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
    mouse_right: bool = False
# integrity-allow: cat2.public-symbol-used; pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused); n/a
    mouse_middle: bool = False

    # Mouse delta in normalized [0,1] units since last update (ti.ui convention).
    mouse_dx: float = 0.0
    mouse_dy: float = 0.0

    # Scroll wheel delta accumulated over the frame.
    scroll_dy: float = 0.0


_DEFAULT_MOVE_SPEED: Final[float] = 0.03   # ti.ui.Camera units; matches mpm3d_ggui upstream
_DEFAULT_FOV_DEG: Final[float] = 55.0
_DEFAULT_NEAR: Final[float] = 0.01
_DEFAULT_FAR: Final[float] = 1000.0


class Camera:
    """Stack D Camera. Wraps ti.ui.Camera; exposes Stack C-conceptual surface.

    Usage:

        cam = Camera(mode=CameraMode.FreeFly)
        cam.set_position(0.5, 1.0, 1.95)
        cam.set_lookat(0.5, 0.3, 0.5)
        cam.set_fov_deg(55)

        while window.running:
            # FreeFly: built-in input handling
            cam.track_user_inputs(window)
            # Other modes: caller-driven update
            # cam.update(dt, input_snapshot)
            scene.set_camera(cam.ti_camera())
            # ... draw ...

    `view()` / `projection()` / `view_projection()` return 4x4 numpy float32 matrices
    computed from (position, lookat, up) via right-handed-Y-up convention matching
    ti.ui.Camera. **No clip-space Y-flip is applied** (Taichi's render path handles
    this internally; doing it again here would reproduce the common-web Phase 7
    Y-flip bug).
    """

    def __init__(
        self,
        mode: CameraMode = CameraMode.FreeFly,
        fov_deg: float = _DEFAULT_FOV_DEG,
        aspect: float = 16.0 / 9.0,
        near: float = _DEFAULT_NEAR,
        far: float = _DEFAULT_FAR,
        move_speed: float = _DEFAULT_MOVE_SPEED,
    ) -> None:
        self._mode: CameraMode = mode
        self._ti_camera: Any = ti.ui.Camera()

        # Position state
        self._position: list[float] = [0.0, 0.0, 5.0]
        self._lookat: list[float] = [0.0, 0.0, 0.0]
        self._up: list[float] = [0.0, 1.0, 0.0]

        # Lens
        self._fov_deg: float = fov_deg
        self._aspect: float = aspect
        self._near: float = near
        self._far: float = far

        # Free-fly tuning
        self._move_speed: float = move_speed

        # Arcball / Orbit state
        self._target: list[float] = [0.0, 0.0, 0.0]
        self._orbit_distance: float = 5.0
        self._orbit_yaw: float = 0.0
        self._orbit_pitch: float = 20.0
        self._orbit_speed: float = 30.0  # deg/s for Orbit mode

        # Sync initial state to the wrapped ti.ui.Camera.
        self._sync_to_ti_camera()

    # ------------------------------------------------------------------
    # Mode + per-frame update
    # ------------------------------------------------------------------

    def mode(self) -> CameraMode:
        return self._mode

    def set_mode(self, mode: CameraMode) -> None:
        self._mode = mode

    def track_user_inputs(self, window: Any) -> None:
        """FreeFly: hand control to ti.ui.Camera.track_user_inputs.

        ti.ui.Camera.track_user_inputs handles WASDQE + RMB-drag-look directly
        from the window's input state. After the call, the ti.ui.Camera's
        internal position / lookat / up have been updated; we mirror them back
        into our Python-side state.
        """
        if self._mode != CameraMode.FreeFly:
            log.warn(
                "Camera.track_user_inputs called with mode=%s; FreeFly-only. "
                "Use Camera.update(dt, input) for Arcball / Orbit.",
                self._mode.value,
            )
            return
        self._ti_camera.track_user_inputs(
            window,
            movement_speed=self._move_speed,
            hold_key=ti.ui.RMB,
        )
        # Pull updated state back to Python side for inspection / serialization.
        self._position = list(self._ti_camera.curr_position)
        self._lookat = list(self._ti_camera.curr_lookat)
        self._up = list(self._ti_camera.curr_up)

    def update(self, dt: float, input_state: CameraInputState) -> None:
        """Arcball / Orbit per-frame update. No-op for FreeFly."""
        if self._mode == CameraMode.FreeFly:
            return

        if self._mode == CameraMode.Orbit:
            self._orbit_yaw += self._orbit_speed * dt

        if self._mode == CameraMode.Arcball:
            if input_state.mouse_left:
                self._orbit_yaw -= input_state.mouse_dx * 360.0
                self._orbit_pitch = max(-89.0, min(89.0, self._orbit_pitch - input_state.mouse_dy * 180.0))
            if input_state.scroll_dy != 0.0:
                self._orbit_distance = max(0.1, self._orbit_distance * (1.0 - 0.1 * input_state.scroll_dy))

        yaw_r = math.radians(self._orbit_yaw)
        pitch_r = math.radians(self._orbit_pitch)
        cp = math.cos(pitch_r)
        self._position = [
            self._target[0] + self._orbit_distance * cp * math.cos(yaw_r),
            self._target[1] + self._orbit_distance * math.sin(pitch_r),
            self._target[2] + self._orbit_distance * cp * math.sin(yaw_r),
        ]
        self._lookat = list(self._target)
        self._sync_to_ti_camera()

    # ------------------------------------------------------------------
    # Output transforms (numpy 4x4 float32; right-handed Y-up)
    # ------------------------------------------------------------------

    def view(self) -> np.ndarray:
        eye = np.array(self._position, dtype=np.float32)
        center = np.array(self._lookat, dtype=np.float32)
        up = np.array(self._up, dtype=np.float32)
        f = center - eye
        f /= np.linalg.norm(f) + 1e-12
        s = np.cross(f, up)
        s /= np.linalg.norm(s) + 1e-12
        u = np.cross(s, f)
        m = np.eye(4, dtype=np.float32)
        m[0, 0:3] = s
        m[1, 0:3] = u
        m[2, 0:3] = -f
        m[0, 3] = -float(np.dot(s, eye))
        m[1, 3] = -float(np.dot(u, eye))
        m[2, 3] = float(np.dot(f, eye))
        return m

    def projection(self) -> np.ndarray:
        f = 1.0 / math.tan(math.radians(self._fov_deg) * 0.5)
        nf = 1.0 / (self._near - self._far)
        m = np.zeros((4, 4), dtype=np.float32)
        m[0, 0] = f / self._aspect
        m[1, 1] = f
        m[2, 2] = (self._near + self._far) * nf
        m[2, 3] = 2.0 * self._near * self._far * nf
        m[3, 2] = -1.0
        return m

    def view_projection(self) -> np.ndarray:
        result: np.ndarray = self.projection() @ self.view()
        return result

    # ------------------------------------------------------------------
    # Position / orientation accessors
    # ------------------------------------------------------------------

    def position(self) -> tuple[float, float, float]:
        return (self._position[0], self._position[1], self._position[2])

    def lookat(self) -> tuple[float, float, float]:
        return (self._lookat[0], self._lookat[1], self._lookat[2])

    def up(self) -> tuple[float, float, float]:
        return (self._up[0], self._up[1], self._up[2])

    def forward(self) -> tuple[float, float, float]:
        v = np.array(self._lookat, dtype=np.float32) - np.array(self._position, dtype=np.float32)
        n = float(np.linalg.norm(v))
        if n < 1e-12:
            return (0.0, 0.0, -1.0)
        v = v / n
        return (float(v[0]), float(v[1]), float(v[2]))

    def right(self) -> tuple[float, float, float]:
        f = np.array(self.forward(), dtype=np.float32)
        u = np.array(self._up, dtype=np.float32)
        r = np.cross(f, u)
        n = float(np.linalg.norm(r))
        if n < 1e-12:
            return (1.0, 0.0, 0.0)
        r = r / n
        return (float(r[0]), float(r[1]), float(r[2]))

    def ti_camera(self) -> Any:
        """Return the wrapped ti.ui.Camera for scene.set_camera(...)."""
        return self._ti_camera

    # ------------------------------------------------------------------
    # Setters (also re-sync to ti.ui.Camera)
    # ------------------------------------------------------------------

    def set_position(self, x: float, y: float, z: float) -> None:
        self._position = [x, y, z]
        self._sync_to_ti_camera()

    def set_lookat(self, x: float, y: float, z: float) -> None:
        self._lookat = [x, y, z]
        self._sync_to_ti_camera()

    def set_up(self, x: float, y: float, z: float) -> None:
        self._up = [x, y, z]
        self._sync_to_ti_camera()

    def set_fov_deg(self, deg: float) -> None:
        self._fov_deg = deg
        self._sync_to_ti_camera()

    def set_aspect(self, aspect: float) -> None:
        self._aspect = aspect

    def set_near_far(self, near: float, far: float) -> None:
        self._near = near
        self._far = far
        self._sync_to_ti_camera()

    def set_target(self, x: float, y: float, z: float) -> None:
        self._target = [x, y, z]

    def set_orbit_distance(self, d: float) -> None:
        self._orbit_distance = d

    def set_orbit_speed(self, deg_per_sec: float) -> None:
        self._orbit_speed = deg_per_sec

    def set_move_speed(self, speed: float) -> None:
        self._move_speed = speed

    # ------------------------------------------------------------------
    # JSON serialization (for StateWriter / StateReader)
    # ------------------------------------------------------------------

    def to_json(self) -> dict[str, Any]:
        """Serialize to a JSON-compatible dict; schema mirrors Stack C `Camera::toJson` (snake_case)."""
        return {
            "mode": self._mode.value,
            "position": list(self._position),
            "lookat": list(self._lookat),
            "up": list(self._up),
            "fov_deg": self._fov_deg,
            "aspect": self._aspect,
            "near": self._near,
            "far": self._far,
            "target": list(self._target),
            "orbit_distance": self._orbit_distance,
            "orbit_yaw": self._orbit_yaw,
            "orbit_pitch": self._orbit_pitch,
            "orbit_speed": self._orbit_speed,
            "move_speed": self._move_speed,
        }

    def from_json(self, j: dict[str, Any]) -> None:
        """Restore from a JSON dict. Missing keys silently left at current values."""
        if "mode" in j:
            self._mode = CameraMode(j["mode"])
        if "position" in j:
            self._position = list(j["position"])
        if "lookat" in j:
            self._lookat = list(j["lookat"])
        if "up" in j:
            self._up = list(j["up"])
        if "fov_deg" in j:
            self._fov_deg = float(j["fov_deg"])
        if "aspect" in j:
            self._aspect = float(j["aspect"])
        if "near" in j:
            self._near = float(j["near"])
        if "far" in j:
            self._far = float(j["far"])
        if "target" in j:
            self._target = list(j["target"])
        if "orbit_distance" in j:
            self._orbit_distance = float(j["orbit_distance"])
        if "orbit_yaw" in j:
            self._orbit_yaw = float(j["orbit_yaw"])
        if "orbit_pitch" in j:
            self._orbit_pitch = float(j["orbit_pitch"])
        if "orbit_speed" in j:
            self._orbit_speed = float(j["orbit_speed"])
        if "move_speed" in j:
            self._move_speed = float(j["move_speed"])
        self._sync_to_ti_camera()

    # ------------------------------------------------------------------
    # Internals
    # ------------------------------------------------------------------

    def _sync_to_ti_camera(self) -> None:
        """Push Python-side state to the wrapped ti.ui.Camera."""
        self._ti_camera.position(self._position[0], self._position[1], self._position[2])
        self._ti_camera.lookat(self._lookat[0], self._lookat[1], self._lookat[2])
        self._ti_camera.up(self._up[0], self._up[1], self._up[2])
        self._ti_camera.fov(self._fov_deg)
        self._ti_camera.z_near(self._near)
        self._ti_camera.z_far(self._far)
