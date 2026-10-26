"""Exercise native fixed-point observer callbacks and generic value bases."""

import gc
from pathlib import Path
from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_dt_observers_probe.h"))
probe = cppyy.gbl.datatype_observer_probe


class PythonNum(probe.Num):
    """Retain callback records while borrowing each native callback value."""

    def __init__(self):
        """Construct a public native observer subobject and own its records."""
        super().__init__()
        self.records = []

    def construct(self, value):
        """Observe construction without retaining the borrowed value."""
        self.records.append(value.to_double())

    def read(self, value):
        """Observe a read without retaining the borrowed value."""
        if getattr(self, "fail_read", False):
            raise ValueError("observer read failure")
        self.records.append(value.to_double() + 1)

    def write(self, value):
        """Observe a write without retaining the borrowed value."""
        self.records.append(value.to_double() + 2)

    def destruct(self, value):
        """Observe destruction without retaining the borrowed value."""
        self.records.append(value.to_double() + 3)


class PythonFastNum(probe.FastNum):
    """Retain callback records while borrowing each native callback value."""

    def __init__(self):
        """Construct a public native observer subobject and own its records."""
        super().__init__()
        self.records = []

    def construct(self, value):
        """Observe construction without retaining the borrowed value."""
        self.records.append(value.to_double())

    def read(self, value):
        """Observe a read without retaining the borrowed value."""
        self.records.append(value.to_double() + 1)

    def write(self, value):
        """Observe a write without retaining the borrowed value."""
        self.records.append(value.to_double() + 2)

    def destruct(self, value):
        """Observe destruction without retaining the borrowed value."""
        self.records.append(value.to_double() + 3)


class PythonVal(probe.Val):
    """Retain callback records while borrowing each native callback value."""

    def __init__(self):
        """Construct a public native observer subobject and own its records."""
        super().__init__()
        self.records = []

    def construct(self, value):
        """Observe construction without retaining the borrowed value."""
        self.records.append(value.to_double())

    def read(self, value):
        """Observe a read without retaining the borrowed value."""
        self.records.append(value.to_double() + 1)

    def write(self, value):
        """Observe a write without retaining the borrowed value."""
        self.records.append(value.to_double() + 2)

    def destruct(self, value):
        """Observe destruction without retaining the borrowed value."""
        self.records.append(value.to_double() + 3)


class PythonFastVal(probe.FastVal):
    """Retain callback records while borrowing each native callback value."""

    def __init__(self):
        """Construct a public native observer subobject and own its records."""
        super().__init__()
        self.records = []

    def construct(self, value):
        """Observe construction without retaining the borrowed value."""
        self.records.append(value.to_double())

    def read(self, value):
        """Observe a read without retaining the borrowed value."""
        self.records.append(value.to_double() + 1)

    def write(self, value):
        """Observe a write without retaining the borrowed value."""
        self.records.append(value.to_double() + 2)

    def destruct(self, value):
        """Observe destruction without retaining the borrowed value."""
        self.records.append(value.to_double() + 3)


def run_observer(observer_type, probe_type, value_type, label):
    """Check exact virtual/default/factory calls and owner destruction.

    Args:
        observer_type: Python subclass overriding each native virtual method.
        probe_type: Public native-derived probe exposing base construction.
        value_type: Native fixed-point value class.
        label: Stable record label for the C++ comparison.
    """
    initial = probe_type.destroyed
    value = value_type(1.25)
    observer = observer_type()
    observer.defaults(value)
    assert observer.records == []
    if label == "Num":
        observer.fail_read = True
        try:
            probe_type.dispatch(observer, value)
        except ValueError as error:
            assert "observer read failure" in str(error)
        else:
            raise AssertionError("native virtual dispatch swallowed the Python error")
        assert observer.records == [1.25]
        observer.records.clear()
        observer.fail_read = False
    probe_type.dispatch(observer, value)
    assert observer.records == [1.25, 2.25, 3.25, 4.25]
    assert probe_type.factory_identity(observer)
    print("DT_OBSERVER", label, *observer.records)
    del observer
    gc.collect()
    assert probe_type.destroyed == initial + 1
    print("DT_OBSERVER_DTOR", label, 1)


run_observer(PythonNum, probe.Num, dt.sc_fixed[23, 11], "Num")
run_observer(PythonFastNum, probe.FastNum, dt.sc_fixed_fast[23, 11], "FastNum")
run_observer(PythonVal, probe.Val, dt.sc_fxval, "Val")
run_observer(PythonFastVal, probe.FastVal, dt.sc_fxval_fast, "FastVal")

base = probe.ValueBase()
assert isinstance(base, dt.sc_value_base)
del base
gc.collect()
assert probe.ValueBase.destroyed == 1
print("DT_VALUE_BASE", 1)
generic = probe.Generic(-37)
assert probe.generic_arrow(generic) == cppyy.addressof(generic)
assert probe.generic_const_arrow(generic) == cppyy.addressof(generic)
assert dt.sc_generic_base[probe.Generic].__cpp_name__ in [
    t.__cpp_name__ for t in type(generic).__mro__ if hasattr(t, "__cpp_name__")
]
signed = dt.sc_signed(137)
unsigned = dt.sc_unsigned(137)
generic.to_sc_signed(signed)
generic.to_sc_unsigned(unsigned)
assert signed.to_int64() == -37
assert unsigned.to_uint64() == (1 << 64) - 37
left = dt.sc_uint[19](12)
right = dt.sc_uint[7](3)
concat = dt.concat(left, right)
assert probe.concat_arrow(concat) == cppyy.addressof(concat)
assert probe.concat_const_arrow(concat) == cppyy.addressof(concat)
assert concat.to_uint64() == (12 << 7) | 3
print("DT_GENERIC", signed.to_int64(), unsigned.to_uint64(), concat.to_uint64())
print("SC_DT_OBSERVERS_PUBLIC_OK")
