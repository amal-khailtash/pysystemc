/** @file tlm_values_public.cpp
 * @brief Independent native reference for DMI, phases and quantumkeepers.
 * @details Process-local native state lives until the fresh audit process exits.
 * DMI borrows the fixture byte vector. The extended phase is owned by the
 * process-wide phase registry. No Python callbacks or GIL access occur here.
 */
#include <tlm_utils/tlm_quantumkeeper.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tlm>
#include <vector>

#include "tlm_values_probe.h"

namespace
{

std::vector<std::string> records;

/** @brief Stop the native reference at the first contract mismatch. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Record one process action and its native simulation timestamp. */
void record(const std::string& owner, const std::string& action)
{
  records.push_back(owner + " " + action + " " + sc_core::sc_time_stamp().to_string());
}

/** @brief Record a simulated time value alongside the current process time.
 * @param owner Label for the native process being checked.
 * @param action Operation whose locally computed time is reported.
 * @param value Native time value owned by the caller and read synchronously.
 */
void record_value(const std::string& owner, const std::string& action, const sc_core::sc_time& value)
{
  records.push_back(owner + " " + action + " " + value.to_string());
}

/** @brief Record mandatory scope exit during process reset or kill. */
struct cleanup
{
  std::string owner; /**< Process label retained while its stack is suspended. */

  /** @brief Append the cleanup action as native stack unwinds. */
  ~cleanup()
  {
    record(owner, "cleanup");
  }
};

/** @brief Validate all DMI pointer, address, latency and permission operations. */
void check_dmi()
{
  tlm::tlm_dmi dmi;
  verify(dmi.get_dmi_ptr() == nullptr, "DMI initial pointer");
  verify(dmi.get_start_address() == 0 && dmi.get_end_address() == UINT64_MAX, "DMI initial range");
  verify(dmi.get_read_latency() == sc_core::SC_ZERO_TIME && dmi.get_write_latency() == sc_core::SC_ZERO_TIME,
         "DMI initial latency");
  verify(dmi.get_granted_access() == tlm::tlm_dmi::DMI_ACCESS_NONE && dmi.is_none_allowed(), "DMI initial access");
  verify(!dmi.is_read_allowed() && !dmi.is_write_allowed() && !dmi.is_read_write_allowed(), "DMI initial predicates");

  dmi.allow_read();
  verify(dmi.get_granted_access() == tlm::tlm_dmi::DMI_ACCESS_READ && dmi.is_read_allowed() &&
           !dmi.is_write_allowed() && !dmi.is_read_write_allowed() && !dmi.is_none_allowed(),
         "DMI read permission");
  dmi.allow_write();
  verify(dmi.get_granted_access() == tlm::tlm_dmi::DMI_ACCESS_WRITE && !dmi.is_read_allowed() &&
           dmi.is_write_allowed() && !dmi.is_read_write_allowed(),
         "DMI write permission");
  dmi.allow_read_write();
  verify(dmi.get_granted_access() == tlm::tlm_dmi::DMI_ACCESS_READ_WRITE && dmi.is_read_allowed() &&
           dmi.is_write_allowed() && dmi.is_read_write_allowed(),
         "DMI read/write permission");
  dmi.allow_none();
  verify(dmi.get_granted_access() == tlm::tlm_dmi::DMI_ACCESS_NONE && dmi.is_none_allowed(), "DMI none permission");

  std::vector<unsigned char> memory(8, 0);
  memory[3] = 29;
  dmi.set_dmi_ptr(memory.data());
  verify(dmi.get_dmi_ptr() == memory.data() && dmi.get_dmi_ptr()[3] == 29, "DMI pointer get/set");
  dmi.get_dmi_ptr()[4] = 47;
  verify(memory[4] == 47, "DMI borrowed pointer write");

  const sc_dt::uint64 start = (sc_dt::uint64{1} << 63) + 29;
  const sc_dt::uint64 end = UINT64_MAX;
  dmi.set_start_address(start);
  dmi.set_end_address(end);
  dmi.set_read_latency(sc_core::sc_time(3, sc_core::SC_NS));
  dmi.set_write_latency(sc_core::sc_time(7, sc_core::SC_NS));
  dmi.set_granted_access(tlm::tlm_dmi::DMI_ACCESS_READ_WRITE);
  verify(dmi.get_start_address() == start && dmi.get_end_address() == end, "DMI 64-bit addresses");
  verify(dmi.get_read_latency() == sc_core::sc_time(3, sc_core::SC_NS) &&
           dmi.get_write_latency() == sc_core::sc_time(7, sc_core::SC_NS),
         "DMI latencies");
  verify(dmi.get_granted_access() == tlm::tlm_dmi::DMI_ACCESS_READ_WRITE && dmi.is_read_write_allowed(),
         "DMI access setter");

  dmi.init();
  verify(dmi.get_dmi_ptr() == nullptr && dmi.get_start_address() == 0 && dmi.get_end_address() == end,
         "DMI reinitialization range and pointer");
  verify(dmi.get_read_latency() == sc_core::SC_ZERO_TIME && dmi.get_write_latency() == sc_core::SC_ZERO_TIME &&
           dmi.is_none_allowed(),
         "DMI reinitialization defaults");
  std::cout << "TLM_VALUES_REFERENCE DMI " << start << ' ' << end << " 3 ns 7 ns 29 47\n";
}

/** @brief Validate standard phase conversion, assignment and custom streaming. */
void check_phases()
{
  const tlm::tlm_phase_enum values[] = {tlm::UNINITIALIZED_PHASE, tlm::BEGIN_REQ, tlm::END_REQ, tlm::BEGIN_RESP,
                                        tlm::END_RESP};
  const char* names[] = {"UNINITIALIZED_PHASE", "BEGIN_REQ", "END_REQ", "BEGIN_RESP", "END_RESP"};
  for (unsigned index = 0; index < 5; ++index)
  {
    const tlm::tlm_phase phase(values[index]);
    verify(static_cast<unsigned int>(phase) == index, "standard phase conversion");
    verify(std::string(phase.get_name()) == names[index], "standard phase name");
    verify(tlm_values_probe::stream_phase(phase) == names[index], "standard phase stream insertion");
  }
  tlm::tlm_phase phase;
  verify(static_cast<unsigned int>(phase) == tlm::UNINITIALIZED_PHASE, "default phase");
  phase = tlm::BEGIN_RESP;
  verify(static_cast<unsigned int>(phase) == tlm::BEGIN_RESP && std::string(phase.get_name()) == "BEGIN_RESP",
         "phase enum assignment");
  const tlm::tlm_phase arbitrary(19);
  verify(static_cast<unsigned int>(arbitrary) == 19, "unsigned phase construction");

  const tlm::tlm_phase& custom = tlm_values_probe::custom_phase();
  const tlm::tlm_phase& custom_again = tlm_values_probe::custom_phase();
  verify(static_cast<unsigned int>(custom) == 5 && static_cast<unsigned int>(custom_again) == 5,
         "extended phase registry identity");
  verify(std::string(custom.get_name()) == "PYTHON_AUDIT_PHASE", "extended phase name");
  verify(tlm_values_probe::stream_phase(custom) == "PYTHON_AUDIT_PHASE", "extended phase stream insertion");
  std::cout << "TLM_VALUES_REFERENCE PHASE 0,1,2,3,4 custom=5:PYTHON_AUDIT_PHASE\n";
}

/** @brief Exercise independent quantumkeepers in native pthread processes. */
struct QuantumReference : sc_core::sc_module
{
  tlm_utils::tlm_quantumkeeper main_keeper;
  tlm_utils::tlm_quantumkeeper reset_keeper;
  tlm_utils::tlm_quantumkeeper kill_keeper;
  sc_core::sc_process_handle reset_handle;
  sc_core::sc_process_handle kill_handle;

  SC_HAS_PROCESS(QuantumReference);

  /** @brief Register the value, reset, kill and controller SystemC threads. */
  explicit QuantumReference(sc_core::sc_module_name name) : sc_module(name)
  {
    SC_THREAD(main_thread);
    SC_THREAD(reset_worker);
    reset_handle = sc_core::sc_get_last_created_process_handle();
    SC_THREAD(kill_worker);
    kill_handle = sc_core::sc_get_last_created_process_handle();
    SC_THREAD(controller);
  }

  /** @brief Run reset/kill workers through native quantumkeeper waits. */
  void worker(unsigned index)
  {
    auto& keeper = index == 0 ? reset_keeper : kill_keeper;
    const std::string owner = index == 0 ? "RESET" : "KILL";
    record(owner, "start");
    keeper.reset();
    cleanup guard{owner};
    if (index == 0)
    {
      keeper.set(sc_core::sc_time(100, sc_core::SC_NS));
      keeper.sync();
    }
    else
    {
      keeper.set_and_sync(sc_core::sc_time(100, sc_core::SC_NS));
    }
    throw std::runtime_error("interrupted quantum sync returned");
  }

  /** @brief Bind the reset worker entry to a reusable helper. */
  void reset_worker()
  {
    worker(0);
  }

  /** @brief Bind the kill worker entry to a reusable helper. */
  void kill_worker()
  {
    worker(1);
  }

  /** @brief Verify increments, threshold queries and both synchronization calls. */
  void main_thread()
  {
    main_keeper.reset();
    verify(main_keeper.get_local_time() == sc_core::SC_ZERO_TIME, "keeper reset local time");
    verify(main_keeper.get_current_time() == sc_core::SC_ZERO_TIME, "keeper reset current time");
    record("MAIN", "start");
    main_keeper.inc(sc_core::sc_time(4, sc_core::SC_NS));
    verify(!main_keeper.need_sync(), "keeper below sync threshold");
    verify(main_keeper.get_local_time() == sc_core::sc_time(4, sc_core::SC_NS) &&
             main_keeper.get_current_time() == sc_core::sc_time(4, sc_core::SC_NS),
           "keeper first increment");
    record_value("MAIN", "inc4", main_keeper.get_current_time());
    main_keeper.inc(sc_core::sc_time(6, sc_core::SC_NS));
    verify(main_keeper.need_sync(), "keeper reaches sync threshold");
    verify(main_keeper.get_local_time() == sc_core::sc_time(10, sc_core::SC_NS) &&
             main_keeper.get_current_time() == sc_core::sc_time(10, sc_core::SC_NS),
           "keeper second increment");
    record_value("MAIN", "inc10", main_keeper.get_current_time());
    main_keeper.sync();
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(10, sc_core::SC_NS), "keeper sync timestamp");
    verify(main_keeper.get_local_time() == sc_core::SC_ZERO_TIME &&
             main_keeper.get_current_time() == sc_core::sc_time(10, sc_core::SC_NS),
           "keeper post-sync time");
    record("MAIN", "sync");
    main_keeper.set_and_sync(sc_core::sc_time(10, sc_core::SC_NS));
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(20, sc_core::SC_NS), "keeper set_and_sync timestamp");
    verify(main_keeper.get_local_time() == sc_core::SC_ZERO_TIME &&
             main_keeper.get_current_time() == sc_core::sc_time(20, sc_core::SC_NS),
           "keeper post-set_and_sync time");
    record("MAIN", "set_sync");
  }

  /** @brief Reset at 2 ns then kill both workers at 3 ns. */
  void controller()
  {
    sc_core::wait(2, sc_core::SC_NS);
    reset_handle.reset();
    kill_handle.reset();
    sc_core::wait(1, sc_core::SC_NS);
    reset_handle.kill();
    kill_handle.kill();
  }
};

}  // namespace

/** @brief Run the standalone reference and emit checked result records. */
int sc_main(int, char**)
{
  auto& global_quantum = tlm::tlm_global_quantum::instance();
  global_quantum.set(sc_core::sc_time(10, sc_core::SC_NS));
  verify(tlm::tlm_global_quantum::instance().get() == sc_core::sc_time(10, sc_core::SC_NS), "global quantum singleton");
  verify(global_quantum.compute_local_quantum() == sc_core::sc_time(10, sc_core::SC_NS), "global quantum compute");
  tlm::tlm_global_quantum::instance().set(sc_core::sc_time(8, sc_core::SC_NS));
  verify(global_quantum.get() == sc_core::sc_time(8, sc_core::SC_NS), "global quantum shared instance");
  global_quantum.set(sc_core::sc_time(10, sc_core::SC_NS));
  verify(tlm_utils::tlm_quantumkeeper::get_global_quantum() == sc_core::sc_time(10, sc_core::SC_NS),
         "quantumkeeper static global quantum");
  {
    tlm_values_probe::QuantumkeeperProbe protected_keeper;
    verify(protected_keeper.compute_local_quantum_probe() == sc_core::sc_time(10, sc_core::SC_NS),
           "quantumkeeper protected local-quantum computation");
  }
  verify(tlm_values_probe::QuantumkeeperProbe::destructions() == 1, "quantumkeeper derived destructor");
  tlm_utils::tlm_quantumkeeper::set_global_quantum(sc_core::sc_time(12, sc_core::SC_NS));
  verify(global_quantum.get() == sc_core::sc_time(12, sc_core::SC_NS), "quantumkeeper global quantum setter");
  tlm_utils::tlm_quantumkeeper::set_global_quantum(sc_core::sc_time(10, sc_core::SC_NS));
  tlm_values_probe::GlobalQuantumProbe independent_quantum;
  verify(independent_quantum.get() == sc_core::SC_ZERO_TIME, "derived protected global quantum constructor");
  independent_quantum.set(sc_core::sc_time(6, sc_core::SC_NS));
  verify(independent_quantum.get() == sc_core::sc_time(6, sc_core::SC_NS) &&
           independent_quantum.compute_local_quantum() == sc_core::sc_time(6, sc_core::SC_NS),
         "derived global quantum value and destruction");

  check_dmi();
  check_phases();
  QuantumReference model("top");
  sc_core::sc_start(21, sc_core::SC_NS);
  verify(model.reset_handle.terminated() && model.kill_handle.terminated(), "quantum worker termination");

  for (const auto& item : records) std::cout << "TLM_VALUES_REFERENCE " << item << '\n';
  sc_core::sc_stop();
  return 0;
}
