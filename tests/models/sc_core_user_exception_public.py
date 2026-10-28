"""Verify native user exceptions crossing Python waits and descendant injection."""

from amal.eda.systemc import sc_core as sc
import cppyy
from pathlib import Path
import sys

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_user_exception_probe.h"))
include_children = len(sys.argv) > 1 and sys.argv[1] == "descendants"
kind = sys.argv[2] if len(sys.argv) > 2 else "custom"
exception_type = cppyy.gbl.UserProcessException if kind == "custom" else cppyy.gbl.std.runtime_error
exception = exception_type(41) if kind == "custom" else exception_type("native user exception")
wait = cppyy.gbl.sc_core.wait if len(sys.argv) > 3 and sys.argv[3] == "namespace" else sc.wait
records = []
handles = []


def stamp(label):
    """Record one ordered callback transition in native resolution ticks."""
    records.append(f"{label} {sc.sc_time_stamp().value()}")


def worker(label):
    """Catch an injected native exception, yield again and run ordinary cleanup."""
    stamp(f"enter {label}")
    if label == "parent":
        handles.append(sc.sc_spawn(lambda: worker("child"), "exception_child"))
    try:
        try:
            wait(5, sc.SC_NS)
        except exception_type as error:
            assert "native user exception" in error.what()
            if kind == "custom":
                assert error.code == 41
            stamp(f"caught {label}")
            wait(1, sc.SC_NS)
        stamp(f"done {label}")
    finally:
        stamp(f"cleanup {label}")


parent = sc.sc_spawn(lambda: worker("parent"), "exception_parent")
handles.append(parent)


def controller():
    """Inject with the default or explicit descendant policy and verify yielding."""
    wait(1, sc.SC_NS)
    if include_children:
        parent.throw_it(exception, sc.SC_INCLUDE_DESCENDANTS)
    else:
        parent.throw_it(exception)
    stamp("controller")


sc.sc_spawn(controller, "exception_controller")
sc.sc_start(6, sc.SC_NS)
assert all(handle.terminated() for handle in handles)
assert sum(line.startswith("caught ") for line in records) == (2 if include_children else 1)
for record in records:
    print("USER_EXCEPTION", record)
for invalid in (RuntimeError("python error"), object()):
    try:
        parent.throw_it(invalid)
    except TypeError:
        pass
    else:
        raise AssertionError("Python exception accepted as native value")
print("USER_EXCEPTION_OK", kind, int(include_children))
sc.sc_stop()
