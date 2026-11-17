"""Port Accellera sysc/fft/fft_fxpt/main.cpp (Apache-2.0 LICENSE/NOTICE)."""

import importlib.util
from pathlib import Path

_source = Path(__file__).resolve().parents[1] / "fft_flpt/main.py"
_spec = importlib.util.spec_from_file_location("pysystemc_example_fft_flpt", _source)
_module = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_module)
run = _module.run

if __name__ == "__main__":
    run(integer=True)
