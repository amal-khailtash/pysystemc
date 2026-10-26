/** @file sc_dt_concat_functions_public.cpp
 * @brief Native reference for public sc_dt concat and comma declarations.
 * @details Vector proxies borrow live owners and nested proxies. Integer
 *          concatenation shells borrow value operands and use the native pool.
 *          This process retains all parents until observations finish; it
 *          creates no callbacks, Python references, or scheduler state.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>

#include "sc_dt_concat_functions_probe.h"
#include "sc_dt_concat_matrix_probe.h"
#include "sc_dt_proxy_probe.h"

namespace
{

using namespace sc_dt_concat_functions_probe;
using BV = sc_dt::sc_bv_base;
using LV = sc_dt::sc_lv_base;
using BVBit = sc_dt::sc_bitref<BV>;
using BVBitR = sc_dt::sc_bitref_r<BV>;
using LVRange = sc_dt::sc_subref<LV>;
using LVRangeR = sc_dt::sc_subref_r<LV>;
using MutablePair = sc_dt::sc_concref<BV, LV>;
using ReadonlyPair = sc_dt::sc_concref_r<BV, LV>;
using MutableNested = sc_dt::sc_concref<MutablePair, BVBit>;
using ReadonlyNested = sc_dt::sc_concref_r<ReadonlyPair, BVBitR>;
using MutableBitRange = sc_dt::sc_concref<BVBit, LVRange>;
using ReadonlyBitRange = sc_dt::sc_concref_r<BVBitR, LVRangeR>;
using Custom = sc_dt_proxy_probe::CustomProxy;

/** @brief Emit one stable tab-separated result record. */
template <typename... Values>
void emit(const char* label, const Values&... values)
{
  std::cout << "SCF\t" << label;
  using swallow = int[];
  (void)swallow{0, ((std::cout << '\t' << values), 0)...};
  std::cout << '\n';
}

static_assert(std::is_same<decltype(sc_dt::concat(std::declval<BV&>(), std::declval<LV&>())), MutablePair>::value,
              "mutable concat result");
static_assert(std::is_same<decltype(sc_dt::operator,(std::declval<BV&>(), std::declval<LV&>())), MutablePair>::value,
              "mutable comma result");
static_assert(
  std::is_same<decltype(sc_dt::concat(std::declval<const BV&>(), std::declval<const LV&>())), ReadonlyPair>::value,
  "const concat result");
static_assert(
  std::is_same<decltype(sc_dt::operator,(std::declval<const BV&>(), std::declval<const LV&>())), ReadonlyPair>::value,
  "const comma result");
static_assert(
  std::is_same<decltype(sc_dt::concat(std::declval<BVBit>(), std::declval<LVRange>())), MutableBitRange>::value,
  "mutable nested proxy result");
static_assert(
  std::is_same<decltype(sc_dt::concat(std::declval<BVBitR>(), std::declval<LVRangeR>())), ReadonlyBitRange>::value,
  "read-only nested proxy result");
static_assert(
  std::is_same<decltype(sc_dt::concat(std::declval<MutablePair&>(), std::declval<BVBit>())), MutableNested>::value,
  "nested mutable result");
static_assert(std::is_same<decltype(sc_dt::concat(std::declval<const ReadonlyPair&>(), std::declval<BVBitR>())),
                           ReadonlyNested>::value,
              "nested read-only result");
static_assert(std::is_same<decltype(sc_dt::concat(std::declval<Custom&>(), std::declval<BV&>())),
                           sc_dt::sc_concref<Custom, BV>>::value,
              "custom CRTP result");
static_assert(std::is_same<decltype(sc_dt::concat(std::declval<BV&>(), "101")), sc_dt::sc_concref_r<BV, LV>>::value,
              "string source result");
static_assert(std::is_same<decltype(sc_dt::concat(std::declval<BV&>(), true)), sc_dt::sc_concref_r<BV, BV>>::value,
              "Boolean source result");
static_assert(std::is_same<decltype(sc_dt::concat(std::declval<BV&>(), std::declval<const sc_dt::sc_logic&>())),
                           sc_dt::sc_concref_r<BV, LV>>::value,
              "four-state source result");
static_assert(
  std::is_same<decltype(sc_dt::concat(std::declval<sc_dt::sc_unsigned&>(), true)), const sc_dt::sc_concatref&>::value,
  "integer concat shell result");
static_assert(
  std::is_same<decltype(sc_dt::operator,(std::declval<sc_dt::sc_value_base&>(), std::declval<sc_dt::sc_value_base&>())),
               sc_dt::sc_concatref&>::value,
  "mutable integer comma shell result");

/** @brief Exercise independent public native calls and their live views. */
void run()
{
  BV bv(137);
  bv = "0x123456789abcdef123456789abcdef";
  LV lv(191);
  lv = "10ZX01ZX001101101";

  auto writable = sc_dt::concat(bv, lv);
  emit("mutable", writable.length(), render(writable));
  sc_dt::sc_lv_base owned_snapshot(writable);
  const std::string snapshot_before = render(owned_snapshot);
  writable = "101001";
  emit("mutable_write", bv.to_string(), lv.to_string(), render(writable), snapshot_before, render(owned_snapshot));

  auto comma_writable = sc_dt::operator,(bv, lv);
  emit("comma_mutable", comma_writable.length(), render(comma_writable));

  const BV& const_bv = bv;
  const LV& const_lv = lv;
  auto readonly_bv = readonly_bv_range(const_bv, bv.length() - 1, 0);
  auto readonly_lv = readonly_lv_range(const_lv, lv.length() - 1, 0);
  auto readonly = sc_dt::concat(readonly_bv, readonly_lv);
  emit("readonly", readonly.length(), render(readonly));
  auto comma_readonly = sc_dt::operator,(readonly_bv, readonly_lv);
  emit("comma_readonly", comma_readonly.length(), render(comma_readonly));

  auto bit = bv.bit(3);
  auto range = lv.range(19, 7);
  auto nested_parts = sc_dt::concat(bit, range);
  emit("mutable_parts", nested_parts.length(), render(nested_parts));
  nested_parts = "101100101";
  emit("mutable_parts_write", bv.to_string(), lv.to_string(), render(nested_parts));

  auto bit_read = readonly_bv_range(const_bv, 3, 3);
  auto range_read = readonly_lv_range(const_lv, 19, 7);
  auto readonly_parts = sc_dt::concat(bit_read, range_read);
  emit("readonly_parts", readonly_parts.length(), render(readonly_parts));

  auto nested = sc_dt::concat(writable, bit);
  emit("nested", nested.length(), render(nested));
  auto nested_readonly = sc_dt::concat(readonly, bit_read);
  emit("nested_readonly", nested_readonly.length(), render(nested_readonly));

  auto text_tail = sc_dt::concat(bv, "10XZ01");
  emit("string_leaf", text_tail.length(), render(text_tail));
  const sc_dt::sc_logic logic(sc_dt::SC_LOGIC_Z);
  auto logic_tail = sc_dt::concat(lv, logic);
  emit("logic_leaf", logic_tail.length(), render(logic_tail));
  auto bool_tail = sc_dt::concat(bv, true);
  emit("bool_leaf", bool_tail.length(), render(bool_tail));
  auto bool_head = sc_dt::concat(false, lv);
  emit("bool_head", bool_head.length(), render(bool_head));

  Custom custom(191);
  custom = "10XZ01";
  auto custom_view = sc_dt::concat(custom, bv);
  emit("custom", custom_view.length(), render(custom_view));

  sc_dt::sc_signed signed_value(137);
  signed_value = "-123456789";
  sc_dt::sc_unsigned unsigned_value(191);
  unsigned_value = "0x123456789abcdef";
  const auto& integer_view = sc_dt::concat(signed_value, unsigned_value);
  emit("integer_concat", integer_view.length(), integer_view.to_string(sc_dt::SC_BIN));
  auto &integer_comma = sc_dt::operator,(signed_value, unsigned_value);
  emit("integer_comma", integer_comma.length(), integer_comma.to_string(sc_dt::SC_BIN));
  const auto &integer_bool = sc_dt::operator,(unsigned_value, true);
  emit("integer_bool", integer_bool.length(), integer_bool.to_string(sc_dt::SC_BIN));

  std::string parent_live;
  std::string parent_snapshot;
  {
    BV local_bv(137);
    local_bv = "0x123456789abcdef";
    LV local_lv(191);
    local_lv = "10ZX01ZX";
    auto local_view = sc_dt::concat(local_bv, local_lv);
    parent_live = render(local_view);
    sc_dt::sc_lv_base copy(local_view);
    parent_snapshot = render(copy);
  }
  emit("parent_lifetime", parent_live, parent_snapshot);

  run_signature_matrix();
}

}  // namespace

/** @brief Run probes in one fresh native process. */
int sc_main(int, char**)
{
  run();
  return 0;
}
