"""Exercise source FIFO interface overloads, references, events and delta accounting."""

import ctypes

from amal.eda.systemc import sc_core as sc


class Fifo(sc.fifo_channel[int]):
    """Retain native FIFO storage inside a Python-defined primitive channel."""

    def __init__(self, name):
        """Create a one-slot FIFO to force producer and consumer suspension."""
        super().__init__(name, 1)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
fifo = Fifo("fifo")
input_port = sc.sc_fifo_in[int]("input")
output_port = sc.sc_fifo_out[int]("output")
input_port.bind(fifo)
output_port.bind(fifo)
del module_name
type(input_port.get_interface()).read.__release_gil__ = True
type(output_port.get_interface()).write.__release_gil__ = True
records = []
written = []
read = []
model.sc_method("written", lambda: written.append(str(sc.sc_time_stamp())))
model.sensitive(fifo.data_written_event())
model.dont_initialize()
model.sc_method("read", lambda: read.append(str(sc.sc_time_stamp())))
model.sensitive(fifo.data_read_event())
model.dont_initialize()


def producer():
    """Fill, reject a second sample, then block until the first is consumed."""
    interface = output_port.get_interface()
    assert interface.num_free() == 1
    assert interface.nb_write(17)
    assert interface.num_free() == 0
    assert not interface.nb_write(99)
    interface.write(23)
    records.append(("write", str(sc.sc_time_stamp())))


def consumer():
    """Read by reference, check delta visibility, then read by value after blocking."""
    interface = input_port.get_interface()
    value = ctypes.c_int(-1)
    assert not interface.nb_read(value)
    assert value.value == -1
    sc.wait(1, sc.SC_NS)
    assert interface.num_available() == 1
    assert interface.nb_read(value)
    assert value.value == 17
    assert interface.num_available() == 0
    assert not interface.nb_read(value)
    assert value.value == 17
    assert interface.read() == 23
    records.append(("read", str(sc.sc_time_stamp())))
    sc.wait(sc.SC_ZERO_TIME)
    assert interface.num_available() == 0
    assert output_port.get_interface().num_free() == 1


handles = [sc.sc_spawn(producer, "producer"), sc.sc_spawn(consumer, "consumer")]
sc.sc_start(2, sc.SC_NS)
assert records == [("write", "1 ns"), ("read", "1 ns")], records
assert written == ["0 s", "1 ns"], written
assert read == ["1 ns", "1 ns"], read
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("NATIVE_FIFO_INTERFACE_OK")
