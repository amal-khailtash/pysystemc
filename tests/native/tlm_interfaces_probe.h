/** @file tlm_interfaces_probe.h
 *  @brief Concrete native probes for small public TLM interface families.
 *  @details These helpers own their event objects and test values. Returned
 *  interface references borrow the adapter and must not outlive it. The
 *  synchronous probes retain no Python callbacks, acquire no GIL state, and
 *  do not wait or enter the scheduler.
 */
#ifndef PYSYSTEMC_TLM_INTERFACES_PROBE_H
#define PYSYSTEMC_TLM_INTERFACES_PROBE_H

#include <array>
#include <systemc>
#include <tlm>
#include <type_traits>
#include <vector>

namespace pysystemc_tlm_interfaces
{

/** @brief Track dynamic destruction through the public interface base. */
inline std::array<int, 4>& destruction_counts()
{
  static std::array<int, 4> counts{};
  return counts;
}

/** @brief A small custom value used to verify non-scalar TLM templates. */
struct Record
{
  int code = 0;
};

/** @brief Concrete implementation of all get, peek and readiness contracts.
 *  @tparam T Public TLM value type, including arbitrary SystemC widths.
 *  @details The adapter owns its events and one sample. Interface consumers
 *  borrow this object; no callback or Python state is retained.
 */
template <class T>
class GetPeekAdapter : public tlm::tlm_get_peek_if<T>
{
 public:
  using tlm::tlm_blocking_get_if<T>::get;
  using tlm::tlm_blocking_peek_if<T>::peek;

  /** @brief Construct an adapter before installing its first sample. */
  GetPeekAdapter() = default;

  /** @brief Construct an adapter with one available sample. */
  explicit GetPeekAdapter(const T& sample) : sample_(sample)
  {
  }

  /** @brief Count destruction through an interface pointer. */
  ~GetPeekAdapter() override
  {
    ++destruction_counts()[0];
  }

  /** @brief Copy a new sample and make it available to read operations. */
  void set_sample(const T& sample)
  {
    sample_ = sample;
    available_ = true;
  }

  /** @brief Consume and return the sample through the blocking value overload. */
  T get(tlm::tlm_tag<T>* = nullptr) override
  {
    ++get_calls_;
    available_ = false;
    return sample_;
  }

  /** @brief Return the sample without consuming it. */
  T peek(tlm::tlm_tag<T>* = nullptr) const override
  {
    ++peek_calls_;
    return sample_;
  }

  /** @brief Consume one sample when available. */
  bool nb_get(T& output) override
  {
    ++get_calls_;
    if (!available_) return false;
    output = sample_;
    available_ = false;
    return true;
  }

  /** @brief Report whether a sample remains available for get. */
  bool nb_can_get(tlm::tlm_tag<T>* = nullptr) const override
  {
    return available_;
  }

  /** @brief Return the event owned by this adapter for get readiness. */
  const sc_core::sc_event& ok_to_get(tlm::tlm_tag<T>* = nullptr) const override
  {
    return get_ready_;
  }

  /** @brief Copy the sample without consuming it. */
  bool nb_peek(T& output) const override
  {
    ++peek_calls_;
    if (!available_) return false;
    output = sample_;
    return true;
  }

  /** @brief Report whether a sample remains available for peek. */
  bool nb_can_peek(tlm::tlm_tag<T>* = nullptr) const override
  {
    return available_;
  }

  /** @brief Return the event owned by this adapter for peek readiness. */
  const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<T>* = nullptr) const override
  {
    return peek_ready_;
  }

  /** @brief Return blocking get call count for runtime evidence. */
  int get_calls() const
  {
    return get_calls_;
  }
  /** @brief Return blocking peek call count for runtime evidence. */
  int peek_calls() const
  {
    return peek_calls_;
  }

 private:
  T sample_;
  mutable bool available_ = true;
  mutable int get_calls_ = 0;
  mutable int peek_calls_ = 0;
  sc_core::sc_event get_ready_;
  sc_core::sc_event peek_ready_;
};

/** @brief Concrete analysis and delayed-analysis implementation. */
template <class T>
class WriteAdapter : public tlm::tlm_analysis_if<T>, public tlm::tlm_delayed_analysis_if<T>
{
 public:
  /** @brief Count destruction through either write interface. */
  ~WriteAdapter() override
  {
    ++destruction_counts()[1];
  }

  /** @brief Retain a copy of an immediate write value. */
  void write(const T& value) override
  {
    immediate_ = value;
    ++immediate_calls_;
  }

  /** @brief Retain a copy and the borrowed delay value for a delayed write. */
  void write(const T& value, const sc_core::sc_time& delay) override
  {
    delayed_ = value;
    delay_ = delay;
    ++delayed_calls_;
  }

  /** @brief Return the last immediate value owned by this adapter. */
  const T& immediate() const
  {
    return immediate_;
  }
  /** @brief Return the last delayed value owned by this adapter. */
  const T& delayed() const
  {
    return delayed_;
  }
  /** @brief Return the copied delay argument. */
  const sc_core::sc_time& delay() const
  {
    return delay_;
  }
  /** @brief Return immediate callback count. */
  int immediate_calls() const
  {
    return immediate_calls_;
  }
  /** @brief Return delayed callback count. */
  int delayed_calls() const
  {
    return delayed_calls_;
  }

 private:
  T immediate_{};
  T delayed_{};
  sc_core::sc_time delay_ = sc_core::SC_ZERO_TIME;
  int immediate_calls_ = 0;
  int delayed_calls_ = 0;
};

/** @brief Concrete FIFO debug and size-configuration implementation. */
template <class T>
class FifoControlAdapter : public tlm::tlm_fifo_debug_if<T>, public tlm::tlm_fifo_config_size_if
{
 public:
  /** @brief Construct a fixed-capacity debug FIFO with two ordered samples. */
  FifoControlAdapter(const T& newest, const T& oldest) : values_{newest, oldest}
  {
  }

  /** @brief Count destruction through a FIFO interface pointer. */
  ~FifoControlAdapter() override
  {
    ++destruction_counts()[2];
  }

  /** @brief Return the number of retained debug samples. */
  int used() const override
  {
    return static_cast<int>(values_.size());
  }
  /** @brief Return the configured capacity. */
  int size() const override
  {
    return capacity_;
  }
  /** @brief Record a debug inspection without changing data. */
  void debug() const override
  {
    ++debug_calls_;
  }

  /** @brief Read the indexed sample, where zero is newest. */
  bool nb_peek(T& output, int index) const override
  {
    if (index < 0 || static_cast<std::size_t>(index) >= values_.size()) return false;
    output = values_[static_cast<std::size_t>(index)];
    return true;
  }

  /** @brief Replace an indexed sample, where zero is newest. */
  bool nb_poke(const T& value, int index = 0) override
  {
    if (index < 0 || static_cast<std::size_t>(index) >= values_.size()) return false;
    values_[static_cast<std::size_t>(index)] = value;
    return true;
  }

  /** @brief Increase capacity by the native default or supplied amount. */
  void nb_expand(unsigned int amount = 1) override
  {
    capacity_ += static_cast<int>(amount);
  }
  /** @brief Record unbounded mode and its native default increment. */
  void nb_unbound(unsigned int amount = 16) override
  {
    capacity_ += static_cast<int>(amount);
  }
  /** @brief Reduce capacity when the requested decrement is valid. */
  bool nb_reduce(unsigned int amount = 1) override
  {
    if (amount >= static_cast<unsigned int>(capacity_)) return false;
    capacity_ -= static_cast<int>(amount);
    return true;
  }
  /** @brief Bound the FIFO to a size no smaller than its current occupancy. */
  bool nb_bound(unsigned int amount) override
  {
    if (amount < values_.size()) return false;
    capacity_ = static_cast<int>(amount);
    return true;
  }

  /** @brief Return the number of debug calls. */
  int debug_calls() const
  {
    return debug_calls_;
  }

 private:
  std::vector<T> values_;
  int capacity_ = 4;
  mutable int debug_calls_ = 0;
};

/** @brief Concrete implementation of the complete base TLM-2 forward/backward
 *  interfaces. It retains only scalar call observations; payload and DMI
 *  objects remain owned by the caller.
 */
class BaseProtocolAdapter : public tlm::tlm_fw_transport_if<>, public tlm::tlm_bw_transport_if<>
{
 public:
  /** @brief Observe a forward nonblocking call without borrowing its arguments. */
  /** @brief Count destruction through forward or backward interface views. */
  ~BaseProtocolAdapter() override
  {
    ++destruction_counts()[3];
  }

  tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload&, tlm::tlm_phase&, sc_core::sc_time&) override
  {
    ++fw_calls_;
    return tlm::TLM_UPDATED;
  }
  /** @brief Observe a backward nonblocking call and return completion. */
  tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload&, tlm::tlm_phase&, sc_core::sc_time&) override
  {
    ++bw_calls_;
    return tlm::TLM_COMPLETED;
  }
  /** @brief Annotate the borrowed delay synchronously without waiting. */
  void b_transport(tlm::tlm_generic_payload&, sc_core::sc_time& delay) override
  {
    ++blocking_calls_;
    delay += sc_core::sc_time(1, sc_core::SC_NS);
  }
  /** @brief Set the borrowed DMI range and report a grant. */
  bool get_direct_mem_ptr(tlm::tlm_generic_payload&, tlm::tlm_dmi& dmi) override
  {
    ++dmi_calls_;
    dmi.set_start_address(0x10);
    dmi.set_end_address(0x1f);
    return true;
  }
  /** @brief Copy an invalidation range without retaining caller storage. */
  void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end) override
  {
    invalidated_start_ = start;
    invalidated_end_ = end;
    ++invalidation_calls_;
  }
  /** @brief Observe debug transport and report four transferred bytes. */
  unsigned int transport_dbg(tlm::tlm_generic_payload&) override
  {
    ++debug_transport_calls_;
    return 4;
  }

  /** @brief Return the native forward callback count. */
  int fw_calls() const
  {
    return fw_calls_;
  }
  /** @brief Return the native backward callback count. */
  int bw_calls() const
  {
    return bw_calls_;
  }
  /** @brief Return the native blocking callback count. */
  int blocking_calls() const
  {
    return blocking_calls_;
  }
  /** @brief Return the direct memory callback count. */
  int dmi_calls() const
  {
    return dmi_calls_;
  }
  /** @brief Return the invalidation callback count. */
  int invalidation_calls() const
  {
    return invalidation_calls_;
  }
  /** @brief Return the debug transport callback count. */
  int debug_transport_calls() const
  {
    return debug_transport_calls_;
  }
  /** @brief Return the most recent invalidated region. */
  sc_dt::uint64 invalidated_start() const
  {
    return invalidated_start_;
  }
  /** @brief Return the most recent invalidated region. */
  sc_dt::uint64 invalidated_end() const
  {
    return invalidated_end_;
  }

 private:
  int fw_calls_ = 0;
  int bw_calls_ = 0;
  int blocking_calls_ = 0;
  int dmi_calls_ = 0;
  int invalidation_calls_ = 0;
  int debug_transport_calls_ = 0;
  sc_dt::uint64 invalidated_start_ = 0;
  sc_dt::uint64 invalidated_end_ = 0;
};

using WideValueAdapter = GetPeekAdapter<sc_dt::sc_biguint<137>>;
using CustomWriteAdapter = WriteAdapter<Record>;
using WideFifoAdapter = FifoControlAdapter<sc_dt::sc_biguint<137>>;

/** @brief Allocate a wide get/peek probe behind a cppyy-friendly factory.
 *  @return Owned adapter pointer; the caller must retain the proxy while using
 *  its borrowed interface views. The short-lived test process releases it.
 */
inline WideValueAdapter* make_wide_value_adapter(const sc_dt::sc_biguint<137>& sample)
{
  return new WideValueAdapter(sample);
}

/** @brief Return get callback count for cppyy proxies of the wide adapter. */
inline int wide_get_calls(const WideValueAdapter& adapter)
{
  return adapter.get_calls();
}
/** @brief Return peek callback count for cppyy proxies of the wide adapter. */
inline int wide_peek_calls(const WideValueAdapter& adapter)
{
  return adapter.peek_calls();
}
/** @brief Reset the wide adapter's retained sample for overload probes. */
inline void reset_wide_value_adapter(WideValueAdapter& adapter, const sc_dt::sc_biguint<137>& sample)
{
  adapter.set_sample(sample);
}

/** @brief Return the blocking get view of a get/peek adapter. */
template <class T>
inline tlm::tlm_blocking_get_if<T>& blocking_get_view(GetPeekAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the nonblocking get view of a get/peek adapter. */
template <class T>
inline tlm::tlm_nonblocking_get_if<T>& nonblocking_get_view(GetPeekAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the combined get view of a get/peek adapter. */
template <class T>
inline tlm::tlm_get_if<T>& get_view(GetPeekAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the blocking peek view of a get/peek adapter. */
template <class T>
inline tlm::tlm_blocking_peek_if<T>& blocking_peek_view(GetPeekAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the nonblocking peek view of a get/peek adapter. */
template <class T>
inline tlm::tlm_nonblocking_peek_if<T>& nonblocking_peek_view(GetPeekAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the combined peek view of a get/peek adapter. */
template <class T>
inline tlm::tlm_peek_if<T>& peek_view(GetPeekAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the blocking get/peek combined view of an adapter. */
template <class T>
inline tlm::tlm_blocking_get_peek_if<T>& blocking_get_peek_view(GetPeekAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the nonblocking get/peek combined view of an adapter. */
template <class T>
inline tlm::tlm_nonblocking_get_peek_if<T>& nonblocking_get_peek_view(GetPeekAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the full get/peek combined view of an adapter. */
template <class T>
inline tlm::tlm_get_peek_if<T>& get_peek_view(GetPeekAdapter<T>& adapter)
{
  return adapter;
}

/** @brief Return the analysis view of a write adapter. */
template <class T>
inline tlm::tlm_analysis_if<T>& analysis_view(WriteAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the delayed analysis view of a write adapter. */
template <class T>
inline tlm::tlm_delayed_analysis_if<T>& delayed_analysis_view(WriteAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the immediate write view of a write adapter. */
template <class T>
inline tlm::tlm_write_if<T>& write_view(WriteAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the delayed write view of a write adapter. */
template <class T>
inline tlm::tlm_delayed_write_if<T>& delayed_write_view(WriteAdapter<T>& adapter)
{
  return adapter;
}

/** @brief Return the FIFO debug interface view. */
template <class T>
inline tlm::tlm_fifo_debug_if<T>& fifo_debug_view(FifoControlAdapter<T>& adapter)
{
  return adapter;
}
/** @brief Return the FIFO configuration interface view. */
template <class T>
inline tlm::tlm_fifo_config_size_if& fifo_config_view(FifoControlAdapter<T>& adapter)
{
  return adapter;
}

/** @brief Return the forward nonblocking transport view. */
inline tlm::tlm_fw_nonblocking_transport_if<>& fw_nonblocking_view(BaseProtocolAdapter& adapter)
{
  return adapter;
}
/** @brief Return the backward nonblocking transport view. */
inline tlm::tlm_bw_nonblocking_transport_if<>& bw_nonblocking_view(BaseProtocolAdapter& adapter)
{
  return adapter;
}
/** @brief Return the blocking transport view. */
inline tlm::tlm_blocking_transport_if<>& blocking_transport_view(BaseProtocolAdapter& adapter)
{
  return adapter;
}
/** @brief Return the forward DMI view. */
inline tlm::tlm_fw_direct_mem_if<>& fw_direct_mem_view(BaseProtocolAdapter& adapter)
{
  return adapter;
}
/** @brief Return the backward DMI view. */
inline tlm::tlm_bw_direct_mem_if& bw_direct_mem_view(BaseProtocolAdapter& adapter)
{
  return adapter;
}
/** @brief Return the debug transport view. */
inline tlm::tlm_transport_dbg_if<>& debug_transport_view(BaseProtocolAdapter& adapter)
{
  return adapter;
}

/** @brief Destroy a concrete channel through its virtual native interface. */
inline void destroy_interface(sc_core::sc_interface* interface)
{
  delete interface;
}

static_assert(std::is_base_of<tlm::tlm_blocking_get_if<sc_dt::sc_biguint<137>>,
                              tlm::tlm_get_peek_if<sc_dt::sc_biguint<137>>>::value,
              "get_peek includes blocking get");
static_assert(std::is_base_of<tlm::tlm_nonblocking_get_if<sc_dt::sc_biguint<137>>,
                              tlm::tlm_get_peek_if<sc_dt::sc_biguint<137>>>::value,
              "get_peek includes nonblocking get");
static_assert(std::is_same<tlm::tlm_base_protocol_types::tlm_payload_type, tlm::tlm_generic_payload>::value,
              "base protocol payload alias");
static_assert(std::is_same<tlm::tlm_base_protocol_types::tlm_phase_type, tlm::tlm_phase>::value,
              "base protocol phase alias");

}  // namespace pysystemc_tlm_interfaces

#endif  // PYSYSTEMC_TLM_INTERFACES_PROBE_H
