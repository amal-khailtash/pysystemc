/** @file sc_dt_integer_owners_probe.h
 * @brief Exact declaration probes and safe helpers for arbitrary-width integers.
 * @details Helpers retain no Python objects or callbacks. Native owners passed
 *          to views must outlive those views. All calls are synchronous and
 *          require the caller to hold the GIL when invoked through cppyy.
 */
#pragma once

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>

namespace sc_dt_integer_owners_probe
{
using Signed = sc_dt::sc_signed;
using Unsigned = sc_dt::sc_unsigned;
using SignedScalar = sc_dt::int64;
using UnsignedScalar = sc_dt::uint64;
using SignedInt = int;
using UnsignedInt = unsigned int;
using SignedLong = long;
using UnsignedLong = unsigned long;
using Double = double;
using SignedFixed = sc_dt::sc_int_base;
using UnsignedFixed = sc_dt::sc_uint_base;

/** @brief Construct native signed and unsigned aliases with distinguishable values. */
inline SignedScalar signed_scalar()
{
  return static_cast<SignedScalar>(-0x123456789abLL);
}

/** @brief Construct a native unsigned alias with high bits set. */
inline UnsignedScalar unsigned_scalar()
{
  return static_cast<UnsignedScalar>(0xfedcba9876543210ULL);
}

/** @brief Return whether fixed-point declarations were enabled in SystemC. */
inline constexpr bool fixed_point_enabled()
{
#ifdef SC_INCLUDE_FX
  return true;
#else
  return false;
#endif
}

#define ASSERT_INTEGER_COMPOUND(Type, Method, Argument)                                             \
  static_assert(std::is_same<decltype(static_cast<const Type& (Type::*)(Argument)>(&Type::Method)), \
                             const Type& (Type::*)(Argument)>::value,                               \
                "arbitrary-width compound declaration changed")
#define ASSERT_INTEGER_CONCAT(Type, Argument)                                                         \
  static_assert(std::is_same<decltype(static_cast<void (Type::*)(Argument, int)>(&Type::concat_set)), \
                             void (Type::*)(Argument, int)>::value,                                   \
                "arbitrary-width concatenation declaration changed")
#define ASSERT_INTEGER_COMPOUNDS(Type)                                    \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, int);                         \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, unsigned int);                \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, long);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, unsigned long);               \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, sc_dt::int64);                \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, sc_dt::uint64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, const sc_dt::sc_int_base&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, const sc_dt::sc_uint_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, const sc_dt::sc_signed&);     \
  ASSERT_INTEGER_COMPOUND(Type, operator+=, const sc_dt::sc_unsigned&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, int);                         \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, unsigned int);                \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, long);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, unsigned long);               \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, sc_dt::int64);                \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, sc_dt::uint64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, const sc_dt::sc_int_base&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, const sc_dt::sc_uint_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, const sc_dt::sc_signed&);     \
  ASSERT_INTEGER_COMPOUND(Type, operator-=, const sc_dt::sc_unsigned&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, int);                         \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, unsigned int);                \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, long);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, unsigned long);               \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, sc_dt::int64);                \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, sc_dt::uint64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, const sc_dt::sc_int_base&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, const sc_dt::sc_uint_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, const sc_dt::sc_signed&);     \
  ASSERT_INTEGER_COMPOUND(Type, operator*=, const sc_dt::sc_unsigned&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, int);                         \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, unsigned int);                \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, long);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, unsigned long);               \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, sc_dt::int64);                \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, sc_dt::uint64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, const sc_dt::sc_int_base&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, const sc_dt::sc_uint_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, const sc_dt::sc_signed&);     \
  ASSERT_INTEGER_COMPOUND(Type, operator/=, const sc_dt::sc_unsigned&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, int);                         \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, unsigned int);                \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, long);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, unsigned long);               \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, sc_dt::int64);                \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, sc_dt::uint64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, const sc_dt::sc_int_base&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, const sc_dt::sc_uint_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, const sc_dt::sc_signed&);     \
  ASSERT_INTEGER_COMPOUND(Type, operator%=, const sc_dt::sc_unsigned&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, int);                         \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, unsigned int);                \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, long);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, unsigned long);               \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, sc_dt::int64);                \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, sc_dt::uint64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, const sc_dt::sc_int_base&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, const sc_dt::sc_uint_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, const sc_dt::sc_signed&);     \
  ASSERT_INTEGER_COMPOUND(Type, operator&=, const sc_dt::sc_unsigned&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, int);                         \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, unsigned int);                \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, long);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, unsigned long);               \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, sc_dt::int64);                \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, sc_dt::uint64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, const sc_dt::sc_int_base&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, const sc_dt::sc_uint_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, const sc_dt::sc_signed&);     \
  ASSERT_INTEGER_COMPOUND(Type, operator|=, const sc_dt::sc_unsigned&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, int);                         \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, unsigned int);                \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, long);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, unsigned long);               \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, sc_dt::int64);                \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, sc_dt::uint64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, const sc_dt::sc_int_base&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, const sc_dt::sc_uint_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, const sc_dt::sc_signed&);     \
  ASSERT_INTEGER_COMPOUND(Type, operator^=, const sc_dt::sc_unsigned&);   \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, int);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, unsigned int);               \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, long);                       \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, unsigned long);              \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, sc_dt::int64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, sc_dt::uint64);              \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, const sc_dt::sc_int_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, const sc_dt::sc_uint_base&); \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, const sc_dt::sc_signed&);    \
  ASSERT_INTEGER_COMPOUND(Type, operator<<=, const sc_dt::sc_unsigned&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, int);                        \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, unsigned int);               \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, long);                       \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, unsigned long);              \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, sc_dt::int64);               \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, sc_dt::uint64);              \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, const sc_dt::sc_int_base&);  \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, const sc_dt::sc_uint_base&); \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, const sc_dt::sc_signed&);    \
  ASSERT_INTEGER_COMPOUND(Type, operator>>=, const sc_dt::sc_unsigned&)

ASSERT_INTEGER_COMPOUNDS(Signed);
ASSERT_INTEGER_COMPOUNDS(Unsigned);

static_assert(std::is_constructible<Signed, const sc_dt::sc_signed&>::value &&
                std::is_constructible<Signed, const sc_dt::sc_unsigned&>::value &&
                std::is_constructible<Signed, const sc_dt::sc_signed_subref_r&>::value &&
                std::is_constructible<Signed, const sc_dt::sc_unsigned_subref_r&>::value &&
                std::is_constructible<Signed, const sc_dt::sc_int_subref_r&>::value &&
                std::is_constructible<Signed, const sc_dt::sc_uint_subref_r&>::value &&
                std::is_constructible<Signed, const sc_dt::sc_bv_base&>::value &&
                std::is_constructible<Signed, const sc_dt::sc_lv_base&>::value,
              "sc_signed public constructors changed");
static_assert(std::is_constructible<Unsigned, const sc_dt::sc_signed&>::value &&
                std::is_constructible<Unsigned, const sc_dt::sc_unsigned&>::value &&
                std::is_constructible<Unsigned, const sc_dt::sc_signed_subref_r&>::value &&
                std::is_constructible<Unsigned, const sc_dt::sc_unsigned_subref_r&>::value &&
                std::is_constructible<Unsigned, const sc_dt::sc_int_subref_r&>::value &&
                std::is_constructible<Unsigned, const sc_dt::sc_uint_subref_r&>::value &&
                std::is_constructible<Unsigned, const sc_dt::sc_bv_base&>::value &&
                std::is_constructible<Unsigned, const sc_dt::sc_lv_base&>::value,
              "sc_unsigned public constructors changed");
static_assert(std::is_same<decltype(++std::declval<Signed&>()), Signed&>::value &&
                std::is_same<std::remove_cv_t<decltype(std::declval<Signed&>()++)>, Signed>::value &&
                std::is_same<decltype(--std::declval<Unsigned&>()), Unsigned&>::value &&
                std::is_same<std::remove_cv_t<decltype(std::declval<Unsigned&>()--)>, Unsigned>::value,
              "arbitrary-width prefix/postfix declarations changed");

/** @brief Borrow a const signed range from a live owner. */
inline const sc_dt::sc_signed_subref_r& const_signed_range(const Signed& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Borrow a const unsigned range from a live owner. */
inline const sc_dt::sc_unsigned_subref_r& const_unsigned_range(const Unsigned& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Borrow a const signed operator() range from a live owner. */
inline const sc_dt::sc_signed_subref_r& const_signed_call(const Signed& owner, int high, int low)
{
  return owner(high, low);
}

/** @brief Borrow a const unsigned operator() range from a live owner. */
inline const sc_dt::sc_unsigned_subref_r& const_unsigned_call(const Unsigned& owner, int high, int low)
{
  return owner(high, low);
}

/** @brief Borrow a const signed indexed bit from a live owner. */
inline const sc_dt::sc_signed_bitref_r& const_signed_bit(const Signed& owner, int index)
{
  return owner[index];
}

/** @brief Borrow a const unsigned indexed bit from a live owner. */
inline const sc_dt::sc_unsigned_bitref_r& const_unsigned_bit(const Unsigned& owner, int index)
{
  return owner[index];
}

/** @brief Borrow a const bounded signed range from a live owner. */
inline const sc_dt::sc_int_subref_r& const_int_range(const SignedFixed& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Borrow a const bounded unsigned range from a live owner. */
inline const sc_dt::sc_uint_subref_r& const_uint_range(const UnsignedFixed& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Return true while preserving the caller-owned native view. */
template <class View>
inline bool view_value(const View& view)
{
  return view.to_bool();
}

/** @brief Return report details for invalid index/range probes with SC_ERROR throwing. */
template <class Action>
inline std::string checked_report(Action action)
{
  const auto previous = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  std::string report;
  try
  {
    action();
  }
  catch (const sc_core::sc_report& error)
  {
    report = std::string(error.get_msg_type()) + "\t" + error.get_msg();
  }
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
  return report;
}

/** @brief Return packed digit count for an arbitrary bit width. */
inline int digit_count(int width)
{
  return (width + sc_dt::SC_DIGIT_SIZE - 1) / sc_dt::SC_DIGIT_SIZE;
}

/** @brief Make SystemC error reports catchable for a bounded probe. */
inline sc_core::sc_actions throw_errors()
{
  return sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
}

/** @brief Restore the prior SystemC error report action. */
inline void restore_errors(sc_core::sc_actions previous)
{
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
}

/** @brief Format native storage digits without exposing pointer ownership. */
inline std::string digit_text(const sc_dt::sc_digit* digits, int count)
{
  std::ostringstream output;
  for (int index = 0; index < count; ++index)
  {
    if (index != 0)
    {
      output << ',';
    }
    output << digits[index];
  }
  return output.str();
}

#undef ASSERT_INTEGER_COMPOUND
#undef ASSERT_INTEGER_CONCAT
#undef ASSERT_INTEGER_COMPOUNDS
}  // namespace sc_dt_integer_owners_probe
