"""Run the Accellera sysc/fir/main_rtl.cpp port; Apache-2.0 LICENSE/NOTICE."""

import importlib.util
from pathlib import Path

_spec = importlib.util.spec_from_file_location("pysystemc_example_fir", Path(__file__).with_name("main.py"))
_module = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_module)
run = _module.run

if __name__ == "__main__":
    run(rtl=True)
