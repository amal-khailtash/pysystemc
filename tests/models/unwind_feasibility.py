"""Experimental reset/kill gate using preserved native unwind ownership.

This isolated probe does not enable reset in the public adapter. Native waits
release the GIL and retain the original scheduler exception on their OS thread.
"""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(r"""
#include <exception>
#include <functional>
namespace unwind_probe {
thread_local std::exception_ptr pending;
bool cycle() {
  try { sc_core::wait(); return true; }
  catch (const sc_core::sc_unwind_exception&) {
    pending = std::current_exception();
    return false;
  }
}
sc_core::sc_process_handle spawn(std::function<void()> callback, const sc_core::sc_spawn_options* options) {
  return sc_core::sc_spawn([callback]() {
    try { callback(); }
    catch (...) {
      if (pending) { auto original = pending; pending = nullptr; std::rethrow_exception(original); }
      throw;
    }
  }, "worker", options);
}
}
""")
cppyy.gbl.unwind_probe.cycle.__release_gil__ = True


class Unwind(BaseException):
    """Unwind Python frames before restoring the native scheduler exception."""


clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
reset = sc.sc_signal[bool]("reset")
options = sc.sc_spawn_options()
options.set_sensitivity(clock.posedge_event())
options.async_reset_signal_is(reset, True)
records = []


def callback():
    """Restart after reset and unwind naturally when the native controller kills it."""
    records.append(str(sc.sc_time_stamp()))
    while True:
        if not cppyy.gbl.unwind_probe.cycle():
            raise Unwind()


handle = cppyy.gbl.unwind_probe.spawn(callback, options)
sc.sc_start(3, sc.SC_NS)
reset.write(True)
sc.sc_start(1, sc.SC_NS)
reset.write(False)
sc.sc_start(1, sc.SC_NS)
print(records, flush=True)
assert records == ["0 s", "3 ns"]
cppyy.cppdef(r"""
namespace unwind_probe {
void kill_from_process(sc_core::sc_process_handle handle) {
  sc_core::sc_spawn([handle]() mutable { handle.kill(); }, "controller");
}
}
""")
cppyy.gbl.unwind_probe.kill_from_process(handle)
sc.sc_start(1, sc.SC_NS)
assert handle.terminated()
sc.sc_stop()
print("UNWIND_PROBE_OK")
