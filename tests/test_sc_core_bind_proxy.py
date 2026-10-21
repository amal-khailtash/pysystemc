"""Compare public native positional-binding proxy constructors and nil storage."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_bind_proxy(tmp_path):
    """Verify borrowed addresses, empty proxies and immutable native nil state."""
    output = run_example(tmp_path, "tests/models/sc_core_bind_proxy.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_bind_proxy.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "BIND_PROXY_REFERENCE 1 1 1" in output and "BIND_PROXY_REFERENCE 1 1 1" in native.stdout
