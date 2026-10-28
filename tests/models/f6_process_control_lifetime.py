"""Check module process control, callback retention, and safe teardown."""

import gc
import weakref

from amal.eda.systemc import sc_core as sc


class ProcessModel(sc.sc_module):
    """Own event, clock, and input-port sources for registered processes."""

    def __init__(self):
        """Create native children before process registration."""
        name = sc.sc_module_name("f6_process_control")
        super().__init__(name)
        self.event = sc.sc_event("f6_method_event")
        self.clock = sc.sc_clock("f6_process_clock", sc.sc_time(2, sc.SC_NS))
        self.clock_port = sc.sc_in[bool]("f6_process_clock_port")
        self.clock_port.bind(self.clock)


class CallbackOwner:
    """Hold the module in a callback cycle until safe native teardown."""

    def __init__(self, module):
        """Store process observations and native process handles."""
        self.module = module
        self.starts = {"thread": [], "cthread": []}
        self.finally_counts = {"thread": [], "cthread": []}
        self.method_calls = []
        self.handles = {}

    def method(self):
        """Record method dispatch without waiting from SC_METHOD."""
        self.method_calls.append(str(sc.sc_time_stamp()))

    def thread(self):
        """Block a module thread until controller reset or kill unwinds it."""
        round_number = len(self.starts["thread"]) + 1
        self.starts["thread"].append((round_number, str(sc.sc_time_stamp())))
        try:
            sc.wait(20, sc.SC_NS)
            raise AssertionError("reset/kill thread wait returned without control")
        finally:
            self.finally_counts["thread"].append((round_number, str(sc.sc_time_stamp())))

    def cthread(self):
        """Block a module clocked thread until controller reset or kill."""
        round_number = len(self.starts["cthread"]) + 1
        self.starts["cthread"].append((round_number, str(sc.sc_time_stamp())))
        try:
            sc.wait(100)
            raise AssertionError("reset/kill clocked wait returned without control")
        finally:
            self.finally_counts["cthread"].append((round_number, str(sc.sc_time_stamp())))

    def controller(self):
        """Control each process only between complete callback invocations."""
        method_event = self.module.event
        method_handle = self.handles["method"]
        method_event.notify(sc.SC_ZERO_TIME)
        self._wait_for_method_call(1)

        # SC_METHOD never calls wait; reset and kill occur between returns.
        method_handle.reset()
        for _ in range(8):
            if len(self.method_calls) >= 2:
                break
            method_event.notify(sc.SC_ZERO_TIME)
            sc.wait(sc.SC_ZERO_TIME)
        assert len(self.method_calls) >= 2, self.method_calls
        before_kill = len(self.method_calls)
        method_handle.kill()
        method_event.notify(sc.SC_ZERO_TIME)
        sc.wait(sc.SC_ZERO_TIME)
        assert len(self.method_calls) == before_kill, self.method_calls
        assert method_handle.terminated()

        self._wait_for_rounds("thread", 1)
        self._wait_for_rounds("cthread", 1)
        sc.wait(1, sc.SC_NS)
        assert str(sc.sc_time_stamp()) == "1 ns"
        self.handles["thread"].reset()
        self.handles["cthread"].reset()
        self._wait_for_rounds("thread", 2)

        sc.wait(1, sc.SC_NS)
        while len(self.starts["cthread"]) < 2:
            sc.wait(sc.SC_ZERO_TIME)
        assert str(sc.sc_time_stamp()) == "2 ns"
        self.handles["thread"].kill()
        self.handles["cthread"].kill()
        sc.wait(sc.SC_ZERO_TIME)
        assert all(self.handles[kind].terminated() for kind in ("thread", "cthread"))
        assert self.finally_counts == {
            "thread": [(1, "1 ns"), (2, "2 ns")],
            "cthread": [(1, "1 ns"), (2, "2 ns")],
        }, self.finally_counts
        sc.sc_stop()

    def _wait_for_method_call(self, count):
        """Yield delta cycles until the method returns from its callback."""
        for _ in range(8):
            if len(self.method_calls) >= count:
                return
            sc.wait(sc.SC_ZERO_TIME)
        raise AssertionError(("method callback did not run", count, self.method_calls))

    def _wait_for_rounds(self, kind, count):
        """Yield delta cycles until a registered target invocation starts."""
        for _ in range(8):
            if len(self.starts[kind]) >= count:
                return
            sc.wait(sc.SC_ZERO_TIME)
        raise AssertionError((kind, count, self.starts[kind]))


def register(model, owner):
    """Register one method, one thread, one clocked thread, and controller."""
    callbacks = {}
    for kind in ("method", "thread", "cthread", "controller"):
        callback = getattr(owner, kind)
        callbacks[kind] = weakref.ref(callback)
        if kind == "method":
            model.sc_method("f6_method", callback)
            model.dont_initialize()
            model.sensitive(model.event)
            model.set_stack_size(2 * 1024 * 1024)
        elif kind == "thread":
            model.sc_thread("f6_thread", callback)
        elif kind == "cthread":
            model.sc_cthread("f6_cthread", callback, model.clock_port.pos())
        else:
            model.sc_thread("f6_controller", callback)
        owner.handles[kind] = sc.sc_get_last_created_process_handle()
    assert len(model._process_callbacks) == 4
    return callbacks


def process_control_lifetime():
    """Run process-control cases then collect their stopped native module cycle."""
    model = ProcessModel()
    model_ref = weakref.ref(model)
    owner = CallbackOwner(model)
    owner_ref = weakref.ref(owner)
    callbacks = register(model, owner)
    del owner
    gc.collect()
    assert owner_ref() is not None
    assert all(reference() is not None for reference in callbacks.values())

    sc.sc_start(4, sc.SC_NS)
    retained_owner = owner_ref()
    assert retained_owner is not None
    assert retained_owner.starts == {
        "thread": [(1, "0 s"), (2, "1 ns")],
        "cthread": [(1, "0 s"), (2, "1 ns")],
    }, retained_owner.starts
    assert len(retained_owner.method_calls) >= 2, retained_owner.method_calls
    assert all(handle.terminated() for handle in retained_owner.handles.values())
    assert retained_owner.finally_counts == {
        "thread": [(1, "1 ns"), (2, "2 ns")],
        "cthread": [(1, "1 ns"), (2, "2 ns")],
    }
    assert owner_ref() is retained_owner
    assert all(reference() is not None for reference in callbacks.values())

    handles = retained_owner.handles
    retained_owner.handles = {}
    del handles, retained_owner
    model.clock_port.__destruct__()
    del model.clock_port
    model.clock.__destruct__()
    del model.clock
    model.event.__destruct__()
    del model.event
    model.__destruct__()
    del model
    gc.collect()
    assert owner_ref() is None, "registered bound callbacks kept their owner after module teardown"
    assert model_ref() is None, "callback/module cycle survived safe native teardown"
    assert all(reference() is None for reference in callbacks.values()), callbacks
    print(
        "F6_01_PROCESS_CONTROL_LIFETIME_OK method=reset/kill thread=reset/kill cthread=reset/kill cleanup=exact released=1"
    )


if __name__ == "__main__":
    """Run a complete isolated process-control and safe-teardown simulation."""
    process_control_lifetime()
