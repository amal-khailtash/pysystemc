"""Check native dynamic-process result assignment, callback lifetime and unwind."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy
import ctypes
import sys
import weakref

cppyy.cppdef("""
/** @brief Arbitrary native result type with ordinary value assignment. */
struct SpawnPayload { int value = -7; };
/** @brief Native process functor must suspend without holding the Python GIL. */
struct NativeSpawnFunctor {
  SpawnPayload* result;
  explicit NativeSpawnFunctor(SpawnPayload& value) : result(&value) {}
  void operator()() { sc_core::wait(0.75, sc_core::SC_NS); result->value = 89; }
};
""")

if len(sys.argv) > 1 and sys.argv[1] == "error":
    result = ctypes.c_int(-9)

    def fail():
        """Raise before native result assignment."""
        raise RuntimeError("typed result failure")

    sc.sc_spawn(result, fail, "failed_result")
    try:
        sc.sc_start(1, sc.SC_NS)
    except sc.sc_report as error:
        assert error.get_id() == 549 and "typed result failure" in error.get_msg()
        assert result.value == -9
        print("SPAWN_RESULT_ERROR", error.get_id(), result.value)
    else:
        raise AssertionError("typed callback error did not propagate")
    sys.exit(0)

for arguments in ((None, lambda: 1), (object(), lambda: 1), (ctypes.c_void_p(), lambda: 1), (None,)):
    try:
        sc.sc_spawn(*arguments)
    except TypeError:
        pass
    else:
        raise AssertionError("invalid spawn arguments accepted")

integer = ctypes.c_int(-1)
real = ctypes.c_double(-1)
wide = sc_dt.sc_biguint[137](0)
payload = cppyy.gbl.SpawnPayload()
time_result = sc.sc_time()
method_result = ctypes.c_int(0)
reset_result = ctypes.c_int(-1)
kill_result = ctypes.c_int(-1)
entries = []
cleanup = []
method_visits = []


class RetainedCallback:
    """Keep a weakly observed Python callable until the native process completes."""

    def __call__(self):
        """Wait and return an integer result."""
        sc.wait(1, sc.SC_NS)
        return 23


callback = RetainedCallback()
reference = weakref.ref(callback)
integer_handle = sc.sc_spawn(integer, callback, "integer")
del callback
assert reference() is not None
sc.sc_spawn(real, lambda: 1.25, name="real")
sc.sc_spawn(wide, lambda: sc_dt.sc_biguint[137]("0x10000000000000000000000000000000011"), "wide")


def make_payload():
    """Return an arbitrary user-defined native value by copy."""
    value = cppyy.gbl.SpawnPayload()
    value.value = 71
    return value


sc.sc_spawn(payload, make_payload, "payload")
native_result = cppyy.gbl.SpawnPayload()
native_functor = cppyy.gbl.NativeSpawnFunctor(native_result)
native_handle = sc.sc_spawn(native_functor, "native_functor")
sc.sc_spawn(time_result, lambda: sc.sc_time(3, sc.SC_NS), "time")
tick = sc.sc_event("method_tick")
options = sc.sc_spawn_options()
options.spawn_method()
options.dont_initialize()
options.set_sensitivity(tick)


def method():
    """Assign one new result on each static-sensitive invocation."""
    method_visits.append(sc.sc_time_stamp().value())
    return len(method_visits) * 10


method_handle = sc.sc_spawn(method_result, method, "method", options)


def resetting():
    """Record cleanup on reset and return after its next complete wait."""
    entries.append(sc.sc_time_stamp().value())
    try:
        sc.wait(1, sc.SC_NS)
        return 37
    finally:
        cleanup.append("reset")


def killed():
    """Execute cleanup without assigning an interrupted callback's result."""
    try:
        sc.wait(10, sc.SC_NS)
        return 99
    finally:
        cleanup.append("kill")


reset_handle = sc.sc_spawn(reset_result, resetting, "reset")
kill_handle = sc.sc_spawn(kill_result, killed, "kill")


def controller():
    """Reset and kill live workers, then deliver two method triggers."""
    sc.wait(0.25, sc.SC_NS)
    reset_handle.reset()
    kill_handle.kill()
    assert reset_result.value == kill_result.value == -1
    sc.wait(0.25, sc.SC_NS)
    tick.notify(sc.SC_ZERO_TIME)
    sc.wait(1, sc.SC_NS)
    tick.notify(sc.SC_ZERO_TIME)


sc.sc_spawn(controller, "controller")
sc.sc_start(2, sc.SC_NS)
assert integer.value == 23 and real.value == 1.25
assert wide[136].to_bool() and wide.to_uint64() == 17
assert payload.value == 71 and time_result == sc.sc_time(3, sc.SC_NS)
assert native_result.value == 89 and native_handle.terminated()
assert method_result.value == 20 and method_visits == [500, 1500]
assert entries == [0, 250] and cleanup == ["reset", "kill", "reset"]
assert reset_result.value == 37 and kill_result.value == -1
assert all(handle.terminated() for handle in (integer_handle, reset_handle, kill_handle))
assert not method_handle.terminated() and reference() is not None
print(
    "SPAWN_RESULTS", integer.value, real.value, int(wide[136].to_bool()), wide.to_uint64(), payload.value, time_result
)
print("SPAWN_RESULT_NATIVE", native_result.value)
print("SPAWN_RESULT_METHOD", method_result.value, *method_visits)
print("SPAWN_RESULT_UNWIND", reset_result.value, kill_result.value, *entries, "|".join(cleanup))
sc.sc_stop()
