"""Verify unbound input ports fail with native diagnostics rather than crash."""

from amal.eda.systemc import sc_core as sc

module_name = sc.sc_module_name("unbound")
model = sc.sc_module(module_name)
port = sc.sc_in[bool]("input")
del module_name
try:
    sc.sc_start(sc.SC_ZERO_TIME)
except Exception as error:
    assert "complete binding failed" in str(error), str(error)
    assert "unbound.input" in str(error), str(error)
else:
    raise AssertionError("unbound required port accepted")
print("BINDING_ERROR_OK")
