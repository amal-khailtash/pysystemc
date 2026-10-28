"""Exercise ordinary and generic SystemC signals across native policies."""

from pathlib import Path
import sys
import traceback

import amal.eda.systemc as systemc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_signals_probe.h"))

sc = systemc.sc_core
dt = systemc.sc_dt
probe = cppyy.gbl.signal_probe
policies = (
    ("one", sc.SC_ONE_WRITER),
    ("many", sc.SC_MANY_WRITERS),
    ("unchecked", sc.SC_UNCHECKED_WRITERS),
)


def value_text(value, type_name):
    """Format one native signal value with the matching C++ stream spelling."""
    if type_name == "bool":
        return "1" if bool(value) else "0"
    if type_name == "logic":
        return str(value.to_char())
    if type_name == "wide":
        return value.to_string()
    return str(value)


def emit_state(case_name, surface, stage, channel, type_name, initial):
    """Assert and report a channel's public signal and event state."""
    policy = int(channel.get_writer_policy())
    current = value_text(channel.read(), type_name)
    new_value = value_text(channel.get_new_value(), type_name)
    initial_value = value_text(initial, type_name)
    assert channel.kind() == "sc_signal"
    assert channel.get_data_ref() == channel.read()
    assert channel.event()
    assert cppyy.addressof(channel.default_event()) == cppyy.addressof(channel.value_changed_event())
    edges = "-|-"
    if type_name in ("bool", "logic"):
        edges = f"{int(bool(channel.posedge()))}|{int(bool(channel.negedge()))}"
        assert cppyy.addressof(channel.posedge_event()) != cppyy.addressof(channel.negedge_event())
    print(
        f"SIGNAL|{case_name}|{surface}|{stage}|{policy}|{initial_value}|{current}|{new_value}|"
        f"{int(bool(channel.event()))}|{edges}"
    )


def exercise_text_and_events(channel, trace, name, type_name, initial):
    """Cover signal_t's data, event, print, dump and deprecated trace methods."""
    assert channel.kind() == "sc_signal"
    assert channel.read() == initial
    assert channel.get_data_ref() == initial
    assert channel.get_new_value() == initial
    assert not channel.event()
    if type_name == "bool" and not bool(initial):
        assert sc.sc_signal_value(channel) is False
        assert bool(channel) is True
    assert cppyy.addressof(channel.default_event()) == cppyy.addressof(channel.value_changed_event())
    stream = cppyy.gbl.std.ostringstream()
    channel.print(stream)
    assert stream.str() == value_text(initial, type_name)
    stream = cppyy.gbl.std.ostringstream()
    channel.dump(stream)
    dump = stream.str()
    assert name in dump
    assert f"value = {value_text(initial, type_name)}" in dump
    assert f"new value = {value_text(initial, type_name)}" in dump
    channel.trace(trace)


class ConstructorSet:
    """Retain the three public signal constructor forms for one value type."""

    def __init__(self, name, value_type, initial):
        """Construct default, named and named-with-initial-value signals."""
        self.name = name
        self.initial = initial
        self.default_value = value_type()
        self.defaulted = sc.sc_signal[value_type]()
        self.named = sc.sc_signal[value_type](f"{name}_named")
        self.initialized = sc.sc_signal[value_type](f"{name}_initialized", initial)

    def verify(self, type_name):
        """Check each constructor's initial state and report its default policy."""
        for form, channel, expected in (
            ("default", self.defaulted, self.default_value),
            ("named", self.named, self.default_value),
            ("initial", self.initialized, self.initial),
        ):
            assert channel.kind() == "sc_signal"
            assert channel.read() == expected
            assert channel.get_writer_policy() == sc.SC_ONE_WRITER
            if form != "default":
                assert f"{self.name}_{'named' if form == 'named' else 'initialized'}" in channel.name()
            print(
                f"SIGNAL_CTOR|{type_name}|{form}|{int(channel.get_writer_policy())}|"
                f"{value_text(channel.read(), type_name)}"
            )


class Model(sc.sc_module):
    """Own public signals and signal_t adapters for one scheduled comparison."""

    def __init__(self):
        """Construct all scalar-policy cases, a 137-bit value and trace file."""
        module_name = sc.sc_module_name("signals_public")
        super().__init__(module_name)
        self.constructor_sets = (
            ConstructorSet("ctor_int", int, 19),
            ConstructorSet("ctor_bool", bool, True),
            ConstructorSet("ctor_logic", dt.sc_logic, dt.sc_logic("1")),
        )
        for constructor_set, type_name in zip(self.constructor_sets, ("int", "bool", "logic"), strict=True):
            constructor_set.verify(type_name)

        generic_signal = sc.sc_signal[int, sc.SC_ONE_WRITER]
        assert generic_signal.base_type.__cpp_name__ == "sc_core::sc_signal_t<int,sc_core::SC_ONE_WRITER>"
        assert generic_signal.if_type.__cpp_name__ == "sc_core::sc_signal_inout_if<int>"
        assert generic_signal.policy_type.__cpp_name__ == "sc_core::sc_writer_policy_check<sc_core::SC_ONE_WRITER>"
        assert generic_signal.this_type.__cpp_name__ == generic_signal.__cpp_name__
        assert generic_signal.value_type.__cpp_name__ == "sc_core::sc_signal<int,sc_core::SC_ONE_WRITER>::value_type"
        self.cases = []
        self.case_types = {}
        for type_name, value_type, initial, first, second in (
            ("int", int, 5, 11, 13),
            ("bool", bool, False, True, False),
            ("logic", dt.sc_logic, dt.sc_logic("0"), dt.sc_logic("1"), dt.sc_logic("0")),
        ):
            selected_policies = policies if type_name != "int" else policies[:1]
            for policy_name, policy in selected_policies:
                name = f"{type_name}_{policy_name}"
                adapter = probe.signal_t_access[value_type, policy](f"{name}_base", initial)
                signal = sc.sc_signal[value_type, policy](name, initial)
                adapter_output = sc.sc_out[value_type](f"{name}_base_output")
                signal_output = sc.sc_out[value_type](f"{name}_output")
                adapter_output.bind(adapter)
                signal_output.bind(signal)
                self.case_types[name] = (value_type, policy)
                self.cases.append(
                    (name, type_name, initial, first, second, adapter, signal, adapter_output, signal_output)
                )

        self.wide_type = dt.sc_biguint[137]
        self.wide_initial = self.wide_type(0)
        self.wide_first = self.wide_type("87112285931760246646623899502532662133001")
        self.wide_second = self.wide_type("87112285931760246646623899502532662133007")
        wide_policy = sc.SC_ONE_WRITER
        self.wide_adapter = probe.signal_t_access[self.wide_type, wide_policy]("wide_base", self.wide_initial)
        self.wide_signal = sc.sc_signal[self.wide_type, wide_policy]("wide_signal", self.wide_initial)
        self.wide_adapter_output = sc.sc_out[self.wide_type]("wide_base_output")
        self.wide_signal_output = sc.sc_out[self.wide_type]("wide_output")
        self.wide_adapter_output.bind(self.wide_adapter)
        self.wide_signal_output.bind(self.wide_signal)
        assert value_text(sc.sc_signal_value(self.wide_adapter), "wide") == "0"
        assert probe.value_conversion_address[self.wide_type, wide_policy](self.wide_adapter) == probe.data_ref_address[
            self.wide_type, wide_policy
        ](self.wide_adapter)

        self.trace = sc.sc_create_vcd_trace_file("signals_public")
        self.trace.set_time_unit(1, sc.SC_NS)
        for name, _type_name, _initial, _first, _second, adapter, signal, _adapter_output, _signal_output in self.cases:
            exercise_text_and_events(adapter, self.trace, f"{name}_base", _type_name, _initial)
            exercise_text_and_events(signal, self.trace, name, _type_name, _initial)
            sc.sc_trace(self.trace, signal, name)
        exercise_text_and_events(self.wide_adapter, self.trace, "wide_base", "wide", self.wide_initial)
        exercise_text_and_events(self.wide_signal, self.trace, "wide_signal", "wide", self.wide_initial)
        sc.sc_trace(self.trace, self.wide_signal, "wide_sample")

        self.errors = []
        self.sc_thread("drive", lambda: self.run_callback(self.drive))

    def run_callback(self, callback):
        """Capture Python callback errors and stop the native kernel cleanly."""
        try:
            callback()
        except BaseException:
            self.errors.append(traceback.format_exc())
            sc.sc_stop()

    def drive(self):
        """Write values, observe delta events and verify falling-edge behavior."""
        for name, type_name, initial, first, _second, adapter, signal, _adapter_output, _signal_output in self.cases:
            adapter.__assign__(first)
            signal.__assign__(first)
            assert value_text(adapter.get_new_value(), type_name) == value_text(first, type_name), (
                name,
                value_text(adapter.get_new_value(), type_name),
                value_text(first, type_name),
            )
            assert value_text(signal.get_new_value(), type_name) == value_text(first, type_name), name
        self.wide_adapter.__assign__(self.wide_first)
        self.wide_signal.__assign__(self.wide_first)
        assert value_text(self.wide_adapter.get_new_value(), "wide") == value_text(self.wide_first, "wide")
        assert value_text(self.wide_signal.get_new_value(), "wide") == value_text(self.wide_first, "wide")
        sc.wait(sc.SC_ZERO_TIME)
        for name, type_name, initial, _first, _second, adapter, signal, _adapter_output, _signal_output in self.cases:
            value_type, policy = self.case_types[name]
            emit_state(name, "signal_t", "rise", adapter, type_name, initial)
            emit_state(name, "signal", "rise", signal, type_name, initial)
            assert value_text(sc.sc_signal_value(adapter), type_name) == value_text(adapter.read(), type_name)
            assert probe.value_conversion_address[value_type, policy](adapter) == probe.data_ref_address[
                value_type, policy
            ](adapter)
            assert value_text(sc.sc_signal_value(signal), type_name) == value_text(signal.read(), type_name)
            assert probe.value_conversion_address[value_type, policy](signal) == probe.data_ref_address[
                value_type, policy
            ](signal)
        assert value_text(sc.sc_signal_value(self.wide_adapter), "wide") == value_text(self.wide_first, "wide")
        assert value_text(sc.sc_signal_value(self.wide_signal), "wide") == value_text(self.wide_first, "wide")
        emit_state("wide_signal", "signal_t", "rise", self.wide_adapter, "wide", self.wide_initial)
        emit_state("wide_signal", "signal", "rise", self.wide_signal, "wide", self.wide_initial)

        for name, type_name, _initial, first, _second, adapter, signal, _adapter_output, _signal_output in self.cases:
            value_type, _policy = self.case_types[name]
            adapter.__assign__(adapter)
            signal.__assign__(signal)
            adapter.__assign__(probe.input_view[value_type](signal))
            signal.__assign__(probe.input_view[value_type](adapter))
            assert value_text(adapter.get_new_value(), type_name) == value_text(first, type_name), name
            assert value_text(signal.get_new_value(), type_name) == value_text(first, type_name), name
        sc.wait(sc.SC_ZERO_TIME)
        for name, type_name, _initial, first, _second, adapter, signal, _adapter_output, _signal_output in self.cases:
            assert value_text(adapter.read(), type_name) == value_text(first, type_name), name
            assert value_text(signal.read(), type_name) == value_text(first, type_name), name
            assert not adapter.event() and not signal.event()
        print("SIGNAL_ASSIGN|interface|self|seven-pairs-stable")

        sc.wait(1, sc.SC_NS)
        for name, _type_name, _initial, _first, second, adapter, _signal, _adapter_output, signal_output in self.cases:
            adapter.write(second)
            signal_output.write(second)
        self.wide_adapter_output.write(self.wide_second)
        self.wide_signal_output.write(self.wide_second)
        sc.wait(sc.SC_ZERO_TIME)
        for name, type_name, initial, _first, _second, adapter, signal, _adapter_output, _signal_output in self.cases:
            emit_state(name, "signal_t", "fall", adapter, type_name, initial)
            emit_state(name, "signal", "fall", signal, type_name, initial)
        emit_state("wide_signal", "signal_t", "fall", self.wide_adapter, "wide", self.wide_initial)
        emit_state("wide_signal", "signal", "fall", self.wide_signal, "wide", self.wide_initial)
        print(f"SIGNAL_TIME|{sc.sc_time_stamp()}|{sc.sc_delta_count()}")
        sc.wait(sc.SC_ZERO_TIME)
        sc.wait(1, sc.SC_NS)


class PortPolicyModule(sc.sc_module):
    """Bind duplicate output ports to test a signal writer policy."""

    def __init__(self, policy):
        """Bind two native output ports to one bool signal."""
        module_name = sc.sc_module_name("signal_port_policy")
        super().__init__(module_name)
        self.signal = sc.sc_signal[bool, policy]("policy_signal")
        self.first = sc.sc_out[bool]("first_output")
        self.second = sc.sc_out[bool]("second_output")
        self.first.bind(self.signal)
        self.second.bind(self.signal)


if len(sys.argv) > 1 and sys.argv[1].startswith("port-"):
    policy_name = sys.argv[1].removeprefix("port-")
    policy = dict(policies)[policy_name]
    module = PortPolicyModule(policy)
    error_id = 0
    try:
        sc.sc_start(sc.SC_ZERO_TIME)
    except Exception as error:
        details = str(error)
        if hasattr(error, "get_id"):
            error_id = int(error.get_id())
        elif "E115" in details:
            error_id = 115
        else:
            raise
    if policy_name == "one":
        assert error_id == 115, error_id
        print("SIGNAL_PORT|one|duplicate-error|115")
    else:
        assert error_id == 0, error_id
        print(f"SIGNAL_PORT|{policy_name}|multiple-outputs-allowed|0")
else:
    model = Model()
    sc.sc_start(2, sc.SC_NS)
    sc.sc_close_vcd_trace_file(model.trace)
    assert not model.errors, "\n".join(model.errors)
    assert str(sc.sc_time_stamp()) == "2 ns"
