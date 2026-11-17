"""Exercise small public adapter edges against a native SystemC reference."""

import ctypes

from amal.eda.systemc import sc_core as sc
import cppyy
from amal.eda.systemc.sc_core._pvector import _wrap


def expect_type_error(action):
    """Run one rejected public call and require TypeError.

    Args:
        action: Zero-argument callable performing the invalid operation.

    Returns:
        None when the public adapter rejects the operation.

    Raises:
        AssertionError: The operation returns or raises a different exception.
    """
    try:
        action()
    except TypeError:
        return
    raise AssertionError("invalid adapter call did not raise TypeError")


class Model(sc.sc_module):
    """Own the signal, exports, and FIFO through a completed native module."""

    def __init__(self):
        """Construct bound exports and fill a deterministic two-slot FIFO."""
        module_name = sc.sc_module_name("core_small_adapter_edges")
        super().__init__(module_name)
        interface_type = sc.sc_signal_in_if[int]
        export_by_call = sc.sc_export(interface_type)
        export_by_index = sc.sc_export[interface_type]
        assert export_by_call is export_by_index
        self.signal = sc.sc_signal[int]("edge_signal")
        self.first_export = export_by_call("call_export")
        self.second_export = export_by_index("index_export")
        self.first_export.bind(self.signal)
        self.second_export.bind(self.signal)
        assert cppyy.addressof(self.first_export.get_interface()) == cppyy.addressof(self.signal)
        assert cppyy.addressof(self.second_export.get_interface()) == cppyy.addressof(self.signal)
        self.signal.write(23)
        fifo_by_call = sc.sc_fifo(int)
        fifo_by_index = sc.sc_fifo[int]
        assert fifo_by_call is fifo_by_index
        self.fifo = fifo_by_call("edge_fifo", 2)
        assert self.fifo.nb_write(31) and self.fifo.nb_write(47)


model = Model()

vector_type = sc.sc_pvector["sc_core::sc_time*"]
get_before = vector_type.__getitem__
set_before = vector_type.__setitem__
fetch_before = vector_type.fetch
_wrap(vector_type)
_wrap(vector_type)
assert vector_type.__getitem__ is get_before
assert vector_type.__setitem__ is set_before
assert vector_type.fetch is fetch_before
assert sc.sc_pvector["sc_core::sc_time*"] is vector_type
first_time = sc.sc_time(11, sc.SC_NS)
second_time = sc.sc_time(22, sc.SC_NS)
vector = vector_type()
vector[0] = cppyy.bind_object(cppyy.addressof(first_time), sc.sc_time)
vector[3] = cppyy.bind_object(cppyy.addressof(second_time), sc.sc_time)
pointer_values = tuple(
    "null" if not bool(vector[index]) else str(round(vector[index].to_seconds() * 1e9))
    for index in range(vector.size())
)
assert pointer_values == ("11", "null", "null", "22")

expect_type_error(lambda: sc.sc_signal_value(23))

invalid_start_calls = (
    lambda: sc.sc_start(unit=sc.SC_NS),
    lambda: sc.sc_start(sc.sc_time(1, sc.SC_NS), sc.SC_RUN_TO_TIME, p=sc.SC_RUN_TO_TIME),
    lambda: sc.sc_start(1),
)
for invalid_start in invalid_start_calls:
    before_time = sc.sc_time(sc.sc_time_stamp())
    before_status = sc.sc_get_status()
    expect_type_error(invalid_start)
    assert sc.sc_time_stamp() == before_time
    assert sc.sc_get_status() == before_status

sc.sc_start(sc.SC_ZERO_TIME)
assert model.fifo.num_available() == 2
first_value = ctypes.c_int(-1)
second_value = ctypes.c_int(-1)
assert model.fifo.nb_read(first_value) and model.fifo.nb_read(second_value)
assert (first_value.value, second_value.value) == (31, 47)
sc.sc_start(1, sc.SC_NS)
assert sc.sc_time_stamp() == sc.sc_time(1, sc.SC_NS)
signal_value = sc.sc_signal_value(model.signal)
export_value = model.first_export.read()
assert export_value == signal_value == 23

print(f"CORE_EXPORT_EDGE value={export_value} bindings=2")
print(f"CORE_FIFO_EDGE values={first_value.value},{second_value.value} capacity=2")
print(f"CORE_PVECTOR_EDGE slots={','.join(pointer_values)} size={len(pointer_values)}")
print(f"CORE_SIGNAL_VALUE_EDGE value={signal_value}")
print(f"CORE_START_EDGE time_ns={round(sc.sc_time_stamp().to_seconds() * 1e9)}")
