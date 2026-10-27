"""Exercise pre-import cached native waits and inherited native blocking methods."""

import builtins
import os
from pathlib import Path
import sys


original_import = builtins.__import__
raw_wait = None
raw_overloads = []


def capture_native_import(name, globals=None, locals=None, fromlist=(), level=0):
    """Capture native bindings during ordinary startup, before guards are installed."""
    global raw_wait
    if level == 1 and name == "sc_core" and globals and globals.get("__name__") == "amal.eda.systemc":
        cppyy = sys.modules["cppyy"]
        raw_wait = cppyy.gbl.sc_core.wait
        cppyy.include(str(Path(__file__).resolve().parents[1] / "native/native_blocking_calls.h"))
        for signature in (
            "sc_core::sc_simcontext*",
            "int, sc_core::sc_simcontext*",
            "const sc_core::sc_time&, sc_core::sc_simcontext*",
            "double, sc_core::sc_time_unit, sc_core::sc_simcontext*",
            "const sc_core::sc_event&, sc_core::sc_simcontext*",
            "const sc_core::sc_event_or_list&, sc_core::sc_simcontext*",
            "const sc_core::sc_event_and_list&, sc_core::sc_simcontext*",
            "const sc_core::sc_time&, const sc_core::sc_event&, sc_core::sc_simcontext*",
            "const sc_core::sc_time&, const sc_core::sc_event_or_list&, sc_core::sc_simcontext*",
            "const sc_core::sc_time&, const sc_core::sc_event_and_list&, sc_core::sc_simcontext*",
            "double, sc_core::sc_time_unit, const sc_core::sc_event&, sc_core::sc_simcontext*",
            "double, sc_core::sc_time_unit, const sc_core::sc_event_or_list&, sc_core::sc_simcontext*",
            "double, sc_core::sc_time_unit, const sc_core::sc_event_and_list&, sc_core::sc_simcontext*",
        ):
            raw_overloads.append(raw_wait.__overload__(signature))
    return original_import(name, globals, locals, fromlist, level)


builtins.__import__ = capture_native_import
try:
    from amal.eda.systemc import sc_core as sc
finally:
    builtins.__import__ = original_import

import cppyy

assert raw_wait is not None and raw_wait is not cppyy.gbl.sc_core.wait
assert len(raw_overloads) == 13
for invalid in (raw_wait, *raw_overloads, lambda: None, cppyy.bind_object(0, cppyy.gbl.blocking_fixture.CachedWait)):
    try:
        sc.native_blocking_call(invalid)
    except TypeError as error:
        assert "native C++ callable" in str(error)
    else:
        raise AssertionError("raw/Python callable accepted")

family, mode = sys.argv[1:]
fixture = cppyy.gbl.blocking_fixture
if family == "rejections":
    for invalid_type in (
        fixture.PointerOperation,
        fixture.ReferenceOperation,
        fixture.MoveOnlyOperation,
        fixture.NonConstOperation,
    ):
        try:
            sc.native_blocking_call(invalid_type())
        except TypeError:
            pass
        else:
            raise AssertionError("invalid native return contract accepted")
    assert fixture.rejected_calls == 0
    print("NATIVE_BLOCKING_REJECTIONS_OK 4")
    sys.exit(0)
if family == "raw":
    raw_overloads[2].__release_gil__ = True

    def fatal_boundary(report, actions):
        """Exit a destructive probe after observing actual native fatal unwind loss."""
        if report.get_id() == 539:
            print("RAW_CPPYY_FATAL F539", report.get_msg_type(), flush=True)
            os._exit(86)
        sc.sc_report_handler.default_handler(report, actions)

    sc.set_report_handler(fatal_boundary)
first, second = sc.sc_event("first"), sc.sc_event("second")
name = sc.sc_module_name("top")
top = sc.sc_module(name)
channel = fixture.ForeignChannel("foreign")
output = cppyy.gbl.std.vector["int"](1, -17)
records, starts, cleanups = [], {}, {}
normal = mode in ("normal", "error")
kinds = range(13) if family == "cached" else range(1)
operations = [
    fixture.CachedWait(kind, normal, first, second, sc.sc_get_curr_simcontext())
    if family == "cached"
    else fixture.CopyOnlyOperation(channel)
    if family == "copy_only"
    else fixture.ForeignOperation(channel, output, mode == "error")
    for kind in kinds
]


def worker(kind):
    """Retain native work and verify value/error results or exactly-once unwind cleanup."""
    starts[kind] = starts.get(kind, 0) + 1
    try:
        if family == "raw":
            raw_overloads[2](sc.sc_time(100, sc.SC_NS), sc.sc_get_curr_simcontext())
        else:
            try:
                result = sc.native_blocking_call(operations[kind])
            except cppyy.gbl.std.runtime_error as error:
                assert mode == "error" and "foreign operation failure" in str(error)
                assert output[0] == -17
                records.append(f"BLOCKING_ERROR {kind} {sc.sc_time_stamp()}")
                channel.ready.notify(sc.SC_ZERO_TIME)
                recovery = fixture.ForeignOperation(channel, output, False)
                assert sc.native_blocking_call(recovery) == 41 and output[0] == 73
                records.append(f"BLOCKING_RECOVERY {kind} {sc.sc_time_stamp()}")
                return
            assert normal, "interrupted operation returned"
            if family == "cached":
                assert result is None
            elif family == "copy_only":
                assert result.value == 73
                operations.clear()
                assert result.value == 73
            else:
                assert result == 41
            if family == "foreign":
                assert output[0] == 73
            records.append(f"BLOCKING_WAIT {kind} {sc.sc_time_stamp()}")
    finally:
        cleanups[kind] = cleanups.get(kind, 0) + 1
        if not normal:
            assert output[0] == -17
            records.append(f"BLOCKING_CLEANUP {kind} {sc.sc_time_stamp()}")
            if family == "raw":
                print("RAW_CPPYY_UNWIND_BYPASS", flush=True)


handles = []
for kind in kinds:
    options = sc.sc_spawn_options()
    options.set_sensitivity(first)
    handles.append(sc.sc_spawn(lambda kind=kind: worker(kind), f"worker_{kind}", options))
del name


def control():
    """Wake native work or reset/kill blocked callers before native event completion."""
    if normal:
        sc.wait(1, sc.SC_NS)
        first.notify(sc.SC_ZERO_TIME)
        channel.ready.notify(sc.SC_ZERO_TIME)
        sc.wait(1, sc.SC_NS)
        first.notify(sc.SC_ZERO_TIME)
        second.notify(sc.SC_ZERO_TIME)
    else:
        sc.wait(2, sc.SC_NS)
        for handle in handles:
            handle.reset()
        sc.wait(1, sc.SC_NS)
        for handle in handles:
            handle.kill()


controller = sc.sc_spawn(control)
sc.sc_start(4, sc.SC_NS)
assert all(handle.terminated() for handle in (*handles, controller))
assert starts == cleanups == dict.fromkeys(kinds, 1 if normal else 2)
if not normal:
    assert sorted(records) == sorted(f"BLOCKING_CLEANUP {kind} {time} ns" for kind in kinds for time in (2, 3))
sc.sc_stop()
for line in sorted(records):
    print(line)
print("NATIVE_BLOCKING_OK", family, mode, len(kinds))
