"""Check writable bind-proxy pointers and default process-monitor dispatch."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(
    r"""
#include <systemc>
namespace fourth_core_pointer_monitor
{
/** @brief Compare borrowed endpoint pointers without retaining their owners.
 * @details The caller owns the proxy and endpoints for this synchronous call.
 * No Python references or callbacks are retained; the calling thread holds GIL.
 */
bool endpoints(const sc_core::sc_bind_proxy& proxy, sc_core::sc_interface* interface, sc_core::sc_port_base* port)
{
  return proxy.iface == interface && proxy.port == port;
}
/** @brief Own a native monitor and count virtual destruction.
 * @details It retains no process pointers or callbacks and is never registered
 * with a process. Its inherited signal is the native default no-op.
 */
struct Monitor : sc_core::sc_process_monitor
{
  static int destroyed;
  ~Monitor() override
  {
    ++destroyed;
  }
};
int Monitor::destroyed = 0;
/** @brief Invoke the exact native monitor signal signature on a live owner.
 * @details The null thread argument is not dereferenced by the default method.
 * No ownership transfer, scheduling or callback occurs; the GIL stays held.
 */
bool native_signal(sc_core::sc_process_monitor& monitor)
{
  void (sc_core::sc_process_monitor::*operation)(sc_core::sc_thread_handle, int) = &sc_core::sc_process_monitor::signal;
  (monitor.*operation)(nullptr, sc_core::sc_process_monitor::spm_exit);
  return sc_core::sc_process_monitor::spm_exit == 0;
}
}  // namespace fourth_core_pointer_monitor
"""
)
probe = cppyy.gbl.fourth_core_pointer_monitor
name = sc.sc_module_name("pointer_monitor")
top = sc.sc_module(name)
signal = sc.sc_signal[int]("signal")
port = sc.sc_port[sc.sc_signal_in_if[int]]("port")
port(signal)
proxy = sc.sc_bind_proxy()
assert probe.endpoints(proxy, cppyy.nullptr, cppyy.nullptr)
proxy.iface = signal
assert probe.endpoints(proxy, signal, cppyy.nullptr)
proxy.iface = cppyy.nullptr
proxy.port = port
assert probe.endpoints(proxy, cppyy.nullptr, port)
proxy.port = cppyy.nullptr
assert probe.endpoints(proxy, cppyy.nullptr, cppyy.nullptr)
for member in ("iface", "port"):
    try:
        setattr(proxy, member, object())
    except (TypeError, RuntimeError) as error:
        if isinstance(error, RuntimeError):
            assert "property type mismatch or assignment not allowed" in str(error)
        assert probe.endpoints(proxy, cppyy.nullptr, cppyy.nullptr)
    else:
        raise AssertionError(f"invalid native pointer accepted for {member}")
assert probe.endpoints(proxy, cppyy.nullptr, cppyy.nullptr)
assert not sc.SC_BIND_PROXY_NIL.iface and not sc.SC_BIND_PROXY_NIL.port

monitor = probe.Monitor()
assert monitor.signal(cppyy.nullptr, int(sc.sc_process_monitor.spm_exit)) is None
assert sc.sc_process_monitor.signal(monitor, cppyy.nullptr, 0) is None
assert probe.native_signal(monitor)
try:
    monitor.signal(object(), 0)
except TypeError:
    pass
else:
    raise AssertionError("invalid native thread pointer accepted")
monitor.__destruct__()
assert probe.Monitor.destroyed == 1
name.__destruct__()
print("FOURTH_CORE_POINTER_MONITOR 4 2 0 1")
