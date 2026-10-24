"""Exercise direct Python overload calls on all public static sensitivity builders."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_sensitivity_probe.h"))
probe = cppyy.gbl.sc_core.SensitivityProbe
records = []


def verify_nested_aliases():
    """Check the six native typedefs exposed by each builder class."""
    aliases = {
        "in_if_b_type": "sc_core::sc_signal_in_if<bool>",
        "in_if_l_type": "sc_core::sc_signal_in_if<sc_dt::sc_logic>",
        "in_port_b_type": "sc_core::sc_in<bool>",
        "in_port_l_type": "sc_core::sc_in<sc_dt::sc_logic>",
        "inout_port_b_type": "sc_core::sc_inout<bool>",
        "inout_port_l_type": "sc_core::sc_inout<sc_dt::sc_logic>",
    }
    for builder_type in (sc.sc_sensitive, sc.sc_sensitive_pos, sc.sc_sensitive_neg):
        for alias, native_name in aliases.items():
            assert getattr(getattr(builder_type, alias), "__cpp_name__") == native_name
    return len(aliases) * 3


print("SENSITIVITY_ALIASES", verify_nested_aliases())


class Model(probe):
    """Own every event source while Python calls native builder overloads."""

    def __init__(self):
        """Create bound bool/logic sources, then register sensitivity methods."""
        module_name = sc.sc_module_name("sensitivity")
        super().__init__(module_name)
        self.signals = {
            "bool": sc.sc_signal[bool]("bool_signal", False),
            "logic": sc.sc_signal[dt.sc_logic]("logic_signal", dt.sc_logic("0")),
        }
        self.ports = {
            kind: sc.sc_in[value_type](f"in_{kind}") for kind, value_type in (("bool", bool), ("logic", dt.sc_logic))
        }
        self.inout_ports = {
            kind: sc.sc_inout[value_type](f"inout_{kind}")
            for kind, value_type in (("bool", bool), ("logic", dt.sc_logic))
        }
        for kind in self.signals:
            self.ports[kind].bind(self.signals[kind])
            self.inout_ports[kind].bind(self.signals[kind])
        self.event = sc.sc_event("event")
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port.bind(clock)
        self.methods = []
        self.handles = []

        # The four ordinary operator() overloads exercise event, interface,
        # port, and event-finder conversion from Python.
        ordinary_call_sources = (
            ("call_event", self.event),
            ("call_interface", self.signals["bool"]),
            ("call_port", self.ports["bool"]),
            ("call_finder", self.ports["bool"].pos()),
        )
        for label, source in ordinary_call_sources:
            self.add_method(label, self.ordinary_builder(), source, call=True)

        # Exercise all ordinary operator<< source types and explicit process
        # selection. Event/interface/port/finder are distinct C++ overloads.
        ordinary_sources = (
            ("shift_event", self.event),
            ("shift_interface", self.signals["logic"]),
            ("shift_port", self.ports["logic"]),
            ("shift_finder", self.ports["logic"].pos()),
        )
        for index, (label, source) in enumerate(ordinary_sources):
            self.add_method(label, self.ordinary_builder(), source, select=index == 0)

        # The legacy positive and negative builders each expose six typed
        # overloads: bool/logic interfaces, input ports, and inout ports.
        for polarity, accessor in (
            ("pos", self.positive_builder),
            ("neg", self.negative_builder),
        ):
            for operator, use_call in (("shift", False), ("call", True)):
                for kind, source_kind in (
                    ("bool_interface", "signal"),
                    ("logic_interface", "signal"),
                    ("bool_input", "input"),
                    ("logic_input", "input"),
                    ("bool_inout", "inout"),
                    ("logic_inout", "inout"),
                ):
                    value_kind = "bool" if kind.startswith("bool") else "logic"
                    source = {
                        "signal": self.signals,
                        "input": self.ports,
                        "inout": self.inout_ports,
                    }[source_kind][value_kind]
                    self.add_method(f"{polarity}_{operator}_{kind}", accessor(), source, call=use_call)

        # Each edge builder has generic process, method, and thread selectors.
        # Keep the returned proxies alive so the process pointers remain valid.
        for polarity, accessor in (("pos", self.positive_builder), ("neg", self.negative_builder)):
            for selector, source in (
                ("process", self.signals["bool"]),
                ("method", self.ports["bool"]),
            ):
                self.add_method(f"{polarity}_{selector}_handle", accessor(), source, selector=selector)
                self.handles.append(sc.sc_get_last_created_process_handle())
            label = f"{polarity}_thread_handle"
            self.sc_thread(label, lambda label=label: record(label))
            self.dont_initialize()
            handle = sc.sc_get_last_created_process_handle()
            self.handles.append(handle)
            accessor() << probe.thread_handle(handle)
            accessor() << self.inout_ports["logic"]
            self.methods.append(label)

        # Each two-argument helper receives its own CTHREAD. The clock's first
        # rising edge is at 4 ns, after all non-clocked source transitions, so
        # a callback at 1 ns proves that the selected helper added sensitivity.
        cthread_sources = (
            ("cthread_finder", self.ports["bool"].pos()),
            ("cthread_bool_interface", self.signals["bool"]),
            ("cthread_logic_interface", self.signals["logic"]),
            ("cthread_bool_input", self.ports["bool"]),
            ("cthread_logic_input", self.ports["logic"]),
            ("cthread_bool_inout", self.inout_ports["bool"]),
            ("cthread_logic_inout", self.inout_ports["logic"]),
        )
        for label, source in cthread_sources:
            self.sc_cthread(label, lambda label=label: record(label), self.clock_port.pos())
            handle = sc.sc_get_last_created_process_handle()
            self.handles.append(handle)
            self.ordinary_builder()(probe.cthread_handle(handle), source)
            self.methods.append(label)

        # Static helpers bypass the builder's current-process mode and take
        # the native process pointer explicitly.
        static_sources = (
            ("static_event", self.event),
            ("static_interface", self.signals["bool"]),
            ("static_port", self.ports["bool"]),
            ("static_finder", self.ports["bool"].pos()),
        )
        for label, source in static_sources:
            self.sc_method(label, lambda label=label: record(label))
            self.dont_initialize()
            handle = sc.sc_get_last_created_process_handle()
            sc.sc_sensitive.make_static_sensitivity(handle.get_process_object(), source)
            self.methods.append(label)

        # reset() clears each builder's selected process. Adding an event after
        # reset must not make the just-registered, dont_initialize method run.
        for label, accessor, source in (
            ("reset_ordinary", self.ordinary_builder, self.event),
            ("reset_pos", self.positive_builder, self.signals["bool"]),
            ("reset_neg", self.negative_builder, self.signals["bool"]),
        ):
            self.sc_method(label, lambda label=label: record(label))
            self.dont_initialize()
            builder = accessor()
            builder.reset()
            builder << source
            self.methods.append(label)

    def add_method(self, label, builder, source, *, call=False, select=False, selector=None):
        """Register a retained callback and attach one source through Python operators."""
        self.sc_method(label, lambda label=label: record(label))
        self.dont_initialize()
        if select or selector:
            handle = sc.sc_get_last_created_process_handle()
            if selector == "method":
                builder << probe.method_handle(handle)
            else:
                builder << handle
        if call:
            builder(source)
        else:
            builder << source
        self.methods.append(label)


def record(label):
    """Record the callback label and native timestamp in picoseconds."""
    records.append((label, int(sc.sc_time_stamp().value())))


clock = sc.sc_clock("clock", sc.sc_time(8, sc.SC_NS), 0.5, sc.sc_time(4, sc.SC_NS))
model = Model()
try:
    model.ordinary_builder() << object()
except TypeError:
    print("SENSITIVITY_INVALID_SOURCE TypeError")
else:
    raise AssertionError("ordinary sensitivity accepted an unrelated Python object")
gc.collect()
sc.sc_start(1, sc.SC_NS)
model.signals["bool"].write(True)
model.signals["logic"].write(dt.sc_logic("1"))
model.event.notify()
sc.sc_start(1, sc.SC_NS)
model.signals["bool"].write(False)
model.signals["logic"].write(dt.sc_logic("0"))
sc.sc_start(1, sc.SC_NS)

expected = {
    "call_event": [1000],
    "call_interface": [1000, 2000],
    "call_port": [1000, 2000],
    "call_finder": [1000],
    "shift_event": [1000],
    "shift_interface": [1000, 2000],
    "shift_port": [1000, 2000],
    "shift_finder": [1000],
    "static_event": [1000],
    "static_interface": [1000, 2000],
    "static_port": [1000, 2000],
    "static_finder": [1000],
}
for operator in ("shift", "call"):
    for kind in ("bool_interface", "logic_interface", "bool_input", "logic_input", "bool_inout", "logic_inout"):
        expected[f"pos_{operator}_{kind}"] = [1000]
        expected[f"neg_{operator}_{kind}"] = [2000]
for polarity, time in (("pos", 1000), ("neg", 2000)):
    for selector in ("process", "method", "thread"):
        expected[f"{polarity}_{selector}_handle"] = [time]
for label in (
    "cthread_finder",
    "cthread_bool_interface",
    "cthread_logic_interface",
    "cthread_bool_input",
    "cthread_logic_input",
    "cthread_bool_inout",
    "cthread_logic_inout",
):
    expected[label] = [1000]
assert {label: sorted(time for found, time in records if found == label) for label in expected} == expected, records
assert not any(label.startswith("reset_") for label, _ in records), records

for label, time in records:
    print(f"SENSITIVITY_CALLBACK {label}@{time}")
print("SENSITIVITY_RESET 1")
print("SENSITIVITY_LIFETIME", len(model.methods), len(records))
sc.sc_stop()
