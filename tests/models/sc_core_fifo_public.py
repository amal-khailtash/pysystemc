"""Exercise public native sc_fifo overloads and compare them with SystemC."""

import ctypes
import gc
import sys
import traceback

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy

cppyy.cppdef(
    r"""
/** Observe destruction of an owned FIFO proxy after its processes finish.
 * The derived object owns only its native FIFO base; it stores no callback or
 * Python references and performs no GIL operations.
 */
namespace fifo_lifecycle {
inline int destroyed = 0;
struct Tracked : sc_core::sc_fifo<int> {
  Tracked() : sc_core::sc_fifo<int>("lifecycle_fifo", 1) {}
  ~Tracked() override { ++destroyed; }
};
/** Return the ABI-native interface name expected by sc_fifo::register_port. */
template <class T> const char* input_name() { return typeid(sc_core::sc_fifo_in_if<T>).name(); }
template <class T> const char* output_name() { return typeid(sc_core::sc_fifo_out_if<T>).name(); }
}
"""
)


def report_invalid_size():
    """Check that a zero-capacity FIFO reports the native invalid-size error."""
    try:
        sc.sc_fifo[int]("invalid_fifo", 0)
    except Exception as error:
        print("FIFO_INVALID_SIZE", str(error).replace("\n", " "))
        return
    raise AssertionError("zero-capacity FIFO construction did not fail")


if len(sys.argv) > 1 and sys.argv[1] == "invalid-size":
    report_invalid_size()
    raise SystemExit(0)


def report_port_conflict(direction):
    """Call public register_port twice and retain its exact reader/writer report."""
    module_name = sc.sc_module_name("fifo_port_error")
    _module = sc.sc_module(module_name)
    fifo = sc.sc_fifo[int]("conflict_fifo", 2)
    is_reader = direction == "reader"
    port_type = sc.sc_fifo_in[int] if is_reader else sc.sc_fifo_out[int]
    first = port_type("first")
    second = port_type("second")
    type_name = cppyy.gbl.fifo_lifecycle.input_name[int]() if is_reader else cppyy.gbl.fifo_lifecycle.output_name[int]()
    try:
        fifo.register_port(first, type_name)
        fifo.register_port(second, type_name)
    except Exception as error:
        print("FIFO_REGISTER_ERROR", direction, str(error).replace("\n", " "))
        return
    raise AssertionError(f"duplicate FIFO {direction} registration did not report")


if len(sys.argv) > 1 and sys.argv[1].startswith("duplicate-"):
    report_port_conflict(sys.argv[1].removeprefix("duplicate-"))
    raise SystemExit(0)


class Model(sc.sc_module):
    """Own FIFO channels, trace storage and blocking producer/consumer processes."""

    def __init__(self):
        """Construct default, named and arbitrary-width FIFOs and register work."""
        module_name = sc.sc_module_name("fifo_public")
        super().__init__(module_name)
        self.default_fifo = sc.sc_fifo[int]()
        self.named_fifo = sc.sc_fifo[int]("int_fifo", 1)
        self.wide_fifo = sc.sc_fifo[dt.sc_biguint[137]]("wide_fifo", 2)
        self.assignment_fifo = sc.sc_fifo[int]("assignment_fifo", 1)
        self.float_fifo = sc.sc_fifo[float]("float_fifo", 1)
        self.bool_fifo = sc.sc_fifo[bool]("bool_fifo", 1)
        self.wide_conversion_fifo = sc.sc_fifo[dt.sc_biguint[137]]("wide_conversion_fifo", 1)
        self.text_fifo = sc.sc_fifo[int]("text_fifo", 2)
        self.lifecycle_fifo = cppyy.gbl.fifo_lifecycle.Tracked()
        assert self.text_fifo.nb_write(51) and self.text_fifo.nb_write(61)
        self.sample = dt.sc_biguint[137](0)
        self.trace = sc.sc_create_vcd_trace_file("fifo_family")
        self.trace.set_time_unit(1, sc.SC_NS)
        sc.sc_trace(self.trace, self.sample, "wide_sample")
        self.wide_fifo.trace(self.trace)
        self.records = []
        self.errors = []
        self.scalar_conversions = None
        self.sc_thread("int_producer", lambda: self.run_process(self.int_producer))
        self.sc_thread("int_consumer", lambda: self.run_process(self.int_consumer))
        self.sc_thread("wide_producer", lambda: self.run_process(self.wide_producer))
        self.sc_thread("wide_consumer", lambda: self.run_process(self.wide_consumer))
        self.sc_thread("assignment_conversion", lambda: self.run_process(self.assignment_conversion))

    def run_process(self, callback):
        """Capture native scheduler callback failures for a useful assertion."""
        try:
            callback()
        except BaseException:
            self.errors.append((callback.__name__, traceback.format_exc()))
            sc.sc_stop()

    def int_producer(self):
        """Exercise blocking writes until the consumer frees each FIFO slot."""
        fifo = self.named_fifo
        assert fifo.num_free() == 1 and fifo.num_available() == 0
        fifo.write(17)
        assert fifo.num_free() == 0 and fifo.num_available() == 0
        fifo.write(23)
        fifo.write(41)
        assert not fifo.nb_write(99)
        self.records.append(("int_producer", str(sc.sc_time_stamp())))

    def int_consumer(self):
        """Exercise empty and successful nonblocking reads plus both blocking reads."""
        fifo = self.named_fifo
        output = ctypes.c_int(-1)
        assert not fifo.nb_read(output) and output.value == -1
        sc.wait(sc.SC_ZERO_TIME)
        assert fifo.num_available() == 1
        fifo.read(output)
        assert output.value == 17
        assert fifo.num_available() == 0 and fifo.num_free() == 0, (fifo.num_available(), fifo.num_free())
        value = fifo.read()
        assert value == 23
        fifo.read(output)
        assert output.value == 41
        assert not fifo.nb_read(output) and output.value == 41
        sc.wait(sc.SC_ZERO_TIME)
        assert fifo.num_available() == 0 and fifo.num_free() == 1
        self.records.append(("int_consumer", value, str(sc.sc_time_stamp())))

    def wide_producer(self):
        """Queue two full-width samples, block on a third and trace value changes."""
        uint137 = dt.sc_biguint[137]
        samples = (
            uint137("174224571863520493293247799005065324265"),
            uint137("174224571863520493293247799005065324267"),
            uint137("174224571863520493293247799005065324271"),
        )
        fifo = self.wide_fifo
        assert fifo.nb_write(samples[0])
        assert fifo.nb_write(samples[1])
        assert not fifo.nb_write(samples[2])
        self.sample.__assign__(samples[0])
        fifo.write(samples[2])
        sc.wait(1, sc.SC_NS)
        self.sample.__assign__(samples[2])
        self.records.append(("wide_producer", str(sc.sc_time_stamp())))

    def wide_consumer(self):
        """Read wide values by reference and value and check empty output stability."""
        uint137 = dt.sc_biguint[137]
        fifo = self.wide_fifo
        output = uint137(9)
        assert not fifo.nb_read(output) and output.to_string() == "9"
        sc.wait(sc.SC_ZERO_TIME)
        assert fifo.num_available() == 2 and fifo.num_free() == 0
        assert fifo.nb_read(output)
        first = output.to_string()
        second = fifo.read().to_string()
        assert first == "174224571863520493293247799005065324265"
        assert second == "174224571863520493293247799005065324267"
        assert fifo.read().to_string() == "174224571863520493293247799005065324271"
        assert not fifo.nb_read(output) and output.to_string() == first
        self.records.append(("wide_consumer", first, second, str(sc.sc_time_stamp())))

    def assignment_conversion(self):
        """Exercise Python assignment and native named conversion on int and uint137."""
        self.assignment_fifo.__assign__(73)
        sc.wait(sc.SC_ZERO_TIME)
        scalar = int(self.assignment_fifo)
        assert scalar == 73
        sc.wait(sc.SC_ZERO_TIME)
        uint137 = dt.sc_biguint[137]
        expected = "174224571863520493293247799005065324269"
        self.wide_conversion_fifo.__assign__(uint137(expected))
        sc.wait(sc.SC_ZERO_TIME)
        conversion = getattr(self.wide_conversion_fifo, "operator sc_dt::sc_biguint<137>")
        wide = conversion().to_string()
        assert wide == expected
        self.float_fifo.__assign__(2.75)
        self.bool_fifo.__assign__(True)
        sc.wait(sc.SC_ZERO_TIME)
        floating = float(self.float_fifo)
        boolean = bool(self.bool_fifo)
        assert floating == 2.75 and boolean is True
        self.scalar_conversions = (floating, boolean)
        self.records.append(("assignment_conversion", scalar, wide, str(sc.sc_time_stamp())))


model = Model()
assert model.default_fifo.num_free() == 16
assert model.default_fifo.kind() == "sc_fifo"
assert model.default_fifo.name().startswith("fifo_")
assert model.named_fifo.name().endswith("int_fifo")
assert model.named_fifo.kind() == "sc_fifo"
assert model.named_fifo.data_read_event().name().endswith("_read_event")
assert model.named_fifo.data_written_event().name().endswith("_write_event")
assert model.wide_fifo.num_free() == 2

sc.sc_start(2, sc.SC_NS)
assert not model.errors, model.errors
expected_records = [
    ("int_producer", "0 s"),
    ("int_consumer", 23, "0 s"),
    ("wide_consumer", "174224571863520493293247799005065324265", "174224571863520493293247799005065324267", "0 s"),
    ("wide_producer", "1 ns"),
    ("assignment_conversion", 73, "174224571863520493293247799005065324269", "0 s"),
]
assert sorted(model.records) == sorted(expected_records), model.records

int_stream = cppyy.gbl.std.ostringstream()
model.text_fifo.print(int_stream)
int_print = str(int_stream.str())
insert_stream = cppyy.gbl.std.ostringstream()
insert_stream << model.text_fifo
stream_insertion = str(insert_stream.str())
assert stream_insertion == int_print
int_stream.str("")
model.text_fifo.dump(int_stream)
int_dump = str(int_stream.str())
assert int_print == "51\n61\n", repr(int_print)
assert "name = " in int_dump and int_dump.endswith("value[1] = 61\n"), repr(int_dump)
print(
    "FIFO_REFERENCE",
    model.default_fifo.num_free(),
    model.default_fifo.kind(),
    model.named_fifo.name(),
    23,
    "174224571863520493293247799005065324265",
    "174224571863520493293247799005065324267",
    model.named_fifo.num_available(),
    model.named_fifo.num_free(),
    model.wide_fifo.num_available(),
    model.wide_fifo.num_free(),
)
print("FIFO_PROCESSES", "int_producer=0 s", "int_consumer=0 s", "wide_producer=1 ns", "wide_consumer=0 s")
print("FIFO_ASSIGNMENT_CONVERSION", 73, "174224571863520493293247799005065324269")
print("FIFO_SCALAR_CONVERSIONS", *model.scalar_conversions)
print("FIFO_TEXT", int_print.replace("\n", "|").rstrip("|"), int_dump.replace("\n", "|").rstrip("|"))
print("FIFO_STREAM", stream_insertion.replace("\n", "|").rstrip("|"))
print("FIFO_EVENTS", model.named_fifo.data_read_event().name(), model.named_fifo.data_written_event().name())

sc.sc_stop()
sc.sc_close_vcd_trace_file(model.trace)
del model.lifecycle_fifo
gc.collect()
assert cppyy.gbl.fifo_lifecycle.destroyed == 1
print("FIFO_DESTRUCTION", int(cppyy.gbl.fifo_lifecycle.destroyed))
print("FIFO_PUBLIC_OK")
