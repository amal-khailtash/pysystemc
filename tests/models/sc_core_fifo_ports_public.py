"""Compare the complete native FIFO input/output port surface with C++."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy
import sys


cppyy.cppdef(
    r"""
    #include <systemc>
    namespace pysystemc_fifo_ports_public {
    template <class T>
    class fifo_in_probe : public sc_core::sc_fifo_in<T> {
    public:
      explicit fifo_in_probe(const char* name) : sc_core::sc_fifo_in<T>(name) {}
      bool before_called = false;
      bool binding_state_allocated() const { return this->m_bind_info != 0; }
    protected:
      void before_end_of_elaboration() override {
        sc_core::sc_fifo_in<T>::before_end_of_elaboration();
        before_called = true;
      }
    };
    template <class T>
    class fifo_out_probe : public sc_core::sc_fifo_out<T> {
    public:
      explicit fifo_out_probe(const char* name) : sc_core::sc_fifo_out<T>(name) {}
      bool before_called = false;
      bool binding_state_allocated() const { return this->m_bind_info != 0; }
    protected:
      void before_end_of_elaboration() override {
        sc_core::sc_fifo_out<T>::before_end_of_elaboration();
        before_called = true;
      }
    };
    }
    """
)


wide = sc_dt.sc_biguint[137]
input_type = sc.sc_fifo_in[wide]
output_type = sc.sc_fifo_out[wide]


if sys.argv[1:] == ["--errors"]:
    """Probe invalid unbound finders in a process that never starts its kernel."""
    module_name = sc.sc_module_name("error_top")
    error_top = sc.sc_module(module_name)
    error_in = input_type("error_in")
    error_out = output_type("error_out")
    reports = []

    def record_error(report, actions):
        """Copy the public fields needed from the borrowed native report."""
        reports.append((report.get_id(), report.get_msg_type()))

    sc.set_report_handler(record_error)
    previous_actions = sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_DO_NOTHING)
    try:
        unresolved_input = error_in.data_written().find_event()
        unresolved_output = error_out.data_read().find_event()
    finally:
        sc.sc_report_handler.set_actions(sc.SC_ERROR, previous_actions)
        sc.set_report_handler()
    assert cppyy.addressof(unresolved_input) == cppyy.addressof(sc.sc_event.none)
    assert cppyy.addressof(unresolved_output) == cppyy.addressof(sc.sc_event.none)
    assert reports == [(118, "find event failed"), (118, "find event failed")], reports
    print("FIFO_PORT_ERRORS\t1\t1")
    del module_name
    sys.exit(0)

module_name = sc.sc_module_name("top")
top = sc.sc_module(module_name)


def make_port_matrix(direction, port_type, interface_type, channels):
    """Create and bind a port through each public constructor form.

    Args:
        direction: ``in`` or ``out``; used to form native object names.
        port_type: The ``sc_fifo_in[T]`` or ``sc_fifo_out[T]`` template.
        interface_type: The matching combined FIFO interface template.
        channels: The eight retained one-slot FIFO channels shared by both directions.

    Returns:
        Tuples of channel, parent ports, and child port for all eight constructors.

    Each channel and port belongs to the active ``top`` module until simulation
    teardown. Parent ports are retained because child ports borrow their binding.
    """
    rows = []
    interface = interface_type[wide]
    generic_port = sc.sc_port[interface, 0, sc.SC_ONE_OR_MORE_BOUND]
    for index, channel in enumerate(channels):
        label = f"matrix_{direction}_{index}"
        parents = []
        if index == 0:
            parent = generic_port(f"{label}_parent")
            parent.bind(channel)
            parents.append(parent)
            port = port_type[wide]()
            port(parent)
        elif index == 1:
            port = port_type[wide](label)
            if direction == "in":
                port(channel)
            else:
                port.bind(channel)
        elif index == 2:
            port = port_type[wide](channel)
        elif index == 3:
            port = port_type[wide](label, channel)
        elif index in (4, 5):
            parent = generic_port(f"{label}_parent")
            parent.bind(channel)
            parents.append(parent)
            port = port_type[wide](parent) if index == 4 else port_type[wide](label, parent)
        else:
            parent = port_type[wide](f"{label}_parent")
            parent.bind(channel)
            parents.append(parent)
            port = port_type[wide](parent) if index == 6 else port_type[wide](label, parent)
        rows.append((channel, parents, port))
    return rows


matrix_channels = [sc.sc_fifo[wide](f"matrix_{index}_fifo", 1) for index in range(8)]
input_matrix = make_port_matrix("in", sc.sc_fifo_in, sc.sc_fifo_in_if, matrix_channels)
output_matrix = make_port_matrix("out", sc.sc_fifo_out, sc.sc_fifo_out_if, matrix_channels)
matrix = list(zip(input_matrix, output_matrix))

blocking_fifo = sc.sc_fifo[wide]("blocking_fifo", 1)
blocking_in = input_type("blocking_in")
blocking_in.bind(blocking_fifo)
blocking_out = output_type("blocking_out")
blocking_out(blocking_fifo)
life_fifo = sc.sc_fifo[wide]("lifetime_fifo", 1)
lifetime_in = input_type("lifetime_in")
lifetime_in.bind(life_fifo)
lifetime_out = output_type("lifetime_out")
lifetime_out(life_fifo)
lifetime_in.data_written()
lifetime_out.data_read()

probe_fifo = sc.sc_fifo[wide]("probe_fifo", 1)
probe_in = cppyy.gbl.pysystemc_fifo_ports_public.fifo_in_probe[wide]("probe_in")
probe_in.bind(probe_fifo)
probe_out = cppyy.gbl.pysystemc_fifo_ports_public.fifo_out_probe[wide]("probe_out")
probe_out.bind(probe_fifo)

activity = sc.sc_signal[bool]("activity")
trace = sc.sc_create_vcd_trace_file("fifo_ports_public")
trace.set_time_unit(1, sc.SC_NS)
sc.sc_trace(trace, activity, "activity")

written_events = []
read_events = []
process_records = []


def on_written():
    """Record input-port sensitivity to a FIFO data-written event."""
    written_events.append(str(sc.sc_time_stamp()))


def on_read():
    """Record output-port sensitivity to a FIFO data-read event."""
    read_events.append(str(sc.sc_time_stamp()))


top.sc_method("on_written", on_written)
top.sensitive(blocking_in.data_written())
top.dont_initialize()
top.sc_method("on_read", on_read)
top.sensitive(blocking_out.data_read())
top.dont_initialize()

start_event = sc.sc_event()


def producer():
    """Exercise blocking output and the wide-value write overloads."""
    sc.wait(start_event)
    sc.wait(1, sc.SC_NS)
    first = wide(17)
    first[136] = True
    blocking_out.write(first)
    activity.write(True)
    process_records.append(("write", "first", str(sc.sc_time_stamp())))
    sc.wait(1, sc.SC_NS)
    blocking_out.write(wide(29))
    activity.write(False)
    process_records.append(("write", "second", str(sc.sc_time_stamp())))


def consumer():
    """Exercise empty checks, output-reference read, and value-return read."""
    sc.wait(start_event)
    sc.wait(1, sc.SC_NS)
    value = wide(73)
    assert not blocking_in.nb_read(value)
    assert int(value.to_uint()) == 73
    blocking_in.read(value)
    assert value[136].to_bool() and int(value.to_uint()) == 17
    assert value.to_string() == str((1 << 136) + 17)
    process_records.append(("read", "first", str(sc.sc_time_stamp())))
    result = blocking_in.read()
    assert int(result.to_uint()) == 29 and not result[136].to_bool()
    assert result.to_string() == "29"
    process_records.append(("read", "second", str(sc.sc_time_stamp())))


producer_handle = sc.sc_spawn(producer, "producer")
consumer_handle = sc.sc_spawn(consumer, "consumer")
sc.sc_fifo_in[wide].read.__release_gil__ = True
sc.sc_fifo_out[wide].write.__release_gil__ = True
del module_name

assert input_type.data_type.__cpp_name__ == wide.__cpp_name__
assert output_type.data_type.__cpp_name__ == wide.__cpp_name__
assert input_type.if_type.__cpp_name__ == "sc_core::sc_fifo_in_if<sc_dt::sc_biguint<137>>"
assert output_type.if_type.__cpp_name__ == "sc_core::sc_fifo_out_if<sc_dt::sc_biguint<137>>"
assert input_type.in_if_type.__cpp_name__ == input_type.if_type.__cpp_name__
assert output_type.out_if_type.__cpp_name__ == output_type.if_type.__cpp_name__
assert input_type.this_type.__cpp_name__ == input_type.__cpp_name__
assert output_type.this_type.__cpp_name__ == output_type.__cpp_name__
assert "sc_port<" in input_type.base_type.__cpp_name__
assert "sc_port_b<" in input_type.in_port_type.__cpp_name__
assert "sc_port<" in output_type.base_type.__cpp_name__
assert "sc_port_b<" in output_type.out_port_type.__cpp_name__


def verify_port_metadata(port, channel, expected_kind):
    """Check inherited port bookkeeping, native finder lookup and object output.

    Args:
        port: One bound FIFO input or output port.
        channel: Its native FIFO channel.
        expected_kind: The native ``kind()`` string for the direction.
    """
    assert port.kind() == expected_kind
    assert port.bind_count() == 1
    assert port.size() == 1
    assert cppyy.addressof(port.get_interface()) == cppyy.addressof(channel)
    print_stream = cppyy.gbl.std.ostringstream()
    dump_stream = cppyy.gbl.std.ostringstream()
    port.print(print_stream)
    port.dump(dump_stream)
    assert port.name() in str(print_stream.str())
    assert port.name() in str(dump_stream.str())
    if expected_kind == "sc_fifo_in":
        finder = port.data_written()
        event = port.data_written_event()
    else:
        finder = port.data_read()
        event = port.data_read_event()
    assert cppyy.addressof(finder) == cppyy.addressof(
        port.data_written() if expected_kind == "sc_fifo_in" else port.data_read()
    )
    assert cppyy.addressof(finder.find_event()) == cppyy.addressof(event)
    port.trace(trace)


matrix_values = []


def matrix_worker():
    """Exercise each constructor, nonblocking call and final FIFO count in-kernel."""
    assert probe_in.before_called and probe_out.before_called
    assert not probe_in.binding_state_allocated() and not probe_out.binding_state_allocated()
    for index, ((input_fifo, _, input_port), (output_fifo, _, output_port)) in enumerate(matrix):
        assert cppyy.addressof(input_fifo) == cppyy.addressof(output_fifo)
        verify_port_metadata(input_port, input_fifo, "sc_fifo_in")
        verify_port_metadata(output_port, output_fifo, "sc_fifo_out")
        assert input_port.num_available() == 0
        assert output_port.num_free() == 1
        assert output_port.nb_write(wide(index + 17))
        assert not output_port.nb_write(wide(99))
    sc.wait(1, sc.SC_NS)
    for index, ((_, _, input_port), (_, _, _)) in enumerate(matrix):
        assert input_port.num_available() == 1
        value = wide(0)
        assert input_port.nb_read(value)
        assert int(value.to_uint()) == index + 17
        assert not input_port.nb_read(value)
        assert int(value.to_uint()) == index + 17
        matrix_values.append(int(value.to_uint()))
    sc.wait(1, sc.SC_NS)
    for (_, _, input_port), (_, _, output_port) in matrix:
        assert input_port.num_available() == 0
        assert output_port.num_free() == 1
    assert cppyy.addressof(blocking_in.data_written().find_event()) == cppyy.addressof(blocking_in.data_written_event())
    assert cppyy.addressof(blocking_out.data_read().find_event()) == cppyy.addressof(blocking_out.data_read_event())
    start_event.notify(sc.SC_ZERO_TIME)


assert probe_in.binding_state_allocated() and probe_out.binding_state_allocated()
matrix_handle = sc.sc_spawn(matrix_worker, "matrix_worker")
sc.sc_start(6, sc.SC_NS)
assert sorted(written_events) == ["3 ns", "4 ns"]
assert sorted(read_events) == ["3 ns", "4 ns"]
assert sorted(process_records) == [
    ("read", "first", "3 ns"),
    ("read", "second", "4 ns"),
    ("write", "first", "3 ns"),
    ("write", "second", "4 ns"),
]
assert matrix_handle.terminated() and producer_handle.terminated() and consumer_handle.terminated()
assert blocking_in.num_available() == 0 and blocking_out.num_free() == 1
sc.sc_stop()
sc.sc_close_vcd_trace_file(trace)
input_lifetime_name = str(lifetime_in.name())
output_lifetime_name = str(lifetime_out.name())
lifetime_in.__destruct__()
lifetime_out.__destruct__()
assert not sc.sc_find_object(input_lifetime_name)
assert not sc.sc_find_object(output_lifetime_name)
print("FIFO_PORT_MATRIX\t" + ",".join(map(str, matrix_values)))
print("FIFO_PORT_PROTECTED\t1\t1")
print("FIFO_PORT_LIFETIME\t1\t1")
print("FIFO_PORT_REFERENCE\t" + "\t".join(sorted(written_events)))
