"""Exercise structural reference returns with borrowed native identity and mutation."""

from typing import Protocol
from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy
import gc
import weakref


@sc.interface(
    methods={
        "read": ("const sc_dt::sc_biguint<137>&", ""),
        "event": ("const sc_core::sc_event&", ""),
        "mutable_value": ("sc_dt::sc_biguint<137>&", "int index"),
    }
)
class Contract(Protocol):
    """Expose borrowed arbitrary-width storage and an event."""


class Owner:
    """Keep native storage whose references must preserve identity."""

    def __init__(self):
        """Construct payload and event before binding."""
        self.value = sc_dt.sc_biguint[137](17)
        self.changed = sc.sc_event("changed")

    def read(self):
        """Borrow the current wide storage."""
        return self.value

    def mutable_value(self, index):
        """Borrow writable storage through a named native argument."""
        assert index == 0
        return self.value

    def event(self):
        """Borrow the same event on every call."""
        return self.changed


name = sc.sc_module_name("top")
top = sc.sc_module(name)
owner = Owner()
port = sc.sc_port[Contract]("port")
port(owner)
del name
native = port.get_interface()
assert cppyy.addressof(native.read()) == cppyy.addressof(owner.value)
assert cppyy.addressof(native.event()) == cppyy.addressof(owner.changed)
owner.value[136] = True
assert native.read()[136]
old_value = native.read()
old_address = cppyy.addressof(old_value)
owner.value = sc_dt.sc_biguint[137](23)
gc.collect()
assert old_value[136] and cppyy.addressof(old_value) == old_address
assert native.read().to_uint() == 23
owner.value = 42
try:
    native.read()
except TypeError as error:
    assert "retained native storage" in str(error)
else:
    raise AssertionError("Python integer accepted for a native reference")
owner.value = sc_dt.sc_biguint[137](29)
assert native.read().to_uint() == 29
native.mutable_value(0)[136] = True
assert owner.value[136]
owner.value[136] = False
owner_ref = weakref.ref(owner)
del owner
gc.collect()
assert owner_ref() is not None
seen = []
top.sc_method("observe", lambda: seen.append(sc.sc_time_stamp().to_double()))
top.sensitive(native.event())
top.dont_initialize()
owner_ref().changed.notify(sc.sc_time(2, sc.SC_NS))
sc.sc_start(3, sc.SC_NS)
assert len(seen) == 1 and sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
sc.sc_stop()
print("REFERENCE_RESULT 29 1 3 ns")
print("CUSTOM_REFERENCE_RETURNS_OK")
