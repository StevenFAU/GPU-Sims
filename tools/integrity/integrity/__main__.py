"""Entry point: `python3 -m integrity`."""

from __future__ import annotations

import sys

from integrity.runner import main


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
