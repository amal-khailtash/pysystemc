/** @file Native fixed-point observer dispatch and value-base lifetime probes. */
#pragma once
#include <cstdint>
#include <stdexcept>
#include <systemc>
#include <vector>
namespace datatype_observer_probe
{
/** @brief Construct a public derived value base and count virtual destruction. */
struct ValueBase : sc_dt::sc_value_base
{
  inline static int destroyed = 0;
  /** @brief Count virtual deletion without accessing a destroyed value. */
  ~ValueBase() override
  {
    ++destroyed;
  }
};
/** @brief User-defined generic numeric source, borrowing no other storage. */
struct Generic : sc_dt::sc_generic_base<Generic>
{
  sc_dt::int64 number;
  /** @brief Own an exact signed source value. */
  explicit Generic(sc_dt::int64 input) : number(input)
  {
  }
  /** @brief Return the native unsigned conversion without owning other storage. */
  sc_dt::uint64 to_uint64() const
  {
    return static_cast<sc_dt::uint64>(number);
  }
  /** @brief Return the signed conversion used by native generic constructors. */
  sc_dt::int64 to_int64() const
  {
    return number;
  }
  /** @brief Assign caller-owned arbitrary-width unsigned output. */
  void to_sc_unsigned(sc_dt::sc_unsigned& value) const
  {
    value = to_uint64();
  }
  /** @brief Assign caller-owned arbitrary-width signed output. */
  void to_sc_signed(sc_dt::sc_signed& value) const
  {
    value = number;
  }
};
/** @brief Return exact mutable/const CRTP arrows, borrowing their source. */
inline std::uintptr_t generic_arrow(Generic& value)
{
  return reinterpret_cast<std::uintptr_t>(value.operator->());
}
inline std::uintptr_t generic_const_arrow(const Generic& value)
{
  return reinterpret_cast<std::uintptr_t>(value.operator->());
}
inline std::uintptr_t concat_arrow(sc_dt::sc_concatref& value)
{
  return reinterpret_cast<std::uintptr_t>(value.operator->());
}
inline std::uintptr_t concat_const_arrow(const sc_dt::sc_concatref& value)
{
  return reinterpret_cast<std::uintptr_t>(value.operator->());
}
/** @brief Public observer probe retaining no callback/value ownership.
 * @details Caller retains the observer during dispatch; Python overrides run
 * with the GIL held and must not suspend. Values are borrowed only for each
 * callback. Exceptions propagate; factory state is restored on every exit.
 */
struct Num : sc_dt::sc_fxnum_observer
{
  inline static int destroyed = 0;
  /** @brief Expose the protected native observer constructor. */
  Num() = default;
  /** @brief Count destruction before the native base subobject is released. */
  ~Num() override
  {
    ++destroyed;
  }
  /** @brief Invoke all qualified native default no-op implementations. */
  void defaults(const sc_dt::sc_fxnum& value)
  {
    sc_dt::sc_fxnum_observer::construct(value);
    sc_dt::sc_fxnum_observer::read(value);
    sc_dt::sc_fxnum_observer::write(value);
    sc_dt::sc_fxnum_observer::destruct(value);
  }
  /** @brief Dispatch each public virtual callback once with borrowed storage. */
  static void dispatch(sc_dt::sc_fxnum_observer& observer, const sc_dt::sc_fxnum& value)
  {
    observer.construct(value);
    observer.read(value);
    observer.write(value);
    observer.destruct(value);
  }
  inline static sc_dt::sc_fxnum_observer* current = nullptr;
  /** @brief Return the borrowed observer during the scoped factory probe. */
  static sc_dt::sc_fxnum_observer* factory()
  {
    return current;
  }
  /** @brief Exercise native factory pointer storage and restore prior state. */
  static bool factory_identity(sc_dt::sc_fxnum_observer& observer)
  {
    const auto previous = sc_dt::sc_fxnum_observer::default_observer;
    current = &observer;
    sc_dt::sc_fxnum_observer::default_observer = &factory;
    const bool matches = sc_dt::sc_fxnum_observer::default_observer() == &observer;
    sc_dt::sc_fxnum_observer::default_observer = previous;
    current = nullptr;
    return matches && sc_dt::sc_fxnum_observer::default_observer == previous;
  }
};
/** @brief Native callback collector independent of Python overrides. */
struct NativeNum : Num
{
  std::vector<double> values;
  /** @brief Record native construct dispatch with a borrowed fixed-point value. */
  void construct(const sc_dt::sc_fxnum& value) override
  {
    values.push_back(value.to_double());
  }
  /** @brief Record native read dispatch with a borrowed fixed-point value. */
  void read(const sc_dt::sc_fxnum& value) override
  {
    values.push_back(value.to_double() + 1);
  }
  /** @brief Record native write dispatch with a borrowed fixed-point value. */
  void write(const sc_dt::sc_fxnum& value) override
  {
    values.push_back(value.to_double() + 2);
  }
  /** @brief Record native destruct dispatch with a borrowed fixed-point value. */
  void destruct(const sc_dt::sc_fxnum& value) override
  {
    values.push_back(value.to_double() + 3);
  }
};
/** @brief Public observer probe retaining no callback/value ownership.
 * @details Caller retains the observer during dispatch; Python overrides run
 * with the GIL held and must not suspend. Values are borrowed only for each
 * callback. Exceptions propagate; factory state is restored on every exit.
 */
struct FastNum : sc_dt::sc_fxnum_fast_observer
{
  inline static int destroyed = 0;
  /** @brief Expose the protected native observer constructor. */
  FastNum() = default;
  /** @brief Count destruction before the native base subobject is released. */
  ~FastNum() override
  {
    ++destroyed;
  }
  /** @brief Invoke all qualified native default no-op implementations. */
  void defaults(const sc_dt::sc_fxnum_fast& value)
  {
    sc_dt::sc_fxnum_fast_observer::construct(value);
    sc_dt::sc_fxnum_fast_observer::read(value);
    sc_dt::sc_fxnum_fast_observer::write(value);
    sc_dt::sc_fxnum_fast_observer::destruct(value);
  }
  /** @brief Dispatch each public virtual callback once with borrowed storage. */
  static void dispatch(sc_dt::sc_fxnum_fast_observer& observer, const sc_dt::sc_fxnum_fast& value)
  {
    observer.construct(value);
    observer.read(value);
    observer.write(value);
    observer.destruct(value);
  }
  inline static sc_dt::sc_fxnum_fast_observer* current = nullptr;
  /** @brief Return the borrowed observer during the scoped factory probe. */
  static sc_dt::sc_fxnum_fast_observer* factory()
  {
    return current;
  }
  /** @brief Exercise native factory pointer storage and restore prior state. */
  static bool factory_identity(sc_dt::sc_fxnum_fast_observer& observer)
  {
    const auto previous = sc_dt::sc_fxnum_fast_observer::default_observer;
    current = &observer;
    sc_dt::sc_fxnum_fast_observer::default_observer = &factory;
    const bool matches = sc_dt::sc_fxnum_fast_observer::default_observer() == &observer;
    sc_dt::sc_fxnum_fast_observer::default_observer = previous;
    current = nullptr;
    return matches && sc_dt::sc_fxnum_fast_observer::default_observer == previous;
  }
};
/** @brief Native callback collector independent of Python overrides. */
struct NativeFastNum : FastNum
{
  std::vector<double> values;
  /** @brief Record native construct dispatch with a borrowed fixed-point value. */
  void construct(const sc_dt::sc_fxnum_fast& value) override
  {
    values.push_back(value.to_double());
  }
  /** @brief Record native read dispatch with a borrowed fixed-point value. */
  void read(const sc_dt::sc_fxnum_fast& value) override
  {
    values.push_back(value.to_double() + 1);
  }
  /** @brief Record native write dispatch with a borrowed fixed-point value. */
  void write(const sc_dt::sc_fxnum_fast& value) override
  {
    values.push_back(value.to_double() + 2);
  }
  /** @brief Record native destruct dispatch with a borrowed fixed-point value. */
  void destruct(const sc_dt::sc_fxnum_fast& value) override
  {
    values.push_back(value.to_double() + 3);
  }
};
/** @brief Public observer probe retaining no callback/value ownership.
 * @details Caller retains the observer during dispatch; Python overrides run
 * with the GIL held and must not suspend. Values are borrowed only for each
 * callback. Exceptions propagate; factory state is restored on every exit.
 */
struct Val : sc_dt::sc_fxval_observer
{
  inline static int destroyed = 0;
  /** @brief Expose the protected native observer constructor. */
  Val() = default;
  /** @brief Count destruction before the native base subobject is released. */
  ~Val() override
  {
    ++destroyed;
  }
  /** @brief Invoke all qualified native default no-op implementations. */
  void defaults(const sc_dt::sc_fxval& value)
  {
    sc_dt::sc_fxval_observer::construct(value);
    sc_dt::sc_fxval_observer::read(value);
    sc_dt::sc_fxval_observer::write(value);
    sc_dt::sc_fxval_observer::destruct(value);
  }
  /** @brief Dispatch each public virtual callback once with borrowed storage. */
  static void dispatch(sc_dt::sc_fxval_observer& observer, const sc_dt::sc_fxval& value)
  {
    observer.construct(value);
    observer.read(value);
    observer.write(value);
    observer.destruct(value);
  }
  inline static sc_dt::sc_fxval_observer* current = nullptr;
  /** @brief Return the borrowed observer during the scoped factory probe. */
  static sc_dt::sc_fxval_observer* factory()
  {
    return current;
  }
  /** @brief Exercise native factory pointer storage and restore prior state. */
  static bool factory_identity(sc_dt::sc_fxval_observer& observer)
  {
    const auto previous = sc_dt::sc_fxval_observer::default_observer;
    current = &observer;
    sc_dt::sc_fxval_observer::default_observer = &factory;
    const bool matches = sc_dt::sc_fxval_observer::default_observer() == &observer;
    sc_dt::sc_fxval_observer::default_observer = previous;
    current = nullptr;
    return matches && sc_dt::sc_fxval_observer::default_observer == previous;
  }
};
/** @brief Native callback collector independent of Python overrides. */
struct NativeVal : Val
{
  std::vector<double> values;
  /** @brief Record native construct dispatch with a borrowed fixed-point value. */
  void construct(const sc_dt::sc_fxval& value) override
  {
    values.push_back(value.to_double());
  }
  /** @brief Record native read dispatch with a borrowed fixed-point value. */
  void read(const sc_dt::sc_fxval& value) override
  {
    values.push_back(value.to_double() + 1);
  }
  /** @brief Record native write dispatch with a borrowed fixed-point value. */
  void write(const sc_dt::sc_fxval& value) override
  {
    values.push_back(value.to_double() + 2);
  }
  /** @brief Record native destruct dispatch with a borrowed fixed-point value. */
  void destruct(const sc_dt::sc_fxval& value) override
  {
    values.push_back(value.to_double() + 3);
  }
};
/** @brief Public observer probe retaining no callback/value ownership.
 * @details Caller retains the observer during dispatch; Python overrides run
 * with the GIL held and must not suspend. Values are borrowed only for each
 * callback. Exceptions propagate; factory state is restored on every exit.
 */
struct FastVal : sc_dt::sc_fxval_fast_observer
{
  inline static int destroyed = 0;
  /** @brief Expose the protected native observer constructor. */
  FastVal() = default;
  /** @brief Count destruction before the native base subobject is released. */
  ~FastVal() override
  {
    ++destroyed;
  }
  /** @brief Invoke all qualified native default no-op implementations. */
  void defaults(const sc_dt::sc_fxval_fast& value)
  {
    sc_dt::sc_fxval_fast_observer::construct(value);
    sc_dt::sc_fxval_fast_observer::read(value);
    sc_dt::sc_fxval_fast_observer::write(value);
    sc_dt::sc_fxval_fast_observer::destruct(value);
  }
  /** @brief Dispatch each public virtual callback once with borrowed storage. */
  static void dispatch(sc_dt::sc_fxval_fast_observer& observer, const sc_dt::sc_fxval_fast& value)
  {
    observer.construct(value);
    observer.read(value);
    observer.write(value);
    observer.destruct(value);
  }
  inline static sc_dt::sc_fxval_fast_observer* current = nullptr;
  /** @brief Return the borrowed observer during the scoped factory probe. */
  static sc_dt::sc_fxval_fast_observer* factory()
  {
    return current;
  }
  /** @brief Exercise native factory pointer storage and restore prior state. */
  static bool factory_identity(sc_dt::sc_fxval_fast_observer& observer)
  {
    const auto previous = sc_dt::sc_fxval_fast_observer::default_observer;
    current = &observer;
    sc_dt::sc_fxval_fast_observer::default_observer = &factory;
    const bool matches = sc_dt::sc_fxval_fast_observer::default_observer() == &observer;
    sc_dt::sc_fxval_fast_observer::default_observer = previous;
    current = nullptr;
    return matches && sc_dt::sc_fxval_fast_observer::default_observer == previous;
  }
};
/** @brief Native callback collector independent of Python overrides. */
struct NativeFastVal : FastVal
{
  std::vector<double> values;
  /** @brief Record native construct dispatch with a borrowed fixed-point value. */
  void construct(const sc_dt::sc_fxval_fast& value) override
  {
    values.push_back(value.to_double());
  }
  /** @brief Record native read dispatch with a borrowed fixed-point value. */
  void read(const sc_dt::sc_fxval_fast& value) override
  {
    values.push_back(value.to_double() + 1);
  }
  /** @brief Record native write dispatch with a borrowed fixed-point value. */
  void write(const sc_dt::sc_fxval_fast& value) override
  {
    values.push_back(value.to_double() + 2);
  }
  /** @brief Record native destruct dispatch with a borrowed fixed-point value. */
  void destruct(const sc_dt::sc_fxval_fast& value) override
  {
    values.push_back(value.to_double() + 3);
  }
};
}  // namespace datatype_observer_probe
