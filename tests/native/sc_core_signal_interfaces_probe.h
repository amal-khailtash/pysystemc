/** @file sc_core_signal_interfaces_probe.h
 * @brief Native signal-interface implementations and channel ownership probes.
 */
#ifndef PYSYSTEMC_SIGNAL_INTERFACES_PROBE_H
#define PYSYSTEMC_SIGNAL_INTERFACES_PROBE_H
#include <cstdint>
#include <systemc>
namespace signal_interfaces_probe
{
/** @brief Return the configured native default policy macro as its enum type. */
inline sc_core::sc_writer_policy default_policy()
{
  return sc_core::SC_DEFAULT_WRITER_POLICY;
}
/** @brief Store the number of destroyed test channels without Python references. */
inline int& destructions()
{
  static int count = 0;
  return count;
}
/** @brief Own a type-independent channel and expose its scheduled update.
 * @details Caller owns the channel and its events; synchronous calls hold GIL.
 * Kernel update invokes no Python callback; delete only after consumers detach.
 */
struct Channel : sc_core::sc_signal_channel
{
  /** @brief Construct a native named primitive channel. */
  explicit Channel(const char* name) : sc_signal_channel(name)
  {
  }
  /** @brief Record virtual destruction before the base releases its event. */
  ~Channel() override
  {
    ++destructions();
  }
  /** @brief Request the native value-change phase without transferring ownership. */
  void change()
  {
    request_update();
  }
  /** @brief Dispatch native signal-channel notification during update. */
  void update() override
  {
    do_update();
  }
};
/** @brief Implement all input/output interfaces with native signal storage.
 * @tparam T Arbitrary native signal value type, including bool and logic.
 * @details The probe owns its backing signal. Interface and event references
 * borrow that storage. GIL stays held for synchronous calls; no Python owners
 * or callbacks are stored. Native signal errors propagate unchanged.
 */
template <class T>
struct Interface : sc_core::sc_signal_inout_if<T>
{
  sc_core::sc_signal<T> backing; /**< Native owned value/event storage. */
  /** @brief Invoke the protected input/inout and public output constructors. */
  explicit Interface(const char* name) : backing(name)
  {
  }
  /** @brief Borrow the native current value without creating copied storage. */
  const T& read() const override
  {
    return backing.read();
  }
  /** @brief Borrow the same native current value for trace consumers. */
  const T& get_data_ref() const override
  {
    return backing.read();
  }
  /** @brief Borrow the backing value-change event. */
  const sc_core::sc_event& value_changed_event() const override
  {
    return backing.value_changed_event();
  }
  /** @brief Query the current native change delta. */
  bool event() const override
  {
    return backing.event();
  }
  /** @brief Copy a pending value into the backing channel. */
  void write(const T& value) override
  {
    backing.write(value);
  }
  /** @brief Borrow a positive-edge event for specialized input interfaces. */
  const sc_core::sc_event& posedge_event() const
  {
    return backing.posedge_event();
  }
  /** @brief Borrow a negative-edge event for specialized input interfaces. */
  const sc_core::sc_event& negedge_event() const
  {
    return backing.negedge_event();
  }
  /** @brief Query specialized positive-edge state. */
  bool posedge() const
  {
    return backing.posedge();
  }
  /** @brief Query specialized negative-edge state. */
  bool negedge() const
  {
    return backing.negedge();
  }
  /** @brief Borrow the input base, retaining the implementation externally. */
  sc_core::sc_signal_in_if<T>& input()
  {
    return *this;
  }
  /** @brief Borrow the output base, retaining the implementation externally. */
  sc_core::sc_signal_write_if<T>& output()
  {
    return *this;
  }
  /** @brief Borrow the combined base, retaining the implementation externally. */
  sc_core::sc_signal_inout_if<T>& inout()
  {
    return *this;
  }
  /** @brief Verify read/data references alias the same owned native value. */
  bool aliases() const
  {
    return &read() == &get_data_ref() && &read() == &backing.read();
  }
};
/** @brief Transfer a derived channel to an owning native base pointer.
 * @details Python caller must adopt deletion ownership; no callback/GIL change.
 */
inline sc_core::sc_signal_channel* owned_channel()
{
  return new Channel("destruction_channel");
}
}  // namespace signal_interfaces_probe
#endif
