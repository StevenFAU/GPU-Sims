"""Logging wrapper for Stack D.

Mirrors common-cpp's gpusims::log surface (info/warn/error free functions) and
common-web's log.ts pattern. Backs Python's stdlib `logging` under the
"gpusims" logger name. Per-sim code calls `log.info(...)` etc.; no per-sim
logger setup needed.
"""

from __future__ import annotations

import logging
import sys
from typing import Final

_LOGGER_NAME: Final[str] = "gpusims"

_configured = False


def _configure_once() -> None:
    """Configure the gpusims logger once per process.

    Idempotent — repeated calls are no-ops. Called lazily on first log call.
    """
    global _configured
    if _configured:
        return
    logger = logging.getLogger(_LOGGER_NAME)
    if logger.handlers:
        # Already configured (e.g., by an embedding app); leave alone.
        _configured = True
        return
    handler = logging.StreamHandler(sys.stderr)
    handler.setFormatter(
        logging.Formatter("[gpusims] [%(levelname)s] %(message)s")
    )
    logger.addHandler(handler)
    logger.setLevel(logging.INFO)
    logger.propagate = False
    _configured = True


def get_logger() -> logging.Logger:
    """Return the configured gpusims logger.

    Useful for child-logger creation: `get_logger().getChild("mpm")`.
    """
    _configure_once()
    return logging.getLogger(_LOGGER_NAME)


class _LogProxy:
    """Convenience callable group: log.info(...), log.warn(...), log.error(...).

    Mirrors common-cpp's `gpusims::log::info` etc. free-function shape.
    """

    @staticmethod
    def info(msg: str, *args: object) -> None:
        _configure_once()
        logging.getLogger(_LOGGER_NAME).info(msg, *args)

    @staticmethod
    def warn(msg: str, *args: object) -> None:
        _configure_once()
        logging.getLogger(_LOGGER_NAME).warning(msg, *args)

    @staticmethod
    def warning(msg: str, *args: object) -> None:
        """Alias for `warn`; matches Python stdlib name for less surprise."""
        _LogProxy.warn(msg, *args)

    @staticmethod
    def error(msg: str, *args: object) -> None:
        _configure_once()
        logging.getLogger(_LOGGER_NAME).error(msg, *args)

    @staticmethod
    def debug(msg: str, *args: object) -> None:
        _configure_once()
        logging.getLogger(_LOGGER_NAME).debug(msg, *args)


log = _LogProxy()
