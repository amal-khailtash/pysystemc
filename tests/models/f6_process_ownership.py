"""Probe selected process, result, binding and native entry ownership edges."""

import gc
import ctypes
from pathlib import Path
import sys
import threading
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy


class RecordedCallback:
    """Record one registered process dispatch without owning its module."""

    def __init__(self, label, records):
        """Store an external observation list and a stable process label."""
        self.label = label
        self.records = records

    def __call__(self):
        """Append this callback's label when SystemC dispatches it."""
        self.records.append(self.label)


def process_registration():
    """Check callback retention and dispatch for all module process kinds."""
    records = []
    name = sc.sc_module_name("f6_registration")
    model = sc.sc_module(name)
    model.clock = sc.sc_clock("f6_clock", sc.sc_time(1, sc.SC_NS))
    model.clock_port = sc.sc_in[bool]("f6_clock_port")
    model.clock_port.bind(model.clock)
    callbacks = [RecordedCallback(kind, records) for kind in ("method", "thread", "cthread")]
    references = [weakref.ref(callback) for callback in callbacks]
    model.sc_method("method", callbacks[0])
    model.sc_thread("thread", callbacks[1])
    model.sc_cthread("cthread", callbacks[2], model.clock_port.pos())
    del name
    del callbacks
    gc.collect()
    assert all(reference() is not None for reference in references)
    assert len(model._process_callbacks) == 3

    sc.sc_start(2, sc.SC_NS)
    sc.sc_stop()
    gc.collect()
    assert set(records) == {"method", "thread", "cthread"}, records
    assert all(reference() is not None for reference in references)
    model.clock_port.__destruct__()
    del model.clock_port
    model.clock.__destruct__()
    del model.clock
    model.__destruct__()
    del model
    gc.collect()
    assert all(reference() is None for reference in references), "module teardown retained callbacks"
    print("F6_PROCESS_REGISTRATION", "|".join(sorted(records)), "released_after_module=3")


def process_registration_failures():
    """Check failed argument conversion adds no process or retained callback."""
    name = sc.sc_module_name("f6_registration_failure")
    model = sc.sc_module(name)
    calls = []
    model.sc_method("valid", lambda: calls.append("valid"))
    del name
    child_count = len(model.get_child_objects())
    for kind in ("sc_method", "sc_thread", "sc_cthread"):
        callback = RecordedCallback("failed", calls)
        reference = weakref.ref(callback)
        try:
            if kind == "sc_cthread":
                model.sc_cthread("invalid", callback, object())
            else:
                getattr(model, kind)(object(), callback)
        except TypeError:
            pass
        else:
            raise AssertionError(f"invalid {kind} registration succeeded")
        del callback
        gc.collect()
        assert reference() is None, kind
        assert len(model._process_callbacks) == 1, kind
        assert len(model.get_child_objects()) == child_count, kind
    sc.sc_start(sc.SC_ZERO_TIME)
    sc.sc_stop()
    assert calls == ["valid"], calls
    del model
    gc.collect()
    print("F6_PROCESS_REGISTRATION_FAILURES", "method thread cthread", "unchanged=1")


def process_callback_failure(kind):
    """Check a raising registered callback remains owned through stop."""
    name = sc.sc_module_name(f"f6_failure_{kind}")
    model = sc.sc_module(name)
    clock = sc.sc_clock(f"f6_failure_clock_{kind}", sc.sc_time(1, sc.SC_NS))
    port = sc.sc_in[bool](f"f6_failure_port_{kind}")
    port.bind(clock)
    records = []

    class BrokenCallback:
        """Raise a distinct error from the selected native process kind."""

        def __call__(self):
            """Record entry and raise for native report translation."""
            records.append("entered")
            raise ValueError(f"f6 callback failure {kind}")

    callback = BrokenCallback()
    reference = weakref.ref(callback)
    if kind == "method":
        model.sc_method("broken", callback)
    elif kind == "thread":
        model.sc_thread("broken", callback)
    elif kind == "cthread":
        model.sc_cthread("broken", callback, port.pos())
    else:
        raise AssertionError(kind)
    del callback
    gc.collect()
    assert reference() is not None
    try:
        sc.sc_start(1, sc.SC_NS)
    except sc.sc_report as error:
        assert f"f6 callback failure {kind}" in error.get_msg(), error.get_msg()
    else:
        raise AssertionError(f"{kind} callback exception was not translated")
    assert records == ["entered"], records
    assert reference() is not None
    sc.sc_stop()
    print("F6_PROCESS_CALLBACK_FAILURE", kind, "entered=1 retained=1")


def process_reset_kill():
    """Check module thread and clocked-thread finally blocks on reset and kill."""
    name = sc.sc_module_name("f6_process_unwind")
    model = sc.sc_module(name)
    model.clock = sc.sc_clock("f6_unwind_clock", sc.sc_time(1, sc.SC_NS))
    model.port = sc.sc_in[bool]("f6_unwind_port")
    model.port.bind(model.clock)
    handles = {}
    entered = []
    finalized = []

    def reset_target():
        """Wait until reset interrupts this module-registered thread."""
        handles["thread"] = sc.sc_get_current_process_handle()
        entered.append("thread")
        try:
            sc.wait(10, sc.SC_NS)
        finally:
            finalized.append("thread")

    def kill_target():
        """Wait until kill interrupts this module-registered clocked thread."""
        handles["cthread"] = sc.sc_get_current_process_handle()
        entered.append("cthread")
        try:
            sc.wait(100)
        finally:
            finalized.append("cthread")

    class Controller:
        """Issue reset and kill only after both targets have started."""

        def __call__(self):
            """Interrupt the suspended native process callbacks in order."""
            sc.wait(250, sc.SC_PS)
            handles["thread"].reset()
            handles["cthread"].kill()
            sc.wait(250, sc.SC_PS)
            sc.sc_stop()

    reset_callback = reset_target
    kill_callback = kill_target
    control_callback = Controller()
    references = [weakref.ref(value) for value in (reset_callback, kill_callback, control_callback)]
    model.sc_thread("reset_target", reset_callback)
    model.sc_cthread("kill_target", kill_callback, model.port.pos())
    sc.sc_spawn(control_callback, "control")
    del reset_callback, kill_callback, control_callback
    gc.collect()
    assert all(reference() is not None for reference in references)
    sc.sc_start(2, sc.SC_NS)
    assert entered.count("thread") == 2, entered
    assert entered.count("cthread") == 1, entered
    assert finalized.count("thread") == 1, finalized
    assert finalized.count("cthread") == 1, finalized
    assert all(reference() is not None for reference in references)
    print("F6_PROCESS_RESET_KILL", "|".join(entered), "cleanup=" + "|".join(finalized))


def typed_spawn_storage():
    """Check representative typed result copies and callback-owner retention."""
    cppyy.cppdef("""
    /** @brief Value used to verify native result storage is copied by value. */
    struct F6SpawnValue { int value = -1; };
    """)
    result = cppyy.gbl.F6SpawnValue()
    source = cppyy.gbl.F6SpawnValue()
    source.value = 41
    scalar_specs = (
        (ctypes.c_bool, True),
        (ctypes.c_byte, -3),
        (ctypes.c_ubyte, 4),
        (ctypes.c_short, -5),
        (ctypes.c_ushort, 6),
        (ctypes.c_int, -7),
        (ctypes.c_uint, 8),
        (ctypes.c_long, -9),
        (ctypes.c_ulong, 10),
        (ctypes.c_longlong, -11),
        (ctypes.c_ulonglong, 12),
        (ctypes.c_float, 1.5),
        (ctypes.c_double, 2.5),
        (ctypes.c_longdouble, 3.5),
    )
    scalar_results = [kind(value) for kind, value in scalar_specs]
    scalar_handles = [
        sc.sc_spawn(storage, lambda storage=storage: storage.value, f"f6_scalar_{index}")
        for index, storage in enumerate(scalar_results)
    ]

    class ReturnValue:
        """Return an arbitrary native value and expose its owner lifetime."""

        def __init__(self, value):
            """Store the value that SystemC copies into caller storage."""
            self.value = value

        def __call__(self):
            """Return a fresh native value proxy for typed assignment."""
            return self.value

    callback = ReturnValue(source)
    reference = weakref.ref(callback)
    handle = sc.sc_spawn(result, callback, "f6_custom_result")
    del callback
    gc.collect()
    assert reference() is not None
    sc.sc_start(1, sc.SC_NS)
    assert all(native_handle.terminated() for native_handle in scalar_handles)
    assert [storage.value for storage in scalar_results] == [value for _, value in scalar_specs]
    assert result.value == 41 and handle.terminated()
    source.value = 97
    assert result.value == 41, "native result storage aliased callback return storage"
    assert reference() is not None, "spawn callbacks are documented through kernel lifetime"
    sc.sc_stop()
    print(
        "F6_TYPED_SPAWN",
        result.value,
        "source=" + str(source.value),
        f"ctypes={len(scalar_results)}",
        "callback_retained=1",
    )


def binding_owners():
    """Check copied binding proxies and bound Python callbacks survive caller GC."""
    cppyy.cppdef("""
    /** @brief Invoke a typed one-argument callable through native code. */
    int f6_invoke_bound(const std::function<int(int)>& callback) { return callback(5); }
    """)
    calls = []

    def python_callback(value):
        """Record the bound argument and return an observable result."""
        calls.append(value)
        return value + 7

    reference = weakref.ref(python_callback)
    typed = cppyy.gbl.std.function["int(int)"](python_callback)
    bound = sc.sc_bind(typed, 12)
    callback_owner = (python_callback,)
    del typed, python_callback
    gc.collect()
    assert reference() is not None
    assert bound() == 19
    assert calls == [12]
    assert cppyy.gbl.f6_invoke_bound(bound) == 19
    assert calls == [12, 12]
    assert reference() is not None
    assert reference() is callback_owner[0]

    name = sc.sc_module_name("f6_bind_hierarchy")
    top = sc.sc_module(name)
    top.signal = sc.sc_signal[int]("signal")
    top.port = sc.sc_port[sc.sc_signal_in_if[int]]("port")
    top.port.bind(top.signal)
    top.export = sc.sc_export[sc.sc_signal_in_if[int]]("export")
    top.export.bind(top.signal)
    port_reference = weakref.ref(top.port)
    signal_reference = weakref.ref(top.signal)
    port_proxy = sc.sc_bind_proxy(top.port)
    interface_proxy = sc.sc_bind_proxy(top.signal)
    assert cppyy.addressof(port_proxy.port) == cppyy.addressof(top.port)
    assert cppyy.addressof(interface_proxy.iface) == cppyy.addressof(top.signal)
    del port_proxy, interface_proxy
    gc.collect()
    assert port_reference() is not None and signal_reference() is not None
    sc.sc_start(sc.SC_ZERO_TIME)
    sc.sc_stop()
    assert reference() is not None
    print("F6_BINDING_OWNERS", "bound_calls=2", "port_interface_live=1", "hierarchy=1")


def entry_bridge(mode):
    """Check bridge references through success, conversion, and unwind paths."""
    header = Path(__file__).parents[1] / "native" / "f6_process_ownership.h"
    cppyy.include(str(header))
    calls = []
    raised_reference = []
    bad_result_reference = []

    class EntryResult(int):
        """Distinguish the bridge-owned result from cached Python integers."""

    class InvalidResult:
        """Provide a weakly observed value that cannot convert to a native int."""

        __slots__ = ("__weakref__",)

    class EntryCallbackError(RuntimeError):
        """Track release of an exception raised by the Python entry callback."""

        def __del__(self):
            """Record final exception release after bridge error display."""
            raised_reference.append("released")

    def entry(arguments):
        """Record dispatch and select the requested return or exception path."""
        calls.append(len(arguments))
        if mode == "callback_exception":
            error = EntryCallbackError("f6 sc_main callback exception")
            raised_reference.append(weakref.ref(error))
            raise error
        if mode == "bad_result":
            result = InvalidResult()
            bad_result_reference.append(weakref.ref(result))
            return result
        return EntryResult(23)

    callback = entry
    reference = weakref.ref(callback)
    globals()["sc_main"] = callback
    failures = {
        "success": 0,
        "list_failure": 1,
        "decode_failure": 2,
        "null_slot": 3,
        "cpp_unwind": 4,
        "callback_exception": 0,
        "bad_result": 0,
    }
    assert mode in failures, mode
    try:
        status = cppyy.gbl.f6_run_python_sc_main_bridge(0, cppyy.nullptr, failures[mode])
        if mode == "success":
            assert status == 23 and calls == [2], (status, calls)
        elif mode == "cpp_unwind":
            assert status == -2 and calls == [], (status, calls)
        elif mode in {"callback_exception", "bad_result"}:
            assert status == -1 and calls == [2], (status, calls)
        else:
            assert status == -1 and calls == [], (status, calls)
        globals().pop("sc_main")
        del callback, entry
        gc.collect()
        count = cppyy.gbl.f6_sc_main_observer_count()
        expected_count = {
            "success": 5,
            "list_failure": 1,
            "decode_failure": 3,
            "null_slot": 3,
            "cpp_unwind": 3,
            "callback_exception": 4,
            "bad_result": 5,
        }[mode]
        assert count == expected_count, (mode, count)
        if mode != "list_failure":
            assert cppyy.gbl.f6_sc_main_observer_refcount(1) == 1
            cppyy.gbl.f6_release_sc_main_observer(1)
        if mode == "bad_result":
            assert cppyy.gbl.f6_sc_main_observer_refcount(4) == 1
            cppyy.gbl.f6_release_sc_main_observer(4)
        references = [
            (index, cppyy.gbl.f6_sc_main_observer_refcount(index))
            for index in range(count)
            if (index != 1 or mode == "list_failure") and (index != 4 or mode != "bad_result")
        ]
        assert all(reference_count == 1 for _, reference_count in references), (mode, references)
    finally:
        globals().pop("sc_main", None)
        cppyy.gbl.f6_release_sc_main_observers()
    gc.collect()
    assert reference() is None
    if mode == "callback_exception":
        assert len(raised_reference) == 2 and raised_reference[0]() is None, raised_reference
        assert raised_reference[1] == "released", raised_reference
    if mode == "bad_result":
        assert bad_result_reference[0]() is None, bad_result_reference
    print("F6_SC_MAIN_BRIDGE", mode, f"status={status}", f"owned_refs={count}", "released=1")


def entry_carrier(mode, entry_kind="runtime"):
    """Check carrier copies release the original Python error under either GIL state."""
    header = Path(__file__).parents[1] / "native" / "f6_process_ownership.h"
    cppyy.include(str(header))
    released = []
    calls = []
    main_thread = threading.get_ident()
    exception_references = []

    class TrackedError(RuntimeError):
        """Mark the thread which decrements the final owned exception reference."""

        def __init__(self, message):
            """Save only a weak observer before this error enters native code."""
            super().__init__(message)
            exception_references.append(weakref.ref(self))

        def __del__(self):
            """Record final Python exception destruction after native transfer."""
            released.append(threading.get_ident())

    def raise_fresh_error():
        """Raise without leaving a local strong reference in the traceback."""
        raise TrackedError("f6 carrier sentinel")

    def report_callback(_report, _actions):
        """Raise the tracked error while native entry translates a C++ throw."""
        calls.append(_report.get_msg())
        if entry_kind == "deprecation" and len(calls) == 1:
            return
        raise_fresh_error()

    sc.set_report_handler(report_callback)
    entry_functions = {
        "runtime": "f6_throw_from_native_entry",
        "direct_report": "f6_report_from_native_entry",
        "caught_report": "f6_native_sc_report_exception",
        "deprecation": "f6_deprecation_summary_entry",
    }
    cppyy.cppdef(f"pysystemc::native_model_entry = &{entry_functions[entry_kind]};")
    operation = cppyy.gbl.f6_consume_entry_exception_carrier
    if mode == "worker":
        operation.__release_gil__ = True
    try:
        caught = operation(1, ["f6-carrier"], mode == "worker")
        assert caught == 1, f"helper did not capture python_entry_callback_error: {caught}"
    finally:
        cppyy.cppdef("pysystemc::native_model_entry = nullptr;")
        sc.set_report_handler()
    del report_callback
    gc.collect()
    assert len(exception_references) == 1
    assert exception_references[0]() is None, "the shared carrier retained the Python exception"
    assert len(released) == 1, released
    assert len(calls) == (2 if entry_kind == "deprecation" else 1), calls
    if mode == "worker":
        assert released[0] != main_thread, released
    else:
        assert released == [main_thread], released
    print("F6_EXCEPTION_CARRIER", mode, "copy_ctor=1", "make_exception_ptr=1", "decref=1", "gil=verified")


def entry_report_identity(entry_kind):
    """Assert report and post-entry callbacks preserve their original exception."""
    released = []
    calls = []

    class TrackedError(RuntimeError):
        """Track destruction after the native entry wrapper restores the error."""

        def __del__(self):
            """Mark final release of the exception instance."""
            released.append(True)

    error = TrackedError(f"f6 {entry_kind} identity")
    reference = weakref.ref(error)
    globals()["f6_report_identity_error"] = error

    def report_callback(report, _actions):
        """Return the first deprecation report and raise the exact error later."""
        calls.append(report.get_msg())
        if entry_kind == "deprecation" and len(calls) == 1:
            return
        raise globals()["f6_report_identity_error"]

    header = Path(__file__).parents[1] / "native" / "f6_process_ownership.h"
    cppyy.include(str(header))
    entry_functions = {
        "caught_report": "f6_native_sc_report_exception",
        "deprecation": "f6_deprecation_summary_entry",
    }
    sc.set_report_handler(report_callback)
    cppyy.cppdef(f"pysystemc::native_model_entry = &{entry_functions[entry_kind]};")
    observed = []
    try:
        try:
            sc.sc_elab_and_sim(1, [f"f6-{entry_kind}"])
        except TrackedError as caught:
            assert caught is error, (id(caught), id(error))
            observed.append(caught)
        else:
            raise AssertionError(f"{entry_kind} callback did not propagate")
    finally:
        cppyy.cppdef("pysystemc::native_model_entry = nullptr;")
        sc.set_report_handler()
        globals().pop("f6_report_identity_error")
    assert len(calls) == (2 if entry_kind == "deprecation" else 1), calls
    observed.clear()
    del error, report_callback
    gc.collect()
    assert reference() is None
    assert released == [True], released
    print("F6_EXCEPTION_IDENTITY", entry_kind, "same_instance=1", "released=1", f"calls={len(calls)}")


def entry_carrier_propagation():
    """Check sc_elab_and_sim restores the same Python exception instance."""
    released = []

    class TrackedError(RuntimeError):
        """Mark release while preserving instance identity at the Python boundary."""

        def __del__(self):
            """Record cleanup after the caller drops the restored exception."""
            released.append(True)

    error = TrackedError("f6 original exception")
    reference = weakref.ref(error)
    globals()["f6_propagated_error"] = error

    def report_callback(_report, _actions):
        """Raise the exact preconstructed exception through native entry."""
        raise globals()["f6_propagated_error"]

    header = Path(__file__).parents[1] / "native" / "f6_process_ownership.h"
    cppyy.include(str(header))
    sc.set_report_handler(report_callback)
    cppyy.cppdef("pysystemc::native_model_entry = &f6_throw_from_native_entry;")
    observed = []
    try:
        try:
            sc.sc_elab_and_sim(1, ["f6-propagation"])
        except TrackedError as caught:
            assert caught is error, (id(caught), id(error))
            observed.append(caught)
        else:
            raise AssertionError("Python report error did not survive native entry")
    finally:
        cppyy.cppdef("pysystemc::native_model_entry = nullptr;")
        sc.set_report_handler()
        globals().pop("f6_propagated_error")
    observed.clear()
    del error, report_callback
    gc.collect()
    assert reference() is None
    assert released == [True], released
    print("F6_EXCEPTION_PROPAGATION", "same_instance=1", "released=1")


def main():
    """Run the requested isolated ownership scenario selected on argv."""
    scenario = sys.argv[1]
    if scenario == "processes":
        process_registration()
    elif scenario == "registration_failures":
        process_registration_failures()
    elif scenario == "process_failure":
        process_callback_failure(sys.argv[2])
    elif scenario == "unwind":
        process_reset_kill()
    elif scenario == "spawn":
        typed_spawn_storage()
    elif scenario == "binding":
        binding_owners()
    elif scenario in {
        "entry_success",
        "entry_list_failure",
        "entry_decode_failure",
        "entry_null_slot",
        "entry_cpp_unwind",
        "entry_callback_exception",
        "entry_bad_result",
    }:
        entry_bridge(scenario.removeprefix("entry_"))
    elif scenario in {"carrier_held", "carrier_worker"}:
        entry_carrier("worker" if scenario == "carrier_worker" else "held")
    elif scenario in {"carrier_direct_report", "carrier_caught_report", "carrier_deprecation"}:
        entry_kind = scenario.removeprefix("carrier_")
        entry_carrier("held", entry_kind)
    elif scenario in {"identity_caught_report", "identity_deprecation"}:
        entry_report_identity(scenario.removeprefix("identity_"))
    elif scenario == "carrier_propagation":
        entry_carrier_propagation()
    else:
        raise AssertionError(f"unknown F6 scenario: {scenario}")


if __name__ == "__main__":
    main()
