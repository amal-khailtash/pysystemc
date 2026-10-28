"""Compare public spawn options and spawned-process behavior with native SystemC."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_spawn_options_probe.h"))

probe = cppyy.gbl.spawn_options_probe
callback_records = []
reset_records = []


class SpawnOptionsModel(sc.sc_module):
    """Keep sensitivity and reset sources alive through their callbacks."""

    def __init__(self):
        """Bind bool ports, exports and reset interfaces before registering processes."""
        name = sc.sc_module_name("spawn_options")
        super().__init__(name)
        self.clock = sc.sc_clock("options_clock", sc.sc_time(2, sc.SC_NS))
        self.event = sc.sc_event("options_event")

        self.port_signal = sc.sc_signal[bool]("options_port_signal")
        self.port = sc.sc_in[bool]("options_port")
        self.port.bind(self.port_signal)

        self.interface_signal = sc.sc_signal[bool]("options_interface_signal")

        bool_if = cppyy.gbl.sc_core.sc_signal_in_if[bool]
        self.export_signal = sc.sc_signal[bool]("options_export_signal")
        self.export = sc.sc_export[bool_if]("options_export")
        self.export.bind(self.export_signal)

        self.finder_signal = sc.sc_signal[bool]("options_finder_signal")
        self.finder_port = sc.sc_in[bool]("options_finder_port")
        self.finder_port.bind(self.finder_signal)

        self.reset_signal = sc.sc_signal[bool]("options_reset_signal")
        self.reset_in_signal = sc.sc_signal[bool]("options_reset_in_signal")
        self.reset_inout_signal = sc.sc_signal[bool]("options_reset_inout_signal")
        self.reset_out_signal = sc.sc_signal[bool]("options_reset_out_signal")
        self.reset_in = sc.sc_in[bool]("options_reset_in")
        self.reset_in.bind(self.reset_in_signal)
        self.reset_inout = sc.sc_inout[bool]("options_reset_inout")
        self.reset_inout.bind(self.reset_inout_signal)
        self.reset_out = sc.sc_out[bool]("options_reset_out")
        self.reset_out.bind(self.reset_out_signal)

        self.options = []
        self.method_options = []
        self._register_sensitivity_callbacks()
        self._register_reset_callbacks()
        self.sc_thread("drive_options", self.drive)
        del name

    def _register_sensitivity_callbacks(self):
        """Register a no-initialize method for each public sensitivity overload."""
        sources = (
            ("event", self.event),
            ("port", self.port),
            ("interface", self.interface_signal),
            ("export", self.export),
            ("finder", self.finder_port.pos()),
        )
        for label, source in sources:
            options = probe.Options()
            options.spawn_method()
            options.dont_initialize()
            options.set_sensitivity(source)
            options.set_stack_size(256 * 1024)
            assert options.is_method()
            assert options.no_initialization()
            assert options.stack_size() == 256 * 1024
            counts = (
                int(options.event_count()),
                int(options.finder_count()),
                int(options.interface_count()),
                int(options.port_count()),
            )
            expected = {
                "event": (1, 0, 0, 0),
                "port": (0, 0, 0, 1),
                "interface": (0, 0, 1, 0),
                "export": (0, 0, 1, 0),
                "finder": (0, 1, 0, 0),
            }[label]
            assert counts == expected, (label, counts, expected)
            self.method_options.append(options)
            sc.sc_spawn(lambda label=label: self.record_callback(label), f"option_{label}", options)

    def _register_reset_callbacks(self):
        """Exercise each sync/async reset overload on its own native process."""
        reset_cases = (
            ("async_in", "async_reset_signal_is", self.reset_in),
            ("async_inout", "async_reset_signal_is", self.reset_inout),
            ("async_out", "async_reset_signal_is", self.reset_out),
            ("async_if", "async_reset_signal_is", self.reset_signal),
            ("sync_in", "reset_signal_is", self.reset_in),
            ("sync_inout", "reset_signal_is", self.reset_inout),
            ("sync_out", "reset_signal_is", self.reset_out),
            ("sync_if", "reset_signal_is", self.reset_signal),
        )
        for label, method, source in reset_cases:
            options = probe.Options()
            asynchronous = method == "async_reset_signal_is"
            if not asynchronous:
                options.set_sensitivity(self.clock.posedge_event())
            getattr(options, method)(source, True)
            assert int(options.reset_count()) == 1, label
            self.options.append(options)
            sc.sc_spawn(
                lambda label=label, clocked=not asynchronous: reset_worker(label, clocked),
                f"option_{label}",
                options,
            )

    def record_callback(self, label):
        """Save the active simulation time for a method sensitivity."""
        callback_records.append((label, int(sc.sc_time_stamp().value())))

    def drive(self):
        """Trigger all five overloads, assert resets, and stop after restart cleanup."""
        sc.wait(1, sc.SC_NS)
        self.port_signal.write(True)
        self.interface_signal.write(True)
        self.export_signal.write(True)
        self.finder_signal.write(True)
        self.event.notify()
        sc.wait(sc.sc_time(500, sc.SC_PS))
        self.reset_signal.write(True)
        self.reset_in_signal.write(True)
        self.reset_inout.write(True)
        self.reset_out.write(True)
        sc.wait(sc.sc_time(2, sc.SC_NS))
        self.reset_signal.write(False)
        self.reset_in_signal.write(False)
        self.reset_inout.write(False)
        self.reset_out.write(False)
        sc.wait(sc.sc_time(1500, sc.SC_PS))
        sc.sc_stop()


def reset_worker(label, clocked):
    """Record process entry and `finally` cleanup when reset unwinds its wait."""
    reset_records.append((label, "start", int(sc.sc_time_stamp().value())))
    try:
        while True:
            sc.wait() if clocked else sc.wait(100, sc.SC_NS)
    finally:
        reset_records.append((label, "cleanup", int(sc.sc_time_stamp().value())))


destroyed_before = int(probe.Options.destroyed)
sentinel = probe.Options()
sentinel.__destruct__()
assert int(probe.Options.destroyed) == destroyed_before + 1
print("OPTION_DESTRUCTOR", int(probe.Options.destroyed) - destroyed_before, sep="\t")

model = SpawnOptionsModel()
sc.sc_start()

expected_callbacks = [(label, 1000) for label in ("event", "export", "finder", "interface", "port")]
assert sorted(callback_records) == expected_callbacks, callback_records
for label, timestamp in sorted(callback_records):
    print("OPTION_CALLBACK", label, timestamp, sep="\t")

expected_reset_records = []
for label in ("async_if", "async_in", "async_inout", "async_out"):
    expected_reset_records.extend(((label, "start", 0), (label, "cleanup", 1500), (label, "start", 1500)))
for label in ("sync_if", "sync_in", "sync_inout", "sync_out"):
    expected_reset_records.extend(((label, "start", 0), (label, "cleanup", 2000), (label, "start", 2000)))
assert sorted(reset_records) == sorted(expected_reset_records), reset_records
for label, action, timestamp in sorted(reset_records):
    print("OPTION_RESET", label, action, timestamp, sep="\t")
assert sc.sc_time_stamp() == sc.sc_time(5, sc.SC_NS)
assert sc.sc_status() == sc.ScStatus.SC_STOPPED
assert len(model.method_options) == 5 and len(model.options) == 8
print("OPTION_FINAL", int(sc.sc_time_stamp().value()), len(callback_records), len(reset_records), sep="\t")
