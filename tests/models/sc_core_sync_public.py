"""Compare remaining mutex and semaphore public-family contracts to SystemC."""

import gc

from amal.eda.systemc import sc_core as sc
import cppyy


cppyy.cppdef(
    r"""
#include <sstream>
#include <string>
#include <systemc>

namespace sync_public_probe {
int mutex_destroyed = 0;
int semaphore_destroyed = 0;

/** @brief Expose the protected mutex state query for contract testing.
 *
 * The probe owns its native base subobjects. No callbacks, Python references,
 * GIL state or blocking operations are retained by the probe itself.
 */
struct Mutex : sc_core::sc_mutex {
  Mutex() = default;
  explicit Mutex(const char* name) : sc_core::sc_mutex(name) {}
  ~Mutex() override { ++mutex_destroyed; }
  bool protected_in_use() const { return in_use(); }
};

/** @brief Expose protected semaphore support and report members.
 *
 * Report exceptions are caught locally and serialized while their report
 * storage is alive; no exception or temporary string crosses into Python.
 */
struct Semaphore : sc_core::sc_semaphore {
  explicit Semaphore(int value) : sc_core::sc_semaphore(value) {}
  Semaphore(const char* name, int value) : sc_core::sc_semaphore(name, value) {}
  ~Semaphore() override { ++semaphore_destroyed; }
  bool protected_in_use() const { return in_use(); }
  std::string protected_report_error() const {
    try {
      report_error("sync_public_probe", "details");
    } catch (const sc_core::sc_report& report) {
      std::ostringstream result;
      result << report.get_id() << '\t' << static_cast<int>(report.get_severity()) << '\t'
             << report.get_msg_type() << '\t' << report.get_msg();
      return result.str();
    }
    return "no-report";
  }
};

/** @brief Constructible implementation used to expose the protected interface constructor. */
struct MutexInterface : sc_core::sc_mutex_if {
  int lock() override { return 10; }
  int trylock() override { return 11; }
  int unlock() override { return 12; }
};

/** @brief Constructible implementation used to expose the protected interface constructor. */
struct SemaphoreInterface : sc_core::sc_semaphore_if {
  int wait() override { return 20; }
  int trywait() override { return 21; }
  int post() override { return 22; }
  int get_value() const override { return 23; }
};
}  // namespace sync_public_probe
"""
)

probe = cppyy.gbl.sync_public_probe
model_name = sc.sc_module_name("sync_public")
model = sc.sc_module(model_name)
default_mutex = probe.Mutex()
mutex = probe.Mutex("named_mutex")
default_semaphore = probe.Semaphore(1)
semaphore = probe.Semaphore("named_semaphore", 1)
report_semaphore = probe.Semaphore("report_semaphore", 1)
interface_mutex = probe.MutexInterface()
interface_semaphore = probe.SemaphoreInterface()
records = []

assert default_mutex.basename().startswith("mutex_")
assert default_semaphore.basename().startswith("semaphore_")
assert mutex.kind() == "sc_mutex" and semaphore.kind() == "sc_semaphore"
assert interface_mutex.lock() == 10
assert interface_mutex.trylock() == 11
assert interface_mutex.unlock() == 12
assert interface_semaphore.wait() == 20
assert interface_semaphore.trywait() == 21
assert interface_semaphore.post() == 22
assert interface_semaphore.get_value() == 23
records.append(("mutex_interface", 10, 11, 12))
records.append(("semaphore_interface", 20, 21, 22, 23))
records.append(("report_error", str(report_semaphore.protected_report_error())))


channels = {"mutex": mutex, "semaphore": semaphore}


def exercise():
    """Check protected in-use state through native process-owned operations."""
    mutex = channels["mutex"]
    semaphore = channels["semaphore"]
    assert not mutex.protected_in_use()
    assert mutex.lock() == 0 and mutex.protected_in_use()
    assert mutex.unlock() == 0 and not mutex.protected_in_use()
    assert not semaphore.protected_in_use() and semaphore.get_value() == 1
    assert semaphore.wait() == 0
    assert semaphore.protected_in_use() and semaphore.get_value() == 0
    assert semaphore.post() == 0
    assert not semaphore.protected_in_use() and semaphore.get_value() == 1
    records.append(("in_use", 0, 1, 0, 0, 1, 0, 1))
    sc.sc_stop()


model.sc_thread("exercise", exercise)
del model_name
sc.sc_start()

# Force owned cppyy proxies through their native destructors while the counters
# remain observable. The SystemC object bases own no Python callback storage.
del default_mutex, mutex, default_semaphore, semaphore, report_semaphore
del interface_mutex, interface_semaphore
channels.clear()
gc.collect()
assert probe.mutex_destroyed == 2
assert probe.semaphore_destroyed == 3
records.append(("destruction", int(probe.mutex_destroyed), int(probe.semaphore_destroyed)))

for record in records:
    print("SYNC_PUBLIC_REFERENCE\t" + "\t".join(map(str, record)))
