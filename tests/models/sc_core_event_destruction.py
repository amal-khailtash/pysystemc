"""Manual reproducer: native SystemC crashes deleting an event with timed waiters.

Not a passing semantic test. Retain event storage until all waiters finish.
Run only in an isolated subprocess with a timeout.
"""

from amal.eda.systemc import sc_core as sc


class Model(sc.sc_module):
    """Own event storage until the driver destroys it during active waits."""

    def __init__(self, name):
        """Register a timed thread waiter, method waiter and destruction driver."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.event = sc.sc_event("target")
        self.records = []
        self.started = False
        self.sc_thread("thread", self.thread)
        self.sc_method("method", self.method)
        self.sc_thread("driver", self.driver)

    def thread(self):
        """Resume at the original timeout after the event has been destroyed."""
        sc.wait(sc.sc_time(3, sc.SC_NS), self.event)
        self.records.append(("thread", sc.sc_time_stamp().value()))

    def method(self):
        """Use timed dynamic sensitivity and verify one timeout invocation."""
        if not self.started:
            self.started = True
            sc.next_trigger(sc.sc_time(4, sc.SC_NS), self.event)
        else:
            self.records.append(("method", sc.sc_time_stamp().value()))

    def driver(self):
        """Destroy an event with dynamic waiters and pending timed delivery."""
        sc.wait(1, sc.SC_NS)
        self.event.notify(1, sc.SC_NS)
        self.event.__destruct__()
        assert not sc.sc_find_event("destroy.target")
        sc.wait(4, sc.SC_NS)
        sc.sc_stop()


model = Model("destroy")
sc.sc_start()
assert model.records == [("thread", 3000), ("method", 4000)], model.records
print("EVENT_DESTRUCTION_REFERENCE", *[f"{kind}:{time}" for kind, time in model.records])
