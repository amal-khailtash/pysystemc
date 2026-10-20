"""Port Accellera at_4_phase executable (Apache-2.0; upstream LICENSE/NOTICE)."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "common"))
from models import run

if __name__ == "__main__":
    run((4, 4), optional=False)
