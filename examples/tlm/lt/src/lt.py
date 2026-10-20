"""Accellera LT executable Python port (Apache-2.0 LICENSE/NOTICE)."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "common"))
from lt_models import run

if __name__ == "__main__":
    run()
