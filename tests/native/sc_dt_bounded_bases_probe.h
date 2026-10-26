/** @file sc_dt_bounded_bases_probe.h
 * @brief Compile-time and scalar probes for bounded SystemC integer bases.
 * @details The test helpers preserve exact native scalar types and retain no
 *          Python state. Calls are synchronous, hold the GIL when entered
 *          through cppyy, and do not interact with the SystemC scheduler.
 */
#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>

namespace sc_dt_bounded_bases_probe
{
using SignedBase = sc_dt::sc_int_base;
using UnsignedBase = sc_dt::sc_uint_base;
using SignedScalar = sc_dt::int_type;
using UnsignedScalar = sc_dt::uint_type;
using SignedInt = int;
using UnsignedInt = unsigned int;
using SignedLong = long;
using UnsignedLong = unsigned long;
using Double = double;
using GenericSource = sc_dt::sc_generic_base<sc_dt::sc_concatref>;

/** @brief Return a signed 64-bit source without Python integer narrowing. */
inline SignedScalar signed_scalar()
{
  return static_cast<SignedScalar>(-0x123456789abLL);
}

/** @brief Return an unsigned 64-bit source with high bits set. */
inline UnsignedScalar unsigned_scalar()
{
  return static_cast<UnsignedScalar>(0xfedcba9876543210ULL);
}

/** @brief Return one typed native signed integer assignment source. */
inline int signed_int()
{
  return -1234567;
}

/** @brief Return one typed native unsigned integer assignment source. */
inline unsigned int unsigned_int()
{
  return 2345678U;
}

/** @brief Return one typed native signed long assignment source. */
inline long signed_long()
{
  return -123456789L;
}

/** @brief Return one typed native unsigned long assignment source. */
inline unsigned long unsigned_long()
{
  return 345678901UL;
}

/** @brief Return one fractional native double assignment source. */
inline double double_value()
{
  return -9876.75;
}

/** @brief Return whether this SystemC build exposes fixed-point assignments. */
inline constexpr bool fixed_point_enabled()
{
#ifdef SC_INCLUDE_FX
  return true;
#else
  return false;
#endif
}

/** @brief Return exact signed scalar spelling as a native decimal string. */
inline std::string signed_scalar_text()
{
  return sc_dt::sc_int_base(signed_scalar(), 64).to_string(sc_dt::SC_DEC);
}

/** @brief Return exact unsigned scalar spelling as a native decimal string. */
inline std::string unsigned_scalar_text()
{
  return sc_dt::sc_uint_base(unsigned_scalar(), 64).to_string(sc_dt::SC_DEC);
}

static_assert(std::is_same<SignedScalar, sc_dt::int64>::value, "sc_int_base scalar alias must remain int64");
static_assert(std::is_same<UnsignedScalar, sc_dt::uint64>::value, "sc_uint_base scalar alias must remain uint64");

#define ASSERT_BASE_SCALAR_OPERATOR(Base, Result, Name, Scalar)                                                  \
  static_assert(                                                                                                 \
    std::is_same<decltype(static_cast<Result (Base::*)(Scalar)>(&Base::Name)), Result (Base::*)(Scalar)>::value, \
    "bounded integer scalar operator signature changed")
#define ASSERT_BASE_ASSIGNMENT(Base, Source)                                                          \
  static_assert(std::is_same<decltype(static_cast<Base& (Base::*)(const Source&)>(&Base::operator=)), \
                             Base& (Base::*)(const Source&)>::value,                                  \
                "bounded integer assignment signature changed")
#define ASSERT_BASE_SCALAR_ASSIGNMENT(Base, Scalar)                                                                 \
  static_assert(                                                                                                    \
    std::is_same<decltype(static_cast<Base& (Base::*)(Scalar)>(&Base::operator=)), Base& (Base::*)(Scalar)>::value, \
    "bounded integer scalar assignment signature changed")
#define ASSERT_BASE_COMPOUND(Base, Name, Scalar)                                                               \
  static_assert(                                                                                               \
    std::is_same<decltype(static_cast<Base& (Base::*)(Scalar)>(&Base::Name)), Base& (Base::*)(Scalar)>::value, \
    "bounded integer compound signature changed")
#define ASSERT_BASE_CONCAT_SET(Base, Scalar)                                                        \
  static_assert(std::is_same<decltype(static_cast<void (Base::*)(Scalar, int)>(&Base::concat_set)), \
                             void (Base::*)(Scalar, int)>::value,                                   \
                "bounded integer concat_set signature changed")
#define ASSERT_BASE_CONST0(Base, Name, Result)                                                                   \
  static_assert(                                                                                                 \
    std::is_same<decltype(static_cast<Result (Base::*)() const>(&Base::Name)), Result (Base::*)() const>::value, \
    "bounded integer observer signature changed")
#define ASSERT_BASE_SCALAR0(Base, Name, Scalar)                                                                  \
  static_assert(                                                                                                 \
    std::is_same<decltype(static_cast<Scalar (Base::*)() const>(&Base::Name)), Scalar (Base::*)() const>::value, \
    "bounded integer conversion signature changed")
#define ASSERT_BASE_SCALAR1(Base, Name, Result, Scalar)                                          \
  static_assert(std::is_same<decltype(static_cast<Result (Base::*)(Scalar) const>(&Base::Name)), \
                             Result (Base::*)(Scalar) const>::value,                             \
                "bounded integer query signature changed")

ASSERT_BASE_COMPOUND(SignedBase, operator+=, SignedScalar);
ASSERT_BASE_COMPOUND(SignedBase, operator-=, SignedScalar);
ASSERT_BASE_COMPOUND(SignedBase, operator*=, SignedScalar);
ASSERT_BASE_COMPOUND(SignedBase, operator/=, SignedScalar);
ASSERT_BASE_COMPOUND(SignedBase, operator%=, SignedScalar);
ASSERT_BASE_COMPOUND(SignedBase, operator&=, SignedScalar);
ASSERT_BASE_COMPOUND(SignedBase, operator|=, SignedScalar);
ASSERT_BASE_COMPOUND(SignedBase, operator^=, SignedScalar);
ASSERT_BASE_COMPOUND(SignedBase, operator<<=, SignedScalar);
ASSERT_BASE_COMPOUND(SignedBase, operator>>=, SignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator+=, UnsignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator-=, UnsignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator*=, UnsignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator/=, UnsignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator%=, UnsignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator&=, UnsignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator|=, UnsignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator^=, UnsignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator<<=, UnsignedScalar);
ASSERT_BASE_COMPOUND(UnsignedBase, operator>>=, UnsignedScalar);

ASSERT_BASE_SCALAR_ASSIGNMENT(SignedBase, SignedScalar);
ASSERT_BASE_SCALAR_ASSIGNMENT(SignedBase, sc_dt::uint64);
ASSERT_BASE_SCALAR_ASSIGNMENT(SignedBase, int);
ASSERT_BASE_SCALAR_ASSIGNMENT(SignedBase, unsigned int);
ASSERT_BASE_SCALAR_ASSIGNMENT(SignedBase, long);
ASSERT_BASE_SCALAR_ASSIGNMENT(SignedBase, unsigned long);
ASSERT_BASE_SCALAR_ASSIGNMENT(SignedBase, double);
ASSERT_BASE_SCALAR_ASSIGNMENT(UnsignedBase, UnsignedScalar);
ASSERT_BASE_SCALAR_ASSIGNMENT(UnsignedBase, sc_dt::int64);
ASSERT_BASE_SCALAR_ASSIGNMENT(UnsignedBase, int);
ASSERT_BASE_SCALAR_ASSIGNMENT(UnsignedBase, unsigned int);
ASSERT_BASE_SCALAR_ASSIGNMENT(UnsignedBase, long);
ASSERT_BASE_SCALAR_ASSIGNMENT(UnsignedBase, unsigned long);
ASSERT_BASE_SCALAR_ASSIGNMENT(UnsignedBase, double);

ASSERT_BASE_ASSIGNMENT(SignedBase, SignedBase);
ASSERT_BASE_ASSIGNMENT(SignedBase, sc_dt::sc_signed);
ASSERT_BASE_ASSIGNMENT(SignedBase, sc_dt::sc_unsigned);
ASSERT_BASE_ASSIGNMENT(SignedBase, sc_dt::sc_int_subref_r);
ASSERT_BASE_ASSIGNMENT(SignedBase, sc_dt::sc_bv_base);
ASSERT_BASE_ASSIGNMENT(SignedBase, sc_dt::sc_lv_base);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, UnsignedBase);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, sc_dt::sc_signed);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, sc_dt::sc_unsigned);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, sc_dt::sc_uint_subref_r);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, sc_dt::sc_bv_base);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, sc_dt::sc_lv_base);
ASSERT_BASE_SCALAR_ASSIGNMENT(SignedBase, const char*);
ASSERT_BASE_SCALAR_ASSIGNMENT(UnsignedBase, const char*);

#ifdef SC_INCLUDE_FX
ASSERT_BASE_ASSIGNMENT(SignedBase, sc_dt::sc_fxval);
ASSERT_BASE_ASSIGNMENT(SignedBase, sc_dt::sc_fxval_fast);
ASSERT_BASE_ASSIGNMENT(SignedBase, sc_dt::sc_fxnum);
ASSERT_BASE_ASSIGNMENT(SignedBase, sc_dt::sc_fxnum_fast);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, sc_dt::sc_fxval);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, sc_dt::sc_fxval_fast);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, sc_dt::sc_fxnum);
ASSERT_BASE_ASSIGNMENT(UnsignedBase, sc_dt::sc_fxnum_fast);
#endif

ASSERT_BASE_CONCAT_SET(SignedBase, sc_dt::int64);
ASSERT_BASE_CONCAT_SET(SignedBase, sc_dt::uint64);
ASSERT_BASE_CONCAT_SET(SignedBase, const sc_dt::sc_signed&);
ASSERT_BASE_CONCAT_SET(SignedBase, const sc_dt::sc_unsigned&);
ASSERT_BASE_CONCAT_SET(UnsignedBase, sc_dt::int64);
ASSERT_BASE_CONCAT_SET(UnsignedBase, sc_dt::uint64);
ASSERT_BASE_CONCAT_SET(UnsignedBase, const sc_dt::sc_signed&);
ASSERT_BASE_CONCAT_SET(UnsignedBase, const sc_dt::sc_unsigned&);

#define ASSERT_BASE_ASSIGNMENT_SET(Base)            \
  ASSERT_BASE_ASSIGNMENT(Base, sc_dt::sc_signed);   \
  ASSERT_BASE_ASSIGNMENT(Base, sc_dt::sc_unsigned); \
  ASSERT_BASE_ASSIGNMENT(Base, sc_dt::sc_bv_base);  \
  ASSERT_BASE_ASSIGNMENT(Base, sc_dt::sc_lv_base)
ASSERT_BASE_ASSIGNMENT_SET(SignedBase);
ASSERT_BASE_ASSIGNMENT_SET(UnsignedBase);
#undef ASSERT_BASE_ASSIGNMENT_SET

ASSERT_BASE_SCALAR0(SignedBase, value, SignedScalar);
ASSERT_BASE_SCALAR0(UnsignedBase, value, UnsignedScalar);
ASSERT_BASE_CONST0(SignedBase, length, int);
ASSERT_BASE_CONST0(UnsignedBase, length, int);
ASSERT_BASE_CONST0(SignedBase, concat_get_uint64, sc_dt::uint64);
ASSERT_BASE_CONST0(UnsignedBase, concat_get_uint64, sc_dt::uint64);
ASSERT_BASE_CONST0(SignedBase, and_reduce, bool);
ASSERT_BASE_CONST0(SignedBase, nand_reduce, bool);
ASSERT_BASE_CONST0(SignedBase, or_reduce, bool);
ASSERT_BASE_CONST0(SignedBase, nor_reduce, bool);
ASSERT_BASE_CONST0(SignedBase, xor_reduce, bool);
ASSERT_BASE_CONST0(SignedBase, xnor_reduce, bool);
ASSERT_BASE_CONST0(UnsignedBase, and_reduce, bool);
ASSERT_BASE_CONST0(UnsignedBase, nand_reduce, bool);
ASSERT_BASE_CONST0(UnsignedBase, or_reduce, bool);
ASSERT_BASE_CONST0(UnsignedBase, nor_reduce, bool);
ASSERT_BASE_CONST0(UnsignedBase, xor_reduce, bool);
ASSERT_BASE_CONST0(UnsignedBase, xnor_reduce, bool);
ASSERT_BASE_CONST0(SignedBase, to_int, int);
ASSERT_BASE_CONST0(SignedBase, to_uint, unsigned int);
ASSERT_BASE_CONST0(SignedBase, to_long, long);
ASSERT_BASE_CONST0(SignedBase, to_ulong, unsigned long);
ASSERT_BASE_CONST0(SignedBase, to_int64, sc_dt::int64);
ASSERT_BASE_CONST0(SignedBase, to_uint64, sc_dt::uint64);
ASSERT_BASE_CONST0(SignedBase, to_double, double);
ASSERT_BASE_CONST0(SignedBase, long_low, long);
ASSERT_BASE_CONST0(SignedBase, long_high, long);
ASSERT_BASE_CONST0(UnsignedBase, to_int, int);
ASSERT_BASE_CONST0(UnsignedBase, to_uint, unsigned int);
ASSERT_BASE_CONST0(UnsignedBase, to_long, long);
ASSERT_BASE_CONST0(UnsignedBase, to_ulong, unsigned long);
ASSERT_BASE_CONST0(UnsignedBase, to_int64, sc_dt::int64);
ASSERT_BASE_CONST0(UnsignedBase, to_uint64, sc_dt::uint64);
ASSERT_BASE_CONST0(UnsignedBase, to_double, double);
ASSERT_BASE_CONST0(UnsignedBase, long_low, long);
ASSERT_BASE_CONST0(UnsignedBase, long_high, long);

ASSERT_BASE_SCALAR1(SignedBase, test, bool, int);
ASSERT_BASE_SCALAR1(UnsignedBase, test, bool, int);
static_assert(
  std::is_same<decltype(static_cast<void (SignedBase::*)(int)>(&SignedBase::set)), void (SignedBase::*)(int)>::value,
  "sc_int_base set(int) signature changed");
static_assert(std::is_same<decltype(static_cast<void (SignedBase::*)(int, bool)>(&SignedBase::set)),
                           void (SignedBase::*)(int, bool)>::value,
              "sc_int_base set(int, bool) signature changed");
static_assert(std::is_same<decltype(static_cast<void (UnsignedBase::*)(int)>(&UnsignedBase::set)),
                           void (UnsignedBase::*)(int)>::value,
              "sc_uint_base set(int) signature changed");
static_assert(std::is_same<decltype(static_cast<void (UnsignedBase::*)(int, bool)>(&UnsignedBase::set)),
                           void (UnsignedBase::*)(int, bool)>::value,
              "sc_uint_base set(int, bool) signature changed");

static_assert(std::is_same<decltype(static_cast<sc_dt::sc_int_bitref& (SignedBase::*)(int)>(&SignedBase::operator[])),
                           sc_dt::sc_int_bitref& (SignedBase::*)(int)>::value,
              "sc_int_base mutable operator[] signature changed");
static_assert(
  std::is_same<decltype(static_cast<const sc_dt::sc_int_bitref_r& (SignedBase::*)(int) const>(&SignedBase::operator[])),
               const sc_dt::sc_int_bitref_r& (SignedBase::*)(int) const>::value,
  "sc_int_base const operator[] signature changed");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_uint_bitref& (UnsignedBase::*)(int)>(&UnsignedBase::operator[])),
               sc_dt::sc_uint_bitref& (UnsignedBase::*)(int)>::value,
  "sc_uint_base mutable operator[] signature changed");
static_assert(std::is_same<decltype(static_cast<const sc_dt::sc_uint_bitref_r& (UnsignedBase::*)(int) const>(
                             &UnsignedBase::operator[])),
                           const sc_dt::sc_uint_bitref_r& (UnsignedBase::*)(int) const>::value,
              "sc_uint_base const operator[] signature changed");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_int_bitref& (SignedBase::*)(int)>(&SignedBase::bit)),
                           sc_dt::sc_int_bitref& (SignedBase::*)(int)>::value,
              "sc_int_base mutable bit signature changed");
static_assert(
  std::is_same<decltype(static_cast<const sc_dt::sc_uint_bitref_r& (UnsignedBase::*)(int) const>(&UnsignedBase::bit)),
               const sc_dt::sc_uint_bitref_r& (UnsignedBase::*)(int) const>::value,
  "sc_uint_base const bit signature changed");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_int_subref& (SignedBase::*)(int, int)>(&SignedBase::operator())),
               sc_dt::sc_int_subref& (SignedBase::*)(int, int)>::value,
  "sc_int_base mutable operator() signature changed");
static_assert(std::is_same<decltype(static_cast<const sc_dt::sc_uint_subref_r& (UnsignedBase::*)(int, int) const>(
                             &UnsignedBase::operator())),
                           const sc_dt::sc_uint_subref_r& (UnsignedBase::*)(int, int) const>::value,
              "sc_uint_base const operator() signature changed");

static_assert(std::is_constructible<SignedBase, const sc_dt::sc_int_subref_r&>::value &&
                std::is_constructible<SignedBase, const sc_dt::sc_uint_subref_r&>::value &&
                std::is_constructible<SignedBase, const sc_dt::sc_signed_subref_r&>::value &&
                std::is_constructible<SignedBase, const sc_dt::sc_unsigned_subref_r&>::value,
              "sc_int_base range constructors remain public");
static_assert(std::is_constructible<UnsignedBase, const sc_dt::sc_int_subref_r&>::value &&
                std::is_constructible<UnsignedBase, const sc_dt::sc_uint_subref_r&>::value &&
                std::is_constructible<UnsignedBase, const sc_dt::sc_signed_subref_r&>::value &&
                std::is_constructible<UnsignedBase, const sc_dt::sc_unsigned_subref_r&>::value,
              "sc_uint_base range constructors remain public");
static_assert(std::is_constructible<SignedBase, const GenericSource&>::value &&
                std::is_constructible<UnsignedBase, const GenericSource&>::value,
              "bounded generic-base constructors remain public");
static_assert(
  std::is_same<decltype(std::declval<SignedBase&>() = std::declval<const GenericSource&>()), SignedBase&>::value,
  "sc_int_base generic assignment remains public");
static_assert(
  std::is_same<decltype(std::declval<UnsignedBase&>() = std::declval<const GenericSource&>()), UnsignedBase&>::value,
  "sc_uint_base generic assignment remains public");

static_assert(std::is_constructible<SignedBase, int>::value, "sc_int_base width constructor is public");
static_assert(std::is_constructible<SignedBase, SignedScalar, int>::value,
              "sc_int_base value-width constructor is public");
static_assert(std::is_constructible<UnsignedBase, int>::value, "sc_uint_base width constructor is public");
static_assert(std::is_constructible<UnsignedBase, UnsignedScalar, int>::value,
              "sc_uint_base value-width constructor is public");
static_assert(std::is_copy_constructible<SignedBase>::value && std::is_copy_assignable<SignedBase>::value,
              "sc_int_base value copy is public");
static_assert(std::is_copy_constructible<UnsignedBase>::value && std::is_copy_assignable<UnsignedBase>::value,
              "sc_uint_base value copy is public");
static_assert(std::is_constructible<SignedBase, const sc_dt::sc_signed&>::value &&
                std::is_constructible<SignedBase, const sc_dt::sc_unsigned&>::value &&
                std::is_constructible<SignedBase, const sc_dt::sc_bv_base&>::value &&
                std::is_constructible<SignedBase, const sc_dt::sc_lv_base&>::value,
              "sc_int_base native value constructors are public");
static_assert(std::is_constructible<UnsignedBase, const sc_dt::sc_signed&>::value &&
                std::is_constructible<UnsignedBase, const sc_dt::sc_unsigned&>::value &&
                std::is_constructible<UnsignedBase, const sc_dt::sc_bv_base&>::value &&
                std::is_constructible<UnsignedBase, const sc_dt::sc_lv_base&>::value,
              "sc_uint_base native value constructors are public");

/** @brief Return the const signed bit-reference overload while retaining owner. */
inline const sc_dt::sc_int_bitref_r& const_signed_bit(const SignedBase& owner, int index)
{
  return owner[index];
}

/** @brief Return the const unsigned bit-reference overload while retaining owner. */
inline const sc_dt::sc_uint_bitref_r& const_unsigned_bit(const UnsignedBase& owner, int index)
{
  return owner[index];
}

/** @brief Return the const signed bit() overload while retaining owner. */
inline const sc_dt::sc_int_bitref_r& const_signed_bit_method(const SignedBase& owner, int index)
{
  return owner.bit(index);
}

/** @brief Return the const unsigned bit() overload while retaining owner. */
inline const sc_dt::sc_uint_bitref_r& const_unsigned_bit_method(const UnsignedBase& owner, int index)
{
  return owner.bit(index);
}

/** @brief Return the const signed range overload while retaining owner. */
inline const sc_dt::sc_int_subref_r& const_signed_range(const SignedBase& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Return the const unsigned range overload while retaining owner. */
inline const sc_dt::sc_uint_subref_r& const_unsigned_range(const UnsignedBase& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Return a const signed arbitrary-width range with a live owner. */
inline const sc_dt::sc_signed_subref_r& const_signed_range(const sc_dt::sc_signed& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Return a const unsigned arbitrary-width range with a live owner. */
inline const sc_dt::sc_unsigned_subref_r& const_unsigned_range(const sc_dt::sc_unsigned& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Return a const bounded signed range with a live owner. */
inline const sc_dt::sc_int_subref_r& const_int_range(const sc_dt::sc_int_base& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Return a const bounded unsigned range with a live owner. */
inline const sc_dt::sc_uint_subref_r& const_uint_range(const sc_dt::sc_uint_base& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Return the const signed operator() overload while retaining owner. */
inline const sc_dt::sc_int_subref_r& const_signed_call(const SignedBase& owner, int high, int low)
{
  return owner(high, low);
}

/** @brief Return the const unsigned operator() overload while retaining owner. */
inline const sc_dt::sc_uint_subref_r& const_unsigned_call(const UnsignedBase& owner, int high, int low)
{
  return owner(high, low);
}

/** @brief Change SC_ERROR to SC_THROW for one-shot failure probes. */
inline sc_core::sc_actions throw_errors()
{
  return sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
}

/** @brief Return the native report identifier used for invalid integer widths. */
inline std::string out_of_bounds_msg_type()
{
  return sc_core::SC_ID_OUT_OF_BOUNDS_;
}

/** @brief Restore the SC_ERROR action captured by throw_errors(). */
inline void restore_errors(sc_core::sc_actions previous)
{
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
}

#undef ASSERT_BASE_SCALAR_OPERATOR
#undef ASSERT_BASE_ASSIGNMENT
#undef ASSERT_BASE_SCALAR_ASSIGNMENT
#undef ASSERT_BASE_COMPOUND
#undef ASSERT_BASE_CONCAT_SET
#undef ASSERT_BASE_CONST0
#undef ASSERT_BASE_SCALAR0
#undef ASSERT_BASE_SCALAR1
}  // namespace sc_dt_bounded_bases_probe
