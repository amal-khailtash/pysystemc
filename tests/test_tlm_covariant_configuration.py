"""Check conditional abstract socket reflection in the selected native build."""

import subprocess
import sys


def test_covariant_getter_reflection_matches_selected_configuration(tmp_path):
    """Verify ordinary headers omit both getters and opt-in headers expose them.

    The check runs before model construction in a fresh interpreter. SystemC's
    own API-version check rejects a library compiled with a different flag.
    """
    code = '''
from amal.eda.systemc import tlm
import cppyy
cppyy.cppdef("""
#ifdef SC_ENABLE_COVARIANT_VIRTUAL_BASE
constexpr bool pysystemc_covariant_selected = true;
#else
constexpr bool pysystemc_covariant_selected = false;
#endif
""")
enabled = bool(cppyy.gbl.pysystemc_covariant_selected)
assert hasattr(tlm.tlm_base_socket_if, "get_base_interface") == enabled
if enabled:
    doc = tlm.tlm_base_socket_if.get_base_interface.__doc__
    assert "sc_core::sc_interface&" in doc
    assert "const" in doc
    print("COVARIANT_ENABLED_PRESENT")
else:
    print("COVARIANT_DEFAULT_ABSENT")
'''
    result = subprocess.run([sys.executable, "-c", code], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    assert any(marker in result.stdout for marker in ("COVARIANT_DEFAULT_ABSENT", "COVARIANT_ENABLED_PRESENT"))
