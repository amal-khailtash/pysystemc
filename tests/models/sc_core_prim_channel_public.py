"""Compare Python primitive-channel callbacks and overloads with native SystemC."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_prim_channel_probe.h"))

probe = cppyy.gbl.prim_channel_public_probe
temporary = probe.Channel()
temporary.__destruct__()
assert int(probe.Channel.destroyed) == 1
print("PRIM_EXPLICIT_DESTRUCTION", int(probe.Channel.destroyed), sep="\t")

default_channel = sc.sc_prim_channel()
assert default_channel.kind() == "sc_prim_channel"
assert default_channel.basename()
print("PRIM_DEFAULT", default_channel.kind(), sep="\t")

wait_records = []
trigger_records = []
lifecycle_records = []


def record(destination, label, timed_out):
    """Append a callback label, exact kernel time, delta and timeout flag."""
    destination.append(f"{label}@{int(sc.sc_time_stamp().value())}/{int(sc.sc_delta_count())}/{int(timed_out)}")


class PythonChannel(sc.sc_prim_channel):
    """Record public primitive-channel lifecycle and update callbacks."""

    def __init__(self, name):
        """Construct a named channel and initialize callback observations."""
        super().__init__(name)
        self.update_count = 0
        self.requested_during_update = False

    def before_end_of_elaboration(self):
        """Record the native pre-elaboration callback."""
        lifecycle_records.append("B")

    def end_of_elaboration(self):
        """Record the native end-of-elaboration callback."""
        lifecycle_records.append("E")

    def start_of_simulation(self):
        """Record the native simulation-start callback."""
        lifecycle_records.append("S")

    def end_of_simulation(self):
        """Record the native simulation-end callback."""
        lifecycle_records.append("F")

    def update(self):
        """Count one scheduled update and observe the coalescing marker."""
        self.update_count += 1
        self.requested_during_update = bool(self.update_requested())


class AsyncPythonChannel(PythonChannel):
    """Use a host thread to request one attached native channel update."""

    def __init__(self, name, ready):
        """Attach as a suspending channel and retain its completion event."""
        super().__init__(name)
        self.ready = ready
        self.attached = bool(self.async_attach_suspending())
        self.detached = False
        self.requester = probe.HostRequester(self)

    def start_of_simulation(self):
        """Start the worker while the native kernel begins simulation."""
        super().start_of_simulation()

    def update(self):
        """Deliver the host request through a kernel-thread callback."""
        super().update()
        self.ready.notify(sc.SC_ZERO_TIME)


class Model(sc.sc_module):
    """Own process callbacks, events and primitive channels for one run."""

    def __init__(self, name):
        """Register all 13 wait and 12 next_trigger overload exercises."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_clock("prim_clock", sc.sc_time(2, sc.SC_NS))
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port.bind(self.clock)
        self.event_a = sc.sc_event("event_a")
        self.event_b = sc.sc_event("event_b")
        self.kick = sc.sc_event("kick")
        self.async_ready = sc.sc_event("async_ready")
        self.or_list = sc.sc_event_or_list(self.event_a)
        self.or_list |= self.event_b
        self.and_list = sc.sc_event_and_list(self.event_a)
        self.and_list &= self.event_b
        self.channel = PythonChannel("prim_probe")
        self.async_channel = AsyncPythonChannel("async_prim_probe", self.async_ready)
        self.async_observed = False
        self.trigger_index = 0
        self.wait_count = 0
        self.trigger_count = 0
        self.async_started = False

        self.sc_thread("wait_0", self.make_waiter(0))
        self.sensitive(self.kick)
        for form in range(1, 12):
            self.sc_thread(f"wait_{form}", self.make_waiter(form))
        self.sc_cthread("wait_12", self.make_waiter(12), self.clock_port.pos())
        self.sc_method("trigger", self.trigger)
        self.sensitive(self.kick)
        self.dont_initialize()
        self.sc_thread("drive", self.drive)
        self.sc_method("async_complete", self.async_complete)
        self.sensitive(self.async_ready)
        self.dont_initialize()

    def make_waiter(self, form):
        """Build one process callback for a protected wait overload."""

        def waiter():
            """Call the selected channel overload and record its resumption."""
            if form == 0:
                self.channel.wait()
            elif form == 1:
                self.channel.wait(self.event_a)
            elif form == 2:
                self.channel.wait(self.or_list)
            elif form == 3:
                self.channel.wait(self.and_list)
            elif form == 4:
                self.channel.wait(sc.sc_time(1, sc.SC_NS))
            elif form == 5:
                self.channel.wait(1.0, sc.SC_NS)
            elif form == 6:
                self.channel.wait(sc.sc_time(10, sc.SC_NS), self.event_a)
            elif form == 7:
                self.channel.wait(10.0, sc.SC_NS, self.event_a)
            elif form == 8:
                self.channel.wait(sc.sc_time(10, sc.SC_NS), self.or_list)
            elif form == 9:
                self.channel.wait(10.0, sc.SC_NS, self.or_list)
            elif form == 10:
                self.channel.wait(sc.sc_time(10, sc.SC_NS), self.and_list)
            elif form == 11:
                self.channel.wait(10.0, sc.SC_NS, self.and_list)
            else:
                self.channel.wait(2)
            record(wait_records, str(form), self.channel.timed_out())
            self.wait_count += 1
            self.maybe_start_async_request()

        return waiter

    def trigger(self):
        """Invoke each next_trigger overload on consecutive callbacks."""
        if self.trigger_index:
            record(trigger_records, str(self.trigger_index - 1), self.channel.timed_out())
            self.trigger_count += 1
        form = self.trigger_index
        if form == 12:
            self.maybe_start_async_request()
            return
        if form == 0:
            self.channel.next_trigger()
        elif form == 1:
            self.channel.next_trigger(self.event_a)
        elif form == 2:
            self.channel.next_trigger(self.or_list)
        elif form == 3:
            self.channel.next_trigger(self.and_list)
        elif form == 4:
            self.channel.next_trigger(sc.sc_time(1, sc.SC_NS))
        elif form == 5:
            self.channel.next_trigger(1.0, sc.SC_NS)
        elif form == 6:
            self.channel.next_trigger(sc.sc_time(10, sc.SC_NS), self.event_a)
        elif form == 7:
            self.channel.next_trigger(10.0, sc.SC_NS, self.event_a)
        elif form == 8:
            self.channel.next_trigger(sc.sc_time(10, sc.SC_NS), self.or_list)
        elif form == 9:
            self.channel.next_trigger(10.0, sc.SC_NS, self.or_list)
        elif form == 10:
            self.channel.next_trigger(sc.sc_time(10, sc.SC_NS), self.and_list)
        elif form == 11:
            self.channel.next_trigger(10.0, sc.SC_NS, self.and_list)
        self.trigger_index += 1

    def maybe_start_async_request(self):
        """Start the native host worker after every scheduling probe returns."""
        if not self.async_started and self.wait_count == 13 and self.trigger_count == 12:
            self.async_started = True
            self.async_channel.requester.start()

    def drive(self):
        """Coalesce updates and pulse trigger and event sensitivities."""
        self.channel.request_update()
        self.channel.request_update()
        self.channel.request_update()
        for tick in range(1, 17):
            sc.wait(1, sc.SC_NS)
            self.kick.notify(sc.SC_ZERO_TIME)
            if tick % 2 == 0:
                self.event_a.notify(sc.SC_ZERO_TIME)
            if tick % 3 == 0:
                self.event_b.notify(sc.SC_ZERO_TIME)

    def async_complete(self):
        """Detach the async channel once its update event reaches the kernel."""
        self.async_observed = True
        self.async_channel.detached = bool(self.async_channel.async_detach_suspending())
        sc.sc_stop()


model = Model("prim_channel_model")
sc.sc_start()
model.async_channel.requester.join()

for item in sorted(wait_records):
    print("PRIM_WAIT", item, sep="\t")
for item in sorted(trigger_records):
    print("PRIM_TRIGGER", item, sep="\t")
print(
    "PRIM_UPDATE",
    model.channel.update_count,
    int(model.channel.requested_during_update),
    int(model.channel.update_requested()),
    sep="\t",
)
print(
    "PRIM_ASYNC",
    int(model.async_channel.attached),
    int(model.async_observed),
    int(model.async_channel.detached),
    model.async_channel.update_count,
    sep="\t",
)
print("PRIM_LIFECYCLE", "".join(lifecycle_records), sep="\t")
