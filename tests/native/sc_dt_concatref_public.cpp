/** @file sc_dt_concatref_public.cpp
 * @brief Native reference for concatenation shells, numeric values and lifetimes.
 * @details Every shell borrows live operands. Only explicit numeric snapshots
 * own their data. Test pool destruction never owns leaves. No callbacks, waits,
 * Python references or kernel resets occur; native reports propagate except in
 * the deliberate invalid-text probe. The Python counterpart holds the GIL.
 */
#include <iostream>
#include <stdexcept>

#include "sc_dt_concatref_probe.h"

namespace
{
/** @brief Emit a stable native-comparable record. */
template <class... Values>
void emit(const char* label, const Values&... values)
{
  std::cout << "CCR\t" << label;
  using swallow = int[];
  (void)swallow{0, ((std::cout << '\t' << values), 0)...};
  std::cout << '\n';
}
/** @brief Copy the live shell's full-width unsigned text. */
std::string render(const sc_dt::sc_concatref& n)
{
  return n.to_string(sc_dt::SC_DEC);
}
/** @brief Emit an exact assignment result and native self-reference. */
template <class Source>
void assignment(sc_dt::sc_concatref& n, const char* label, const Source& value)
{
  n = "0";
  const auto& result = (n = value);
  emit("assign", label, render(n), int(&result == &n));
}
/** @brief Emit one exact concat_set overload at an offset. */
template <class Source>
void set(sc_dt::sc_concatref& n, const char* label, const Source& value, int low)
{
  n = "0";
  n.concat_set(value, low);
  emit("set", label, low, render(n));
}
/** @brief Pack into sentinel-filled native digits at a nonzero offset. */
void packed(const sc_dt::sc_concatref& n, const char* label, int low = 7)
{
  for (bool control : {false, true})
  {
    std::vector<sc_dt::sc_digit> words(concatref_probe::packing_digits(n, low), 0x15555555U);
    bool result = control ? n.concat_get_ctrl(words.data(), low) : n.concat_get_data(words.data(), low);
    std::ostringstream digits;
    for (std::size_t i = 0; i < words.size(); ++i)
    {
      if (i) digits << '\t';
      digits << words[i];
    }
    emit("pack", label, int(control), int(result), digits.str());
  }
}
/** @brief Exercise all scalar numeric conversions and exact string overloads. */
void conversions(const sc_dt::sc_concatref& n, const char* label)
{
  using namespace concatref_probe;
  emit("convert", label, n.to_uint64(), n.to_int64(), n.to_uint(), n.to_int(), n.to_ulong(), n.to_long(),
       implicit_integer(n), n.concat_get_uint64());
  emit("string", label, n.to_string(), n.to_string(sc_dt::SC_HEX), n.to_string(sc_dt::SC_BIN, false), printed(n));
  const sc_dt::sc_unsigned copy(n.value());
  emit("value", label, copy.length(), copy.to_string(), implicit_reference(n).to_string(),
       int(n.to_double() == copy.to_double()));
  sc_dt::sc_signed sign(349);
  sc_dt::sc_unsigned unsign(349);
  n.to_sc_signed(sign);
  n.to_sc_unsigned(unsign);
  emit("output", label, sign.to_string(), unsign.to_string());
  const auto positive = +n;
  const auto negative = -n;
  const auto complement = ~n;
  emit("unary", label, positive.length(), positive.to_string(), negative.length(), negative.to_string(),
       complement.length(), complement.to_string());
}
/** @brief Emit all six reduction member results. */
void reductions(const sc_dt::sc_concatref& n, const char* label)
{
  emit("reduce", label, int(n.and_reduce()), int(n.nand_reduce()), int(n.or_reduce()), int(n.nor_reduce()),
       int(n.xor_reduce()), int(n.xnor_reduce()));
}
}  // namespace

/** @brief Run all probes with fresh native singleton and temporary pools. */
int sc_main(int, char**)
{
  using namespace concatref_probe;
  sc_dt::sc_signed left(137);
  left = "0x123456789abcdef123456789abcdef";
  sc_dt::sc_unsigned right(191);
  right = "0xabcdef00112233445566778899aabbccddeeff";
  auto& n = select(left, right);
  emit("capacity", capacity(), int(traits()), n.length(), native_bitwidth(n), n.concat_length(nullptr), int(has_xz(n)),
       int(sc_dt::sc_concatref::cf_none), int(sc_dt::sc_concatref::cf_xz_present));
  auto& cn = const_select(left, right);
  emit("const", cn.length(), render(cn));
  conversions(n, "wide");
  packed(n, "wide");
  sc_dt::sc_uint<11> small_left(0x555);
  sc_dt::sc_uint<7> small_right(0x53);
  auto& small = select(small_left, small_right);
  conversions(small, "small");
  sc_dt::sc_signed signed_source(349);
  signed_source = "-81985529216486895";
  sc_dt::sc_unsigned unsigned_source(349);
  unsigned_source = "0x123456789abcdef001122334455667788";
  sc_dt::sc_unsigned source_left(173);
  source_left = "0x13579abcdef";
  sc_dt::sc_unsigned source_right(155);
  source_right = "0xfedcba98765";
  auto& source_concat = select(source_left, source_right);
  sc_dt::sc_bv_base bv("101001001101011001011001110101011");
  sc_dt::sc_lv_base lv("101101001101011001011001110101011");
  assignment(n, "int", int(-13579));
  assignment(n, "long", long(-0x123456789L));
  assignment(n, "int64", sc_dt::int64(-0x123456789ABCDELL));
  assignment(n, "unsigned_int", unsigned(0xA5A55A5A));
  assignment(n, "unsigned_long", static_cast<unsigned long>(0x123456789ABCDEFUL));
  assignment(n, "uint64", sc_dt::uint64(0xFEDCBA9876543210ULL));
  assignment(n, "concatref", source_concat);
  assignment(n, "signed", signed_source);
  assignment(n, "unsigned", unsigned_source);
  assignment(n, "text", "0x123456789abcdef00123456789");
  assignment(n, "bv", bv);
  assignment(n, "lv", lv);
  for (int low : {0, 9})
  {
    set(n, "int64", sc_dt::int64(-0x123456789ABCDELL), low);
    set(n, "uint64", sc_dt::uint64(0xFEDCBA9876543210ULL), low);
    set(n, "signed", signed_source, low);
    set(n, "unsigned", unsigned_source, low);
  }
  small = "0";
  reductions(small, "zero");
  small = "0x3ffff";
  reductions(small, "ones");
  small = "7";
  reductions(small, "odd");
  small = "3";
  reductions(small, "even");
  std::istringstream input("0x13579 extra");
  small.scan(input);
  emit("scan", render(small));
  bool rejected = false;
  const auto previous = render(small);
  const auto old = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  try
  {
    small = "invalid-number";
  }
  catch (const sc_core::sc_report&)
  {
    rejected = true;
  }
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, old);
  emit("error", int(rejected), int(render(small) == previous));
  sc_dt::sc_uint<8> overlap_left(0xA5), overlap_right(0x3C);
  auto& overlap = select(overlap_left, overlap_right);
  auto& reverse = select(overlap_right, overlap_left);
  overlap = reverse;
  emit("overlap", overlap_left.to_uint(), overlap_right.to_uint(), render(overlap));
  LogicLeaf logic_left("10XZ01"), logic_right("Z110");
  auto& logic = select(logic_left, logic_right);
  emit("logic", logic.length(), int(has_xz(logic)), logic.concat_get_uint64(), render(logic));
  packed(logic, "xz");
  packed(logic, "xz_cross", 27);
  logic.concat_clear_data(false);
  emit("clear", 0, logic_left.bits, logic_right.bits, int(has_xz(logic)));
  packed(logic, "clear0");
  logic.concat_clear_data(true);
  emit("clear", 1, logic_left.bits, logic_right.bits, int(has_xz(logic)));
  packed(logic, "clear1");
  logic.initialize(logic_left, logic_right);
  emit("reinit", int(has_xz(logic)), logic.length());
  auto& retained = select(small_left, small_right);
  const auto stable = snapshot(retained);
  const auto positive = +retained;
  const auto negative = -retained;
  const auto complement = ~retained;
  sc_dt::sc_uint<11> reuse_left(7);
  sc_dt::sc_uint<7> reuse_right(9);
  sc_dt::sc_concatref* last = nullptr;
  for (std::size_t i = 0; i < capacity(); ++i) last = &select(reuse_left, reuse_right);
  emit("shell_wrap", int(last == &retained), render(retained), stable.to_string(), positive.to_string(),
       negative.to_string(), complement.to_string());
  sc_dt::sc_uint<8> value_left(45), value_right(67);
  auto& value_node = select(value_left, value_right);
  const auto& borrowed = retained.value();
  const sc_dt::sc_unsigned owned(borrowed);
  const sc_dt::sc_unsigned* last_value = nullptr;
  for (std::size_t i = 0; i < value_capacity(); ++i) last_value = &value_node.value();
  emit("value_wrap", value_capacity(), int(last_value == &borrowed), borrowed.to_string(), owned.to_string());
  const int before = LogicLeaf::destroyed;
  sc_dt::sc_unsigned pool_copy(10);
  {
    OwnedPool pool;
    auto* pooled = pool.allocate();
    pooled->initialize(logic_left, logic_right);
    pool_copy = pooled->value();
  }
  emit("destruction", int(LogicLeaf::destroyed == before), pool_copy.to_string());
  std::unique_ptr<sc_dt::sc_unsigned> surviving_positive, surviving_complement;
  std::unique_ptr<sc_dt::sc_signed> surviving_negative;
  sc_dt::sc_unsigned surviving(16);
  sc_dt::sc_unsigned surviving_unary(16);
  {
    sc_dt::sc_uint<8> a(0xA5), b(0x3C);
    auto& live = select(a, b);
    surviving = live.value();
    surviving_unary = +live;
    surviving_positive.reset(new sc_dt::sc_unsigned(+live));
    surviving_negative.reset(new sc_dt::sc_signed(-live));
    surviving_complement.reset(new sc_dt::sc_unsigned(~live));
  }
  emit("owners_gone", surviving.to_string(), surviving_unary.to_string(), surviving_positive->length(),
       surviving_positive->to_string(), surviving_negative->length(), surviving_negative->to_string(),
       surviving_complement->length(), surviving_complement->to_string());
  return 0;
}
