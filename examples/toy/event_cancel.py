"""Show timed event cancellation and static method sensitivity.

Run as a separate simulation. The first notification is cancelled; only the
replacement at 3 ns reaches the observer. No process runs during initialization.
"""

from amal.eda.systemc import main, sc_core as sc


class EventDemo(sc.sc_module):
    """Retain an event and observer callback for one finite simulation."""

    def __init__(self, name: str):
        """Register an observer with initialization disabled."""
        super().__init__(name)
        self.event = sc.sc_event("pulse")
        self.observed = []
        self.sc_method("observe", self.observe)
        self.sensitive(self.event)
        self.dont_initialize()
        self.sc_thread("stimulus", self.stimulus)

    def observe(self):
        """Record the exact native notification time."""
        self.observed.append(str(sc.sc_time_stamp()))

    def stimulus(self):
        """Cancel one notification and schedule its replacement."""
        self.event.notify(5, sc.SC_NS)
        sc.wait(1, sc.SC_NS)
        self.event.cancel()
        self.event.notify(2, sc.SC_NS)


def sc_main(args: list[str]) -> int:
    """Run for 6 ns and assert that the cancelled event never fires."""
    model = EventDemo("events")
    sc.sc_start(6, sc.SC_NS)
    assert model.observed == ["3 ns"], model.observed
    print("EVENT_CANCEL_OK: observer ran once at 3 ns")
    return 0


if __name__ == "__main__":
    main()
