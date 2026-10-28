"""Compare all native module reset overloads with Python cleanup behavior."""

from amal.eda.systemc import sc_core as sc


ROWS = (
    ("sync", "sc_in", "reset_signal_is", "const sc_core::sc_in<bool>&,bool"),
    ("sync", "sc_inout", "reset_signal_is", "const sc_core::sc_inout<bool>&,bool"),
    ("sync", "sc_out", "reset_signal_is", "const sc_core::sc_out<bool>&,bool"),
    ("sync", "sc_signal_in_if", "reset_signal_is", "const sc_core::sc_signal_in_if<bool>&,bool"),
    ("async", "sc_in", "async_reset_signal_is", "const sc_core::sc_in<bool>&,bool"),
    ("async", "sc_inout", "async_reset_signal_is", "const sc_core::sc_inout<bool>&,bool"),
    ("async", "sc_out", "async_reset_signal_is", "const sc_core::sc_out<bool>&,bool"),
    ("async", "sc_signal_in_if", "async_reset_signal_is", "const sc_core::sc_signal_in_if<bool>&,bool"),
)


class ResetModel(sc.sc_module):
    """Run one CTHREAD for each native synchronous/asynchronous reset signature."""

    def __init__(self, name):
        """Create distinct reset sources and bind each exact overload to a CTHREAD."""
        super().__init__(name)
        self.clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port.bind(self.clock)
        self.sources = [sc.sc_signal[bool](f"reset_source_{index}", False) for index in range(len(ROWS))]
        port_types = {
            "sc_in": sc.sc_in[bool],
            "sc_inout": sc.sc_inout[bool],
            "sc_out": sc.sc_out[bool],
        }
        self.targets = []
        for index, (_kind, port_kind, _method, _signature) in enumerate(ROWS):
            target = (
                self.sources[index]
                if port_kind == "sc_signal_in_if"
                else port_types[port_kind](f"reset_target_{index}")
            )
            if port_kind != "sc_signal_in_if":
                target.bind(self.sources[index])
            self.targets.append(target)

        self.starts = [[] for _ in ROWS]
        self.cleanups = [[] for _ in ROWS]
        self.finders = [self.clock_port.pos() for _ in ROWS]
        for index, (kind, _port_kind, method_name, signature) in enumerate(ROWS):

            def callback(row=index):
                """Run this CTHREAD's indexed callback body."""
                self.run_row(row)

            self.sc_cthread(f"reset_worker_{index}", callback, self.finders[index])
            overload = getattr(sc.sc_module, method_name).__overload__(signature)
            overload(self, self.targets[index], True)
            assert kind in ("sync", "async")
        self.sc_thread("reset_driver", self.drive_resets)

    def run_row(self, index):
        """Record each process start and run until a reset unwinds its Python frame."""
        self.starts[index].append(int(sc.sc_time_stamp().value()))
        try:
            while True:
                self.wait()
        finally:
            self.cleanups[index].append(int(sc.sc_time_stamp().value()))

    def drive_resets(self):
        """Pulse each distinct active-high source away from clock edges."""
        self.wait(3, sc.SC_NS)
        for index, source in enumerate(self.sources):
            source.write(True)
            self.wait(2 if index < 4 else 0.5, sc.SC_NS)
            source.write(False)
            if index + 1 < len(self.sources):
                self.wait(2 if index < 4 else 3.5, sc.SC_NS)


module_name = sc.sc_module_name("module_resets")
model = ResetModel(module_name)
del module_name
sc.sc_start(40, sc.SC_NS)

assert len(model.starts) == len(model.cleanups) == len(ROWS) == 8
for index, (kind, port_kind, method_name, signature) in enumerate(ROWS):
    starts = model.starts[index]
    cleanups = model.cleanups[index]
    assert len(starts) == 2, (index, starts)
    assert len(cleanups) == 1, (index, cleanups)
    assert starts[0] == 0, (index, starts)
    assert cleanups[0] > starts[0], (index, starts, cleanups)
    expected_cleanup = (4 + 4 * index if index < 4 else 3 + 4 * index) * 1000
    assert cleanups[0] == expected_cleanup, (index, starts, cleanups)
    print(
        f"MODULE_RESET|{kind}|{port_kind}|{method_name}|{signature}|"
        f"starts={','.join(map(str, starts))}|cleanups={','.join(map(str, cleanups))}"
    )
