"""Exercise exact enum types and modeled native module/clock/queue aliases."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_small_types_probe.h"))
ENUMS = {
    "sc_curr_proc_kind": ("SC_NO_PROC_", "SC_METHOD_PROC_", "SC_THREAD_PROC_", "SC_CTHREAD_PROC_"),
    "sc_descendant_inclusion_info": ("SC_NO_DESCENDANTS", "SC_INCLUDE_DESCENDANTS", "SC_INVALID_DESCENDANTS"),
    "sc_port_policy": ("SC_ONE_OR_MORE_BOUND", "SC_ZERO_OR_MORE_BOUND", "SC_ALL_BOUND"),
    "sc_severity": ("SC_INFO", "SC_WARNING", "SC_ERROR", "SC_FATAL", "SC_MAX_SEVERITY"),
    "sc_starvation_policy": ("SC_EXIT_ON_STARVATION", "SC_RUN_TO_TIME"),
    "sc_stop_mode": ("SC_STOP_FINISH_DELTA", "SC_STOP_IMMEDIATE"),
    "sc_time_unit": ("SC_FS", "SC_PS", "SC_NS", "SC_US", "SC_MS", "SC_SEC"),
    "sc_verbosity": ("SC_NONE", "SC_LOW", "SC_MEDIUM", "SC_HIGH", "SC_FULL", "SC_DEBUG"),
    "sc_writer_policy": ("SC_ONE_WRITER", "SC_MANY_WRITERS", "SC_UNCHECKED_WRITERS"),
}
for name, members in ENUMS.items():
    enum = getattr(sc, name)
    assert enum.__cpp_name__ == "sc_core::" + name
    values = []
    for member in members:
        value = enum(getattr(sc, member))
        result = cppyy.gbl.small_types_probe.enum_value[enum](value)
        assert result == int(getattr(sc, member))
        values.append(str(result))
    print("SMALL_TYPE", name, ",".join(values))
actions = sc.sc_actions(sc.SC_DISPLAY | sc.SC_CACHE_REPORT)
assert cppyy.gbl.small_types_probe.actions(actions) == int(sc.SC_DISPLAY | sc.SC_CACHE_REPORT)
print("SMALL_TYPE sc_actions", int(actions))
assert sc.sc_in_clk is sc.sc_in[bool]
assert sc.sc_inout_clk is sc.sc_inout[bool]
assert sc.sc_out_clk is sc.sc_out[bool]
assert sc.sc_channel is sc.sc_module
assert sc.sc_behavior is cppyy.gbl.sc_core.sc_module


class AliasModel(sc.sc_channel):
    """Model native clock-port and event-queue aliases using a channel module."""

    def __init__(self):
        """Bind aliases to live channels and register meaningful callbacks."""
        name = sc.sc_module_name("aliases")
        super().__init__(name)
        self.clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
        self.input = sc.sc_in_clk("input")
        self.input.bind(self.clock)
        self.io_signal = sc.sc_signal[bool]("io_signal")
        self.out_signal = sc.sc_signal[bool]("out_signal")
        self.io = sc.sc_inout_clk("io")
        self.io.bind(self.io_signal)
        self.output = sc.sc_out_clk("output")
        self.output.bind(self.out_signal)
        self.queue = sc.sc_event_queue("queue")
        self.queue_port = sc.sc_event_queue_port("queue_port")
        self.queue_port.bind(self.queue)
        self.edges = []
        self.events = []
        self.sc_method("edge", self.edge)
        self.sensitive(self.input.pos())
        self.dont_initialize()
        self.sc_method("queued", self.queued)
        self.sensitive(self.queue_port.default_event())
        self.dont_initialize()
        self.sc_thread("writer", self.writer)
        del name

    def edge(self):
        """Record each positive clock edge through the clock-input alias."""
        self.edges.append(sc.sc_time_stamp().value())

    def queued(self):
        """Record queued notifications received through the native queue port."""
        self.events.append(sc.sc_time_stamp().value())

    def writer(self):
        """Write boolean output aliases and schedule distinct queue events."""
        self.queue_port.notify(1, sc.SC_NS)
        self.queue_port.notify(3, sc.SC_NS)
        sc.wait(1, sc.SC_NS)
        self.io.write(True)
        self.output.write(True)
        sc.wait(2, sc.SC_NS)
        self.io.write(False)
        self.output.write(False)


model = AliasModel()
assert cppyy.gbl.small_types_probe.behavior_name(model) == "aliases"
assert cppyy.gbl.small_types_probe.channel_name(model) == "aliases"
sc.sc_start(5, sc.SC_NS)
assert model.edges == [0, 2000, 4000], model.edges
assert model.events == [1000, 3000], model.events
assert not model.io.read() and not model.output.read()
print("SMALL_ALIAS aliases edges=0,2000,4000 queue=1000,3000 io=0 out=0 end=5000")
