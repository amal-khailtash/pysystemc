/** @file range_alias_edges.cpp
 * @brief Compare legacy SystemC datatype aliases with Python-facing aliases.
 * @details The model uses one fresh process, keeps every range parent alive,
 *          and verifies copied vectors after parent mutation. Calls are
 *          synchronous with no callbacks, GIL use, or kernel reset.
 */
#include <iostream>
#include <string>
#include <systemc>

#include "sc_dt_concatref_probe.h"
#include "sc_dt_fixed_subrefs_probe.h"

namespace
{

/** @brief Emit one deterministic semantic observation for the Python comparator. */
template <typename... Fields>
void emit(const Fields&... fields)
{
  std::cout << "RANGEALIAS";
  ((std::cout << '\t' << fields), ...);
  std::cout << '\n';
}

/** @brief Use the native deprecated width alias when SystemC exposes it. */
template <typename View>
int native_bitwidth(const View& view)
{
#ifdef SC_DT_DEPRECATED
  return view.bitwidth();
#else
  return view.length();
#endif
}

/** @brief Emit fixed-point and arbitrary-width vector-copy alias observations. */
void fixed_aliases()
{
  sc_dt::sc_fixed<137, 69> owner(-13.375);
  auto mutable_view = owner.range(103, 96);
  auto const_view = sc_dt_fixed_subrefs_probe::fixed_const_range(owner, 103, 96);
  emit("fixed", "mutable_before", mutable_view.length(), mutable_view.to_int(), mutable_view.to_uint());
  emit("fixed", "const_before", const_view.length(), const_view.to_int(), const_view.to_uint());
  owner.range(103, 96) = 0x5A;
  emit("fixed", "mutable_after", mutable_view.length(), mutable_view.to_int(), mutable_view.to_uint());
  emit("fixed", "const_after", const_view.length(), const_view.to_int(), const_view.to_uint());

  sc_dt::sc_fixed_fast<137, 69> fast(-13.375);
  auto fast_mutable = fast.range(103, 96);
  auto fast_const = sc_dt_fixed_subrefs_probe::fast_const_range(fast, 103, 96);
  emit("fast", "mutable_before", fast_mutable.length(), fast_mutable.to_int(), fast_mutable.to_uint());
  emit("fast", "const_before", fast_const.length(), fast_const.to_int(), fast_const.to_uint());
  fast.range(103, 96) = 0x5A;
  emit("fast", "mutable_after", fast_mutable.length(), fast_mutable.to_int(), fast_mutable.to_uint());
  emit("fast", "const_after", fast_const.length(), fast_const.to_int(), fast_const.to_uint());

  auto whole = owner.range(136, 0);
  const sc_dt::sc_bv_base base_copy(whole);
  const sc_dt::sc_bv<137> template_copy(base_copy);
  const auto base_bits = base_copy.to_string(sc_dt::SC_BIN, true);
  const auto template_bits = template_copy.to_string(sc_dt::SC_BIN, true);
  whole = 0;
  emit("vector_copy", base_copy.length(), base_bits, template_copy.length(), template_bits,
       whole.to_string(sc_dt::SC_BIN, true), base_copy.to_string(sc_dt::SC_BIN, true),
       template_copy.to_string(sc_dt::SC_BIN, true));
}

/** @brief Emit legacy conversion aliases at non-precompiled integer widths. */
void integer_owner_aliases()
{
  sc_dt::sc_signed signed_owner(137);
  signed_owner = "-81985529216486895";
  emit("signed_owner", "before", signed_owner.length(), signed_owner.to_int(), signed_owner.to_uint());
  signed_owner = "1234567890123456789012345";
  emit("signed_owner", "after", signed_owner.length(), signed_owner.to_int(), signed_owner.to_uint());

  sc_dt::sc_unsigned unsigned_owner(191);
  unsigned_owner = "0x123456789abcdef001122334455667788";
  emit("unsigned_owner", "before", unsigned_owner.length(), unsigned_owner.to_int(), unsigned_owner.to_uint());
  unsigned_owner = "0xfedcba98765432100123456789abcdef";
  emit("unsigned_owner", "after", unsigned_owner.length(), unsigned_owner.to_int(), unsigned_owner.to_uint());
}

/** @brief Emit bounded integer range/bit aliases before and after owner writes. */
void integer_reference_aliases()
{
  sc_dt::sc_int<37> signed_owner(-0x1234567);
  auto signed_range = signed_owner(29, 17);
  auto signed_bit = signed_owner[9];
  emit("int_range", "before", native_bitwidth(signed_range), signed_range.length(), signed_range.to_int());
  emit("int_bit", "before", native_bitwidth(signed_bit), signed_bit.length(), signed_bit.to_bool());
  signed_owner = 0x123456789;
  emit("int_range", "after", native_bitwidth(signed_range), signed_range.length(), signed_range.to_int());
  emit("int_bit", "after", native_bitwidth(signed_bit), signed_bit.length(), signed_bit.to_bool());

  sc_dt::sc_uint<53> unsigned_owner(0x123456789ABCDULL);
  auto unsigned_range = unsigned_owner(47, 31);
  auto unsigned_bit = unsigned_owner[41];
  emit("uint_range", "before", native_bitwidth(unsigned_range), unsigned_range.length(), unsigned_range.to_uint());
  emit("uint_bit", "before", native_bitwidth(unsigned_bit), unsigned_bit.length(), unsigned_bit.to_bool());
  unsigned_owner = 0x1ABCDE1234567ULL;
  emit("uint_range", "after", native_bitwidth(unsigned_range), unsigned_range.length(), unsigned_range.to_uint());
  emit("uint_bit", "after", native_bitwidth(unsigned_bit), unsigned_bit.length(), unsigned_bit.to_bool());
}

/** @brief Emit width alias results while concatref borrows changing operands. */
void concat_alias()
{
  sc_dt::sc_unsigned left(137);
  left = "0x123456789abcdef";
  sc_dt::sc_unsigned right(191);
  right = "0xabcdef00112233445566778899aabbccddeeff";
  auto& node = concatref_probe::select(left, right);
  emit("concat", "before", native_bitwidth(node), node.length(), node.to_string(sc_dt::SC_HEX));
  left = "0xfedcba987654321";
  right = "0x112233445566778899aabbccddeeff";
  emit("concat", "after", native_bitwidth(node), node.length(), node.to_string(sc_dt::SC_HEX));
}

}  // namespace

/** @brief Run the bounded alias matrix with live owners in a fresh process. */
int sc_main(int, char**)
{
  fixed_aliases();
  integer_owner_aliases();
  integer_reference_aliases();
  concat_alias();
  return 0;
}
