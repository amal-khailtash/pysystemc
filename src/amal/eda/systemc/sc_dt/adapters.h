#pragma once
#include <systemc>

namespace pysystemc_datatypes
{
/** Return the exact native enum spelling without cross-enum overload conversion.
 * The enum value is copied; the returned string owns its storage. Call with the
 * GIL held. Native allocation errors propagate; no callbacks or waits occur.
 */
template <class Enum>
std::string enum_string(Enum value)
{
  return sc_dt::to_string(value);
}
/** Select a mutable fixed-point bit using the exact non-const native overload.
 * The proxy is a value borrowing its number; Python must retain that number.
 * Call with the GIL held; no waits or callbacks are introduced. Native bounds
 * reports propagate and retain the configured report actions.
 */
template <class Number>
struct fixed_number_bits
{
  /** Return the native mutable proxy; index is the public storage-bit index. */
  static auto bit(Number& number, int index)
  {
    return number.bit(index);
  }
};
/** Return independent native signed remainder storage; GIL held, no callbacks.
 * Native division reports propagate. A zero divisor retains native abort semantics.
 */
sc_dt::sc_signed remainder(const sc_dt::sc_signed& left, const sc_dt::sc_signed& right)
{
  return left % right;
}
/** Shift into independent native storage, preserving native widths and int counts.
 * GIL held; conversion/report errors propagate and operands remain borrowed.
 */
sc_dt::sc_unsigned left_shift(const sc_dt::sc_unsigned& value, int count)
{
  return value << count;
}
/** Right shift into owned native storage with the same lifetime/thread contract. */
sc_dt::sc_unsigned right_shift(const sc_dt::sc_unsigned& value, int count)
{
  return value >> count;
}

/** Own a native context as a subobject, avoiding its private heap allocator.
 * Values are copied. Returned references borrow this owner/current context.
 * Call with the GIL held in the originating process; no callbacks are retained.
 * Native reports propagate. Nested scopes must end/destroy in reverse order.
 */
template <class T>
class context_owner
{
  sc_dt::sc_context<T> context_;

 public:
  /** Copy parameters and optionally activate; native construction reports propagate. */
  explicit context_owner(const T& value, sc_dt::sc_context_begin begin = sc_dt::SC_NOW) : context_(value, begin)
  {
  }
  context_owner(const context_owner&) = delete;
  context_owner& operator=(const context_owner&) = delete;
  /** Activate this copied value; native duplicate-begin reports propagate. */
  void begin()
  {
    context_.begin();
  }
  /** Restore the preceding value; native inactive-end reports propagate. */
  void end()
  {
    context_.end();
  }
  /** Borrow the owner's immutable value until owner destruction. */
  const T& value() const
  {
    return context_.value();
  }
  /** Borrow the current default until its owning context ends. */
  static const T& default_value()
  {
    return sc_dt::sc_context<T>::default_value();
  }
};

}  // namespace pysystemc_datatypes
