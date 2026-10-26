/** @file Exact native concatenation selection for boolean bit operands. */
#pragma once

#include <string>

namespace pysystemc_datatypes
{
/** @brief Supply Python text through call-local native C-string storage.
 * @details Native concatenation copies the text into an owned vector while
 * borrowing the other operand's parent. Keep that parent live. Calls hold the
 * GIL, retain no Python text references and propagate native allocation and
 * conversion reports. All native vector widths and states remain available.
 */
template <class Owner>
struct ConcatText
{
  /** @brief Concatenate a borrowed native operand and copied text. */
  static auto tail(Owner& owner, const std::string& text)
  {
    return sc_dt::concat(owner, text.c_str());
  }
  /** @brief Concatenate copied text and a borrowed native operand. */
  static auto head(const std::string& text, Owner& owner)
  {
    return sc_dt::concat(text.c_str(), owner);
  }
};

/** @brief Select defined native boolean/bit concatenation overloads.
 * @details SystemC also declares incompatible, undefined sc_lv_base result
 * overloads for these parameters. Exact function pointers select the defined
 * sc_bv_base result. Returned shells own cloned views and boolean storage;
 * the bit's parent remains borrowed and must stay live. Calls hold the GIL,
 * register no callbacks and propagate native allocation/report exceptions.
 */
template <class Bit, class Owner>
struct BoolBitConcatNative
{
  /** @brief Concatenate a borrowed bit followed by a copied boolean. */
  static auto tail(Bit bit, bool value)
  {
    using Result = sc_dt::sc_concref_r<sc_dt::sc_bitref_r<Owner>, sc_dt::sc_bv_base>;
    auto function = static_cast<Result (*)(Bit, bool)>(&sc_dt::concat);
    return function(bit, value);
  }

  /** @brief Concatenate a copied boolean followed by a borrowed bit. */
  static auto head(bool value, Bit bit)
  {
    using Result = sc_dt::sc_concref_r<sc_dt::sc_bv_base, sc_dt::sc_bitref_r<Owner>>;
    auto function = static_cast<Result (*)(bool, Bit)>(&sc_dt::concat);
    return function(value, bit);
  }
};

/** @brief Dispatch boolean concatenation for an arbitrary native bit proxy. */
template <class Bit>
struct BoolBitConcat;

/** @brief Preserve mutable bit input and native read-only result semantics. */
template <class Owner>
struct BoolBitConcat<sc_dt::sc_bitref<Owner>> : BoolBitConcatNative<sc_dt::sc_bitref<Owner>, Owner>
{
};

/** @brief Preserve read-only bit input and native read-only result semantics. */
template <class Owner>
struct BoolBitConcat<sc_dt::sc_bitref_r<Owner>> : BoolBitConcatNative<sc_dt::sc_bitref_r<Owner>, Owner>
{
};
}  // namespace pysystemc_datatypes
