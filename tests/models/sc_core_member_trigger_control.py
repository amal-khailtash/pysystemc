"""Compare cached member method registration reset/kill with native SystemC."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(r"""

#include <systemc>
#include <vector>
namespace member_trigger_control
{
/**
 * @brief Native method reset/kill reference with a retained event target.
 * @details SystemC owns the processes for the module lifetime. Callbacks stay
 * in C++; no Python references or GIL acquisition are required. Native reports
 * and scheduler exceptions propagate through SystemC process control.
 */
struct Reference : sc_core::sc_module
{
  SC_HAS_PROCESS(Reference);
  sc_core::sc_event event;
  sc_core::sc_process_handle method;
  std::vector<unsigned long long> times;
  /** Register the method and an independent native controller thread. */
  explicit Reference(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
    SC_METHOD(tick);
    method = sc_core::sc_get_last_created_process_handle();
    SC_THREAD(control);
  }
  /** Register the same event signature on initial and reset callbacks. */
  void tick()
  {
    times.push_back(sc_core::sc_time_stamp().value());
    next_trigger(event);
  }
  /** Reset a dormant registered method, kill it, then notify its event. */
  void control()
  {
    wait(2, sc_core::SC_NS);
    method.reset();
    wait(1, sc_core::SC_NS);
    method.kill();
    wait(1, sc_core::SC_NS);
    event.notify(sc_core::SC_ZERO_TIME);
    wait(1, sc_core::SC_NS);
  }
};
}  // namespace member_trigger_control
""")


class Model(sc.sc_module):
    """Retain a Python method, controller, event and handles through completion."""

    def __init__(self, name):
        """Register the method before saving its handle and controller thread."""
        super().__init__(name)
        self.event = sc.sc_event("target")
        self.times = []
        self.control_times = []
        self.sc_method("tick", self.tick)
        self.handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("control", self.control)
        self.controller = sc.sc_get_last_created_process_handle()

    def tick(self):
        """Reuse the event specialization after a native method reset."""
        self.times.append(int(sc.sc_time_stamp().value()))
        self.next_trigger(self.event)

    def control(self):
        """Reset at 2 ns, kill at 3 ns and notify at 4 ns without resumption."""
        sc.wait(2, sc.SC_NS)
        self.handle.reset()
        self.control_times.append(int(sc.sc_time_stamp().value()))
        sc.wait(1, sc.SC_NS)
        self.handle.kill()
        self.control_times.append(int(sc.sc_time_stamp().value()))
        sc.wait(1, sc.SC_NS)
        self.event.notify(sc.SC_ZERO_TIME)
        sc.wait(1, sc.SC_NS)


native_name = sc.sc_module_name("native")
native = cppyy.gbl.member_trigger_control.Reference(native_name)
del native_name
python_name = sc.sc_module_name("python")
model = Model(python_name)
del python_name
sc.sc_start()
assert model.times == list(native.times) == [0, 2000]
assert model.control_times == [2000, 3000]
assert model.handle.valid() and model.handle.terminated()
assert native.method.valid() and native.method.terminated()
assert model.controller.valid() and model.controller.terminated()
assert int(sc.sc_time_stamp().value()) == 5000
print("MEMBER_TRIGGER_CONTROL_OK times=0,2000 reset=2000 kill=3000 final=5000")
