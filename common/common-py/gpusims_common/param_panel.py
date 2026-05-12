"""ParamPanel — thin wrapper around Taichi GGUI sub-windows.

Stack D's analog of common-cpp ImGui glue and common-web's lil-gui ParamPanel.
Taichi's `gui.sub_window(...)` is already a context manager exposing
`slider_float / checkbox / button / color_edit_3 / text`; ParamPanel adds:

  - Named-folder organization (matches lil-gui's folder shape).
  - A `refresh_displays()` no-op (lil-gui slider-freeze workaround isn't needed
    on Taichi GGUI; the method is exposed for API parity with common-web).
  - A `persist_key` constructor arg, currently a no-op — Taichi GGUI has no
    built-in localStorage equivalent. v1.1 stretch: read/write to
    ~/.config/gpusims/<sim>.json if the user wants slider persistence.
"""

from __future__ import annotations

from contextlib import contextmanager
from typing import Any, Iterator

from gpusims_common.log import log


class ParamPanel:
    """Per-sim parameter panel; wraps Taichi GGUI sub-windows.

    Usage:

        panel = ParamPanel("MPM", persist_key="mpm-multimaterial")
        while window.running:
            panel.bind(window.get_gui())
            with panel.folder("Gravity", 0.05, 0.05, 0.2, 0.12) as f:
                gx = f.slider_float("x", gx, -10.0, 10.0)
            window.show()
    """

    def __init__(self, title: str, persist_key: str | None = None) -> None:
        self._title: str = title
        self._persist_key: str | None = persist_key
        self._gui: Any = None
        if persist_key is not None:
            log.debug(
                "ParamPanel(persist_key=%r) constructed; persistence is a no-op in v1",
                persist_key,
            )

    def bind(self, gui: Any) -> None:
        """Bind the per-frame ti.ui.Gui handle. Call at the top of each render-loop iteration."""
        self._gui = gui

    @contextmanager
    def folder(
        self,
        title: str,
        x: float,
        y: float,
        width: float,
        height: float,
    ) -> Iterator[Any]:
        """Open a Taichi sub-window as a named folder.

        Coordinates are normalized [0,1]^2 (Taichi convention). Yields the
        sub-window handle which exposes `slider_float`/`slider_int`/`checkbox`/
        `button`/`color_edit_3`/`text` directly.
        """
        if self._gui is None:
            raise RuntimeError(
                "ParamPanel.folder called before bind(gui); call panel.bind(window.get_gui()) first"
            )
        with self._gui.sub_window(title, x, y, width, height) as w:
            yield w

    def refresh_displays(self) -> None:
        """API-parity no-op with common-web's `panel.refreshDisplays()`.

        Taichi GGUI re-renders all bound widgets every frame from the underlying
        Python state — no slider-freeze-on-external-mutation issue equivalent to
        lil-gui's.
        """
        log.debug("ParamPanel.refresh_displays called (no-op on Taichi GGUI)")
