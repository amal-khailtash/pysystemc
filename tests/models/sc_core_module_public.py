"""Exercise guarded public module members against native SystemC behavior."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_module_probe.h"))
cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_process_utilities_probe.h"))


class SixPortModel(sc.sc_module):
    """Provide six native input ports for legacy parent positional binding."""

    def __init__(self, name):
        """Create the six ports while the child module name scope is active."""
        super().__init__(name)
        self.inputs = tuple(sc.sc_in[bool](f"input{index}") for index in range(6))

    def bound_count(self):
        """Return the count of ports with a native interface after elaboration."""
        return sum(port.size() != 0 for port in self.inputs)


class ModuleModel(sc.sc_module):
    """Model dynamic waits, triggers, hierarchy, and lifecycle through sc_module."""

    def __init__(self, name):
        """Create child hierarchy and register methods, threads, and edge thread."""
        super().__init__(name)
        self.seed = sc.sc_event("seed")
        self.event_a = sc.sc_event("event_a")
        self.event_b = sc.sc_event("event_b")
        self.edge_bool = sc.sc_signal[bool]("edge_bool", False)
        self.edge_logic = sc.sc_signal[dt.sc_logic]("edge_logic", dt.sc_logic("0"))
        self.clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port.bind(self.clock)
        self.logic_port = sc.sc_in[dt.sc_logic]("logic_port")
        self.logic_port.bind(self.edge_logic)
        child_name = sc.sc_module_name("child")
        self.child = sc.sc_module(child_name)
        del child_name
        binding_name = sc.sc_module_name("bindings")
        self.bindings = SixPortModel(binding_name)
        del binding_name
        self.bindings(self.edge_bool)
        self.bindings(self.clock_port)
        self.bindings << self.edge_bool
        self.bindings << self.clock_port
        cppyy.gbl.sc_core_module_probe.positional_comma_interface(self.bindings, self.edge_bool)
        cppyy.gbl.sc_core_module_probe.positional_comma_port(self.bindings, self.clock_port)
        self.waits = []
        self.triggers = []
        self.edges = []
        self.lifecycle = []
        self.trigger_step = 0

        self.sc_method("dynamic_trigger", self.dynamic_trigger)
        self.sensitive(self.seed)
        self.dont_initialize()
        self.sc_thread("wait_matrix", self.wait_matrix)
        self.sensitive(self.seed)
        self.sensitivity_hits = 0
        self.sc_method("sensitivity_probe", self.sensitivity_probe)
        self.sensitive(self.seed)
        self.sensitive(self.edge_bool)
        self.sensitive(self.clock_port)
        self.sensitive(self.clock_port.pos())
        self.sensitive_pos << self.edge_bool
        self.sensitive_pos << self.edge_logic
        self.sensitive_pos << self.clock_port
        self.sensitive_pos << self.logic_port
        self.sensitive_neg << self.edge_bool
        self.sensitive_neg << self.edge_logic
        self.sensitive_neg << self.clock_port
        self.sensitive_neg << self.logic_port
        self.sc_cthread("edge_matrix", self.edge_matrix, self.clock_port.pos())
        self.edge_handle = sc.sc_get_last_created_process_handle()
        self.set_stack_size(2 * 1024 * 1024)
        assert cppyy.gbl.process_utilities_probe.stack_probe.size(self.edge_handle) == 2 * 1024 * 1024
        self.sc_thread("event_driver", self.event_driver)

    def sensitivity_probe(self):
        """Count callbacks from native default and edge sensitivity builders."""
        self.sensitivity_hits += 1

    def before_end_of_elaboration(self):
        """Record the native pre-elaboration callback."""
        self.lifecycle.append("before")

    def end_of_elaboration(self):
        """Record the native elaboration callback."""
        self.lifecycle.append("elaboration")

    def start_of_simulation(self):
        """Record the native simulation-start callback."""
        self.lifecycle.append("start")

    def end_of_simulation(self):
        """Record the native simulation-end callback."""
        self.lifecycle.append("end")

    def dynamic_trigger(self):
        """Invoke each exact native next_trigger overload from this method process."""
        if self.trigger_step:
            self.triggers.append(f"{self.trigger_step - 1}@{sc.sc_time_stamp().value()}")
        if self.trigger_step == 12:
            return
        either = self.event_a | self.event_b
        both = self.event_a & self.event_b
        if self.trigger_step == 0:
            self.next_trigger()
        elif self.trigger_step == 1:
            self.next_trigger(self.event_a)
        elif self.trigger_step == 2:
            self.next_trigger(either)
        elif self.trigger_step == 3:
            self.next_trigger(both)
        elif self.trigger_step == 4:
            self.next_trigger(sc.sc_time(1, sc.SC_NS))
        elif self.trigger_step == 5:
            self.next_trigger(1, sc.SC_NS)
        elif self.trigger_step == 6:
            self.next_trigger(sc.sc_time(2, sc.SC_NS), self.event_a)
        elif self.trigger_step == 7:
            self.next_trigger(2, sc.SC_NS, self.event_a)
        elif self.trigger_step == 8:
            self.next_trigger(sc.sc_time(2, sc.SC_NS), either)
        elif self.trigger_step == 9:
            self.next_trigger(2, sc.SC_NS, either)
        elif self.trigger_step == 10:
            self.next_trigger(sc.sc_time(2, sc.SC_NS), both)
        else:
            self.next_trigger(2, sc.SC_NS, both)
        self.trigger_step += 1

    def wait_matrix(self):
        """Invoke all twelve non-clocked wait members and record completion timeouts."""
        calls = (
            lambda: self.wait(),
            lambda: self.wait(self.event_a),
            lambda: self.wait(self.event_a | self.event_b),
            lambda: self.wait(self.event_a & self.event_b),
            lambda: self.wait(sc.sc_time(1, sc.SC_NS)),
            lambda: self.wait(1, sc.SC_NS),
            lambda: self.wait(sc.sc_time(2, sc.SC_NS), self.event_a),
            lambda: self.wait(2, sc.SC_NS, self.event_a),
            lambda: self.wait(sc.sc_time(2, sc.SC_NS), self.event_a | self.event_b),
            lambda: self.wait(2, sc.SC_NS, self.event_a | self.event_b),
            lambda: self.wait(sc.sc_time(2, sc.SC_NS), self.event_a & self.event_b),
            lambda: self.wait(2, sc.SC_NS, self.event_a & self.event_b),
        )
        for index, call in enumerate(calls):
            call()
            self.waits.append(f"{index}@{sc.sc_time_stamp().value()}/{int(self.timed_out())}")

    def edge_matrix(self):
        """Exercise wait(int), stack sizing, edge overloads, and native halt in CTHREAD."""
        self.wait(1)
        self.waits.append(f"12@{sc.sc_time_stamp().value()}/{int(self.timed_out())}")
        self.at_posedge(self.edge_bool)
        self.edges.append(f"bool_pos@{sc.sc_time_stamp().value()}")
        self.at_negedge(self.edge_bool)
        self.edges.append(f"bool_neg@{sc.sc_time_stamp().value()}")
        self.at_posedge(self.edge_logic)
        self.edges.append(f"logic_pos@{sc.sc_time_stamp().value()}")
        self.at_negedge(self.edge_logic)
        self.edges.append(f"logic_neg@{sc.sc_time_stamp().value()}")
        self.halt()

    def event_driver(self):
        """Notify both event lists and toggle bool/logic inputs for bounded waits."""
        self.seed.notify(sc.SC_ZERO_TIME)
        for tick in range(14):
            self.wait(1, sc.SC_NS)
            if tick == 0:
                self.seed.notify(sc.SC_ZERO_TIME)
            self.event_a.notify(sc.SC_ZERO_TIME)
            self.event_b.notify(sc.SC_ZERO_TIME)
            value = (tick // 2) % 2 == 0
            self.edge_bool.write(value)
            self.edge_logic.write(dt.sc_logic("1" if value else "0"))
        self.wait(6, sc.SC_NS)
        sc.sc_stop()


legacy = cppyy.gbl.sc_core_module_probe.LegacyModule("legacy")
legacy_name = str(legacy.name())
del legacy
assert not sc.sc_find_object(legacy_name)
legacy_destructions = cppyy.gbl.sc_core_module_probe.LegacyModule.destructions
module_name = sc.sc_module_name("module_top")
model = ModuleModel(module_name)
assert callable(model.end_module)
assert model.kind() == "sc_module"
assert model.sc_get_curr_simcontext()
children = model.get_child_objects()
assert any(str(child.name()) == "module_top.child" for child in children)
assert str(model.child.name()) == "module_top.child"
unique_first = str(model.gen_unique_name("generated", True))
unique_second = str(model.gen_unique_name("generated", True))
assert unique_first != unique_second
model.end_module()
sc.sc_start()
assert model.bindings.bound_count() == 6
del module_name

print(
    "MODULE_HIERARCHY {} {} {} {} {} {}".format(
        model.name(),
        model.child.name(),
        unique_first,
        unique_second,
        int(bool(model.sc_get_curr_simcontext())),
        int(any(str(child.name()) == "module_top.child" for child in children)),
    )
)
print("MODULE_STACK 2097152")
print(
    "MODULE_BINDINGS {} {}".format(
        model.bindings.bound_count(), "".join("1" if p.read() else "0" for p in model.bindings.inputs)
    )
)
print(f"MODULE_SENSITIVITY {model.sensitivity_hits}")
print(f"MODULE_DESTRUCTOR {legacy_name} 1 {legacy_destructions}")
for item in model.waits:
    print("MODULE_WAIT " + item)
for item in model.triggers:
    print("MODULE_TRIGGER " + item)
for item in model.edges:
    print("MODULE_EDGE " + item)
print("MODULE_LIFECYCLE " + " ".join(model.lifecycle))
print("MODULE_CONTEXT 1")
