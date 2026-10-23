/** @file sc_core_clock_vector_probe.h
 * @brief Native helpers for the public clock and pointer-vector audit.
 * @details Pointer vectors borrow their pointees; fixture storage is static and
 * remains alive for the lifetime of each fresh-process audit. Clock probes keep
 * ordinary SystemC ownership and callback rules.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_CLOCK_VECTOR_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_CLOCK_VECTOR_PROBE_H

#include <sysc/communication/sc_clock.h>
#include <sysc/kernel/sc_module.h>
#include <sysc/utils/sc_pvector.h>

#include <cstddef>
#include <typeinfo>
#include <vector>

namespace sc_core_clock_vector_probe
{

/** @brief Expose one protected clock predicate through legal native derivation.
 * @details The inherited clock owns its events and process state. The helper
 * does not retain callbacks, acquire the GIL, or access kernel state from a
 * host thread.
 */
class ClockProbe final : public sc_core::sc_clock
{
 public:
  /** @brief Construct a standard clock for protected-helper inspection.
   * @param name Native SystemC object name retained by the clock.
   * @param period Clock period copied into the native clock.
   */
  ClockProbe(const char* name, const sc_core::sc_time& period) : sc_clock(name, period)
  {
  }

  /** @brief Invoke the protected native clock-type helper. */
  bool protected_is_clock() const
  {
    return is_clock();
  }
};

/** @brief Module-scoped output port used to exercise clock registration.
 * @details The signal owns its channel value; the port binds to that signal
 * during module construction. Both follow native SystemC module lifetime.
 */
class OutputPortProbe final : public sc_core::sc_module
{
 public:
  sc_core::sc_signal<bool> sink; /**< Native signal bound to port. */
  sc_core::sc_out<bool> port;    /**< Module-owned output port. */

  /** @brief Construct a valid module-scoped output port and its sink.
   * @param name Unique native module name.
   */
  explicit OutputPortProbe(sc_core::sc_module_name name) : sc_module(name), sink("sink"), port("port")
  {
    port.bind(sink);
  }
};

/** @brief Return a pointer to fixture-owned integer storage.
 * @param index Fixture index from zero through four.
 * @return Borrowed pointer valid until the process exits.
 */
inline int* integer_pointer(unsigned index)
{
  static int values[5]{3, 1, 4, 1, 5};
  return &values[index];
}

/** @brief Read a pvector sort argument as a borrowed integer pointer slot.
 * @param slot Pointer to one qsort element containing an int pointer.
 * @return The integer pointee value.
 * @details The vector and fixture own no pointee through this pointer. Calls
 * are synchronous and do not use Python or the GIL.
 */
inline int integer_slot_value(const void* slot)
{
  return **static_cast<int* const*>(slot);
}

/** @brief Read one borrowed fixture integer. */
inline int integer_value(const int* value)
{
  return *value;
}

/** @brief Read an integer through the vector's public indexed-access method. */
inline int integer_vector_value(const sc_core::sc_pvector<int*>& vector, unsigned index)
{
  return *vector[index];
}

/** @brief Read an integer through the vector's public fetch method. */
inline int fetched_integer_value(sc_core::sc_pvector<int*>& vector, int index)
{
  return *vector.fetch(index);
}

/** @brief Set an integer pointer through public indexing from a native adapter. */
inline void set_integer_vector_value(sc_core::sc_pvector<int*>& vector, unsigned index, int* value)
{
  vector[index] = value;
}

/** @brief Check the value-initialized slot produced by indexed growth. */
inline bool integer_vector_slot_is_null(sc_core::sc_pvector<int*>& vector, unsigned index)
{
  return vector[index] == nullptr;
}

/** @brief Native object used to instantiate a second arbitrary pointer type. */
struct Owner
{
  int value; /**< Caller-visible value stored in fixture-owned native storage. */
};

/** @brief Arbitrary payload used for direct typed-item pointer indexing. */
struct Item
{
  int key; /**< Value read after native pointer retrieval. */
};

/** @brief Return a borrowed pointer to fixture-owned Owner storage.
 * @param index Fixture index from zero through two.
 * @return Borrowed pointer valid until the process exits.
 */
inline Owner* owner_pointer(unsigned index)
{
  static Owner owners[3]{{17}, {4}, {29}};
  return &owners[index];
}

/** @brief Return a borrowed pointer to Item fixture storage.
 * @param index Fixture index from zero through two.
 * @return Borrowed Item pointer valid until process exit.
 */
inline Item* item_pointer(unsigned index)
{
  static Item items[3]{{11}, {7}, {19}};
  return &items[index];
}

/** @brief Read one Item through pvector's indexed pointer value. */
inline int item_vector_value(const sc_core::sc_pvector<Item*>& vector, unsigned index)
{
  return vector[index]->key;
}

/** @brief Verify Item pointer slots preserve fixture addresses across copies. */
inline bool item_vector_points_to_fixture(const sc_core::sc_pvector<Item*>& vector, unsigned index,
                                          unsigned fixture_index)
{
  return vector[index] == item_pointer(fixture_index);
}

/** @brief Read one borrowed Owner value without transferring ownership. */
inline int owner_value(const Owner* owner)
{
  return owner->value;
}

/** @brief Read one owner through pvector's indexed access. */
inline int owner_vector_value(const sc_core::sc_pvector<Owner*>& vector, unsigned index)
{
  return vector[index]->value;
}

/** @brief Read one Owner through pvector's public fetch method. */
inline int fetched_owner_value(sc_core::sc_pvector<Owner*>& vector, int index)
{
  return vector.fetch(index)->value;
}

/** @brief Verify shallow pointer-copy semantics for one fixture owner. */
inline bool owner_vector_points_to_fixture(const sc_core::sc_pvector<Owner*>& vector, unsigned index,
                                           unsigned fixture_index)
{
  return vector[index] == owner_pointer(fixture_index);
}

/** @brief Return the address of fixture-owned time storage. */
inline sc_core::sc_time* time_pointer(unsigned index)
{
  static sc_core::sc_time times[3]{sc_core::sc_time(3, sc_core::SC_NS), sc_core::sc_time(1, sc_core::SC_NS),
                                   sc_core::sc_time(2, sc_core::SC_NS)};
  return &times[index];
}

/** @brief Read a borrowed time pointer in nanoseconds. */
inline int time_nanoseconds(const sc_core::sc_time* time)
{
  return static_cast<int>(time->to_seconds() * 1.0e9 + 0.5);
}

/** @brief Read a time value through pvector's public fetch method. */
inline int fetched_time_nanoseconds(sc_core::sc_pvector<sc_core::sc_time*>& vector, int index)
{
  return time_nanoseconds(vector.fetch(index));
}

/** @brief Return the size through sc_pvector's std::vector conversion. */
template <class T>
inline std::size_t converted_size(const sc_core::sc_pvector<T>& vector)
{
  const std::vector<T>& storage = vector;
  return storage.size();
}

/** @brief Check that public iterator-like pointers refer to raw vector storage.
 * @return True for an empty vector or when begin and raw_data agree.
 */
template <class T>
inline bool iterator_storage_agrees(sc_core::sc_pvector<T>& vector)
{
  return vector.size() == 0 || (vector.begin() == vector.raw_data() && vector.end() - vector.begin() == vector.size());
}

/** @brief Exercise the const iterator, index, fetch and raw-storage overloads. */
template <class T>
inline bool const_storage_agrees(const sc_core::sc_pvector<T>& vector)
{
  if (vector.size() == 0) return vector.begin() == nullptr && vector.end() == nullptr;
  return vector.begin() == vector.raw_data() && vector.end() - vector.begin() == vector.size() &&
         &vector.fetch(0) == vector.raw_data() && &vector[0] == vector.raw_data();
}

/** @brief Assign through the public sc_pvector copy-assignment operator. */
template <class T>
inline void assign(sc_core::sc_pvector<T>& destination, const sc_core::sc_pvector<T>& source)
{
  destination = source;
}

/** @brief Exercise the protected clock predicate on a temporary native clock.
 * @return True when the derived native object reports itself as a clock.
 */
inline bool protected_clock_helper()
{
  ClockProbe clock("protected_clock", sc_core::sc_time(2, sc_core::SC_NS));
  return clock.protected_is_clock();
}

/** @brief Return the ABI interface name used by sc_clock output-port checks. */
inline const char* bool_output_interface_type_name()
{
  return typeid(sc_core::sc_signal_inout_if<bool>).name();
}

/** @brief Register a legal module-owned output port with a clock.
 * @param clock Clock whose output-port restriction is checked.
 * @return Native report ID, or -1 if registration produces no report.
 * @details The local module owns its port and sink until this synchronous call
 * returns. The native report is caught here; no Python callback or GIL crossing
 * occurs in module or port setup.
 */
inline int output_port_registration_report_id(sc_core::sc_clock& clock)
{
  OutputPortProbe output(sc_core::sc_module_name("clock_output_probe"));
  try
  {
    clock.register_port(output.port, bool_output_interface_type_name());
  }
  catch (const sc_core::sc_report& report)
  {
    return report.get_id();
  }
  return -1;
}

}  // namespace sc_core_clock_vector_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_CLOCK_VECTOR_PROBE_H
