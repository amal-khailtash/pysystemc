"""Check native virtual defaults and equivalent public signal/port template defaults."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/inherited_defaults_probe.h"))
probe = cppyy.gbl.inherited_defaults_probe
derived = probe.Derived()
assert derived.value() == 229
# An explicit class method selects the qualified base implementation.
assert probe.Base.value(derived) == 111
# This borrowed static base view retains virtual dispatch; derived owns storage.
base = cppyy.bind_object(cppyy.addressof(derived), probe.Base)
base.__python_owns__ = False
assert base.value() == 211
assert derived.value(11) == 211
assert probe.template_default_mask() == 3

default_signal_type = sc.sc_signal[int]
explicit_signal_type = sc.sc_signal[int, sc.SC_ONE_WRITER]
default_port_type = sc.sc_port[sc.sc_signal_in_if[int]]
explicit_port_type = sc.sc_port[sc.sc_signal_in_if[int], 1, sc.SC_ONE_OR_MORE_BOUND]
assert default_signal_type is explicit_signal_type
assert default_signal_type.__cpp_name__ == explicit_signal_type.__cpp_name__
assert default_port_type is explicit_port_type
assert default_port_type.__cpp_name__ == explicit_port_type.__cpp_name__

name = sc.sc_module_name("top")
top = sc.sc_module(name)
default_signal = default_signal_type("default_signal")
explicit_signal = explicit_signal_type("explicit_signal", 0)
default_port = default_port_type("default_port")
explicit_port = explicit_port_type("explicit_port")
default_port(default_signal)
explicit_port(explicit_signal)
del name
sc.sc_start(sc.SC_ZERO_TIME)
assert default_port.size() == explicit_port.size() == 1
assert default_port[0].read() == explicit_port[0].read() == 0
assert default_signal.read() == explicit_signal.read() == 0
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
sc.sc_stop()
print("INHERITED_DEFAULTS_REFERENCE 229 111 211 211 3 1 1 0 0")
