"""Verify public throw_it restores kill unwind after the target yields."""

from amal.eda.systemc import sc_core as sc
import cppyy
from pathlib import Path

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_user_exception_probe.h"))

records = []
exception = cppyy.gbl.UserProcessException(41)
injector_handle = None


def stamp(label):
    """Record one callback transition in native resolution ticks."""
    records.append(f"{label} {sc.sc_time_stamp().value()}")


def target():
    """Catch the injected value, kill its suspended caller and then yield."""
    try:
        stamp("target_enter")
        sc.wait(10, sc.SC_NS)
    except cppyy.gbl.UserProcessException as error:
        assert error.code == 41
        assert error.what() == "native user exception"
        stamp("target_caught")
        injector_handle.kill()
        stamp("after_kill")
        sc.wait(1, sc.SC_NS)
        stamp("target_done")
    finally:
        stamp("target_cleanup")


target_handle = sc.sc_spawn(target, "throw_unwind_target")


def injector():
    """Call public throw_it and let native kill unwind this callback."""
    try:
        sc.wait(1, sc.SC_NS)
        stamp("injector_before")
        target_handle.throw_it(exception)
        stamp("injector_returned")
    finally:
        stamp("injector_cleanup")


injector_handle = sc.sc_spawn(injector, "throw_unwind_injector")
sc.sc_start(3, sc.SC_NS)
assert target_handle.terminated()
assert injector_handle.terminated()
assert "injector_returned" not in " ".join(records)

for record in records:
    print("THROW_IT_UNWIND", record)
print("THROW_IT_UNWIND_OK")
sc.sc_stop()
