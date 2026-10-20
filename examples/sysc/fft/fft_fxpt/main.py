"""Port Accellera sysc/fft/fft_fxpt/main.cpp (Apache-2.0 LICENSE/NOTICE)."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "fft_flpt"))
from main import run

if __name__ == "__main__":
    run(integer=True)
