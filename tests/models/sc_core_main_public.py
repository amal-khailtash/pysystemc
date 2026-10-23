"""Exercise Python, public-wrapper and native sc_main entry contracts."""

import gc
import sys

from amal.eda.systemc import sc_main as direct_main
from amal.eda.systemc import sc_elab_and_sim as public_elab
from amal.eda.systemc import sc_core as sc
import cppyy


class EntryModel(sc.sc_module):
    """Record event and timed-thread callbacks during an entry-driven simulation."""

    def __init__(self):
        """Register a worker thread and a method sensitive to its event."""
        name = sc.sc_module_name("main_model")
        super().__init__(name)
        self.event = sc.sc_event("tick")
        self.records = []
        self.sc_method("observe", self.observe)
        self.sensitive(self.event)
        self.dont_initialize()
        self.sc_thread("worker", self.worker)
        del name

    def worker(self):
        """Wait twice and notify the method between waits."""
        sc.wait(1, sc.SC_NS)
        self.records.append(("thread", str(sc.sc_time_stamp())))
        self.event.notify(sc.SC_ZERO_TIME)
        sc.wait(1, sc.SC_NS)
        self.records.append(("thread", str(sc.sc_time_stamp())))

    def observe(self):
        """Record the method callback caused by the worker event."""
        self.records.append(("event", str(sc.sc_time_stamp())))


def run_simulation():
    """Run the bounded callback model and return its records and final time."""
    model = EntryModel()
    sc.sc_start(3, sc.SC_NS)
    expected = [("thread", "1 ns"), ("event", "1 ns"), ("thread", "2 ns")]
    assert model.records == expected, model.records
    return model.records, str(sc.sc_time_stamp())


entry_mode = sys.argv[1]
original_sys_argv = list(sys.argv)


def sc_main(args):
    """Run the event simulation and verify callback arguments for each entry path."""
    expected = [entry_mode, "café", ""]
    assert args == expected, args

    if entry_mode in {"elab", "public_elab"}:
        assert sc.sc_argc() == 3
        original = sc.sc_argv()
        assert original == tuple(expected)
        args[1] = "changed"
        assert sc.sc_argv() == original
        assert sys.argv == original_sys_argv
    elif entry_mode in {"direct", "global"}:
        assert sc.sc_argc() == 0
        try:
            sc.sc_argv()
        except RuntimeError:
            pass
        else:
            raise AssertionError("direct sc_main incorrectly activated entry argv")
        assert sys.argv == original_sys_argv
    else:
        raise AssertionError(f"unexpected entry mode: {entry_mode}")

    records, end_time = run_simulation()
    fields = "|".join("<empty>" if value == "" else value for value in expected)
    record_text = ";".join(f"{kind}:{time}" for kind, time in records)
    print(f"MAIN_PUBLIC_REFERENCE {entry_mode} {fields} {record_text} {end_time}")
    return 7


if entry_mode == "global":
    supplied = ["global", "café", ""]
    status = cppyy.gbl.sc_main(len(supplied), supplied)
    assert status == 7 and supplied == ["global", "café", ""]
elif entry_mode == "direct":
    supplied = ["direct", "café", ""]
    status = direct_main(len(supplied), supplied)
    assert status == 7 and supplied == ["direct", "café", ""]
elif entry_mode == "elab":
    supplied = ["elab", "café", "", "ignored"]
    status = sc.sc_elab_and_sim(3, supplied)
    assert status == 7 and supplied == ["elab", "café", "", "ignored"]
elif entry_mode == "public_elab":
    supplied = ["public_elab", "café", "", "ignored"]
    status = public_elab(3, supplied)
    assert status == 7 and supplied == ["public_elab", "café", "", "ignored"]
else:
    released = []

    class Callback:
        """Track callback object release after native dispatch."""

        def __init__(self, outcome):
            """Store the outcome returned or raised by the callback."""
            self.outcome = outcome

        def __call__(self, _args):
            """Return the configured value or raise the configured exception."""
            if self.outcome == "system_exit":
                raise SystemExit(23)
            return self.outcome

        def __del__(self):
            """Record reference cleanup after the wrapper drops its owned reference."""
            released.append(True)

    def invoke_invalid(callback_state):
        """Install one callback state and verify the native dispatcher failure status."""
        if callback_state == "missing":
            globals().pop("sc_main")
        elif callback_state == "noncallable":
            globals()["sc_main"] = None
        else:
            outcome = {
                "bad_return": object(),
                "int_overflow": 1 << 40,
                "overflow": 1 << 100,
                "system_exit": "system_exit",
            }[callback_state]
            callback = Callback(outcome)
            globals()["sc_main"] = callback
            expected_releases = len(released) + 1
            assert direct_main(1, ["error"]) == -1
            globals()["sc_main"] = None
            del callback
            gc.collect()
            assert len(released) == expected_releases, (callback_state, released)
            return
        assert direct_main(1, ["error"]) == -1

    for invalid_state in ("missing", "noncallable", "bad_return", "int_overflow", "overflow", "system_exit"):
        invoke_invalid(invalid_state)
    assert direct_main(0, []) == -1
    callback = Callback(5)
    globals()["sc_main"] = callback
    for invalid_arguments in ((-1, []), (2, ["only"])):
        try:
            direct_main(*invalid_arguments)
        except ValueError:
            pass
        else:
            raise AssertionError(f"invalid direct sc_main arguments were accepted: {invalid_arguments!r}")
    assert cppyy.gbl.sc_main(-1, cppyy.nullptr) == -1
    assert cppyy.gbl.sc_main(1, cppyy.nullptr) == -1
    expected_releases = len(released) + 1
    assert direct_main(1, ["recovered"]) == 5
    globals()["sc_main"] = None
    del callback
    gc.collect()
    assert len(released) == expected_releases
    print(
        "MAIN_PUBLIC_ERRORS missing noncallable bad_return int_overflow overflow system_exit "
        "invalid_count null_argv cleanup recovery"
    )
