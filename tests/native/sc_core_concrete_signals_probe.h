/** @file sc_core_concrete_signals_probe.h
 * @brief Small native probes for public concrete signal families.
 * @details These probes expose protected state for read-only assertions and
 *          count kernel-dispatched updates. They do not retain Python values
 *          or callbacks and are used only on the simulation thread.
 */
#ifndef PYSYSTEMC_SC_CORE_CONCRETE_SIGNALS_PROBE_H
#define PYSYSTEMC_SC_CORE_CONCRETE_SIGNALS_PROBE_H

#include <cstddef>
#include <ostream>
#include <systemc>

namespace pysystemc_concrete_signals
{
/** @brief Native payload used to prove sc_buffer remains unrestricted. */
struct Payload
{
  int value;

  /** @brief Construct a zero-valued payload. */
  Payload() : value(0)
  {
  }

  /** @brief Construct a payload with the supplied integer. */
  explicit Payload(int value_) : value(value_)
  {
  }

  /** @brief Compare payload values for sc_signal's update policy. */
  bool operator==(const Payload& other) const
  {
    return value == other.value;
  }
};

/** @brief Print a payload in native reference records. */
inline std::ostream& operator<<(std::ostream& output, const Payload& payload)
{
  return output << payload.value;
}

/** @brief Return the shared test-only resolved-vector destruction count. */
inline int& rv_destruction_count_storage()
{
  static int count = 0;
  return count;
}

/** @brief Read the number of destroyed derived resolved-vector channels. */
inline int rv_destruction_count()
{
  return rv_destruction_count_storage();
}

/** @brief Reset the resolved-vector destruction counter between isolated cases. */
inline void reset_rv_destruction_count()
{
  rv_destruction_count_storage() = 0;
}

/** @brief Count destruction after deletion through sc_signal_rv's owning base.
 * @tparam Width Unrestricted positive vector width.
 * @details This test object owns no external resources and retains no Python
 *          references. Its virtual destructor records native destruction only.
 */
template <int Width>
class ResolvedVectorDestructionProbe : public sc_core::sc_signal_rv<Width>
{
  using Base = sc_core::sc_signal_rv<Width>;

 public:
  /** @brief Construct the derived channel using a native SystemC name. */
  explicit ResolvedVectorDestructionProbe(const char* name) : Base(name)
  {
  }

  /** @brief Record deletion through the public virtual base destructor. */
  ~ResolvedVectorDestructionProbe() override
  {
    ++rv_destruction_count_storage();
  }
};

/** @brief Expose sc_buffer protected storage and count native updates.
 * @tparam T Value type supported by sc_buffer.
 * @tparam Policy Native writer policy.
 * @details The channel owns its stored values. All methods run on the kernel
 *          thread; no Python references or GIL state are involved.
 */
template <typename T, sc_core::sc_writer_policy Policy = sc_core::SC_DEFAULT_WRITER_POLICY>
class BufferProbe : public sc_core::sc_buffer<T, Policy>
{
  using Base = sc_core::sc_buffer<T, Policy>;

 public:
  int update_count = 0;

  /** @brief Construct with native unique naming. */
  BufferProbe() : Base()
  {
  }

  /** @brief Construct by name. */
  explicit BufferProbe(const char* name) : Base(name)
  {
  }

  /** @brief Construct by name with an initial value. */
  BufferProbe(const char* name, const T& value) : Base(name, value)
  {
  }

  /** @brief Read current storage through the protected native member. */
  T current_storage() const
  {
    return this->m_cur_val;
  }

  /** @brief Read pending storage through the protected native member. */
  T pending_storage() const
  {
    return this->m_new_val;
  }

  /** @brief Count and perform the native scheduled update. */
  void update() override
  {
    ++update_count;
    Base::update();
  }
};

/** @brief Expose protected resolved scalar signal state and updates. */
class ResolvedProbe : public sc_core::sc_signal_resolved
{
  using Base = sc_core::sc_signal_resolved;

 public:
  int update_count = 0;

  /** @brief Construct with native unique naming. */
  ResolvedProbe() : Base()
  {
  }

  /** @brief Construct by name. */
  explicit ResolvedProbe(const char* name) : Base(name)
  {
  }

  /** @brief Construct by name with an initial value. */
  ResolvedProbe(const char* name, const value_type& value) : Base(name, value)
  {
  }

  /** @brief Read current storage through the protected native member. */
  value_type current_storage() const
  {
    return this->m_cur_val;
  }

  /** @brief Read pending storage through the protected native member. */
  value_type pending_storage() const
  {
    return this->m_new_val;
  }

  /** @brief Return the number of process-owned resolution values. */
  std::size_t driver_count() const
  {
    return this->m_val_vec.size();
  }

  /** @brief Return the number of process identities retained for resolution. */
  std::size_t process_count() const
  {
    return this->m_proc_vec.size();
  }

  /** @brief Count and perform the native scheduled update. */
  void update() override
  {
    ++update_count;
    Base::update();
  }
};

/** @brief Expose protected resolved vector signal state and updates.
 * @tparam Width Unrestricted positive sc_lv width.
 */
template <int Width>
class ResolvedVectorProbe : public sc_core::sc_signal_rv<Width>
{
  using Base = sc_core::sc_signal_rv<Width>;

 public:
  int update_count = 0;

  /** @brief Construct with native unique naming. */
  ResolvedVectorProbe() : Base()
  {
  }

  /** @brief Construct by name. */
  explicit ResolvedVectorProbe(const char* name) : Base(name)
  {
  }

  /** @brief Construct by name with an initial value. */
  ResolvedVectorProbe(const char* name, const typename Base::value_type& value) : Base(name, value)
  {
  }

  /** @brief Read current storage through the protected native member. */
  typename Base::value_type current_storage() const
  {
    return this->m_cur_val;
  }

  /** @brief Read pending storage through the protected native member. */
  typename Base::value_type pending_storage() const
  {
    return this->m_new_val;
  }

  /** @brief Return the number of process-owned resolution values. */
  std::size_t driver_count() const
  {
    return this->m_val_vec.size();
  }

  /** @brief Return the number of process identities retained for resolution. */
  std::size_t process_count() const
  {
    return this->m_proc_vec.size();
  }

  /** @brief Count and perform the native scheduled update. */
  void update() override
  {
    ++update_count;
    Base::update();
  }
};

static_assert(__is_same(sc_core::sc_buffer<bool>::this_type, sc_core::sc_buffer<bool>), "sc_buffer this_type");
static_assert(__is_same(sc_core::sc_buffer<bool>::value_type, bool), "sc_buffer value_type");
static_assert(__is_same(sc_core::sc_buffer<bool>::base_type,
                        sc_core::sc_signal<bool, sc_core::SC_DEFAULT_WRITER_POLICY>),
              "sc_buffer base_type");
static_assert(__is_same(sc_core::sc_signal_resolved::this_type, sc_core::sc_signal_resolved),
              "sc_signal_resolved this_type");
static_assert(__is_same(sc_core::sc_signal_resolved::value_type, sc_dt::sc_logic), "sc_signal_resolved value_type");
static_assert(__is_same(sc_core::sc_signal_resolved::base_type,
                        sc_core::sc_signal<sc_dt::sc_logic, sc_core::SC_MANY_WRITERS>),
              "sc_signal_resolved base_type");
static_assert(__is_same(sc_core::sc_signal_rv<16>::this_type, sc_core::sc_signal_rv<16>), "sc_signal_rv this_type");
static_assert(__is_same(sc_core::sc_signal_rv<16>::value_type, sc_dt::sc_lv<16>), "sc_signal_rv value_type");
static_assert(__is_same(sc_core::sc_signal_rv<16>::base_type,
                        sc_core::sc_signal<sc_dt::sc_lv<16>, sc_core::SC_MANY_WRITERS>),
              "sc_signal_rv base_type");
}  // namespace pysystemc_concrete_signals

#endif  // PYSYSTEMC_SC_CORE_CONCRETE_SIGNALS_PROBE_H
