/** @file sc_dt_fixed_subrefs_public.cpp
 * @brief Native reference matrix for the four fixed-point range proxies.
 * @details Emits read-only and mutable range behavior, all assignment and
 *          compound sources, scans, concatenations, parent bits and reports.
 *          Proxy instances borrow their owner and own only the selected-bit
 *          cache; ordinary C++ scope destruction releases each cache.
 */
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>

#include "sc_dt_fixed_subrefs_probe.h"

namespace
{

template <typename... Fields>
void emit(const Fields&... fields)
{
  std::cout << "FXSUB";
  ((std::cout << '\t' << fields), ...);
  std::cout << '\n';
}

std::string flatten(std::string value)
{
  for (char& character : value)
  {
    if (character == '\n' || character == '\r') character = '|';
  }
  return value;
}

template <typename View>
void emit_view(const char* family, const char* owner, const char* label, int high, int low, const View& view)
{
  std::ostringstream printed;
  std::ostringstream dumped;
  view.print(printed);
  view.dump(dumped);
  const auto bits = static_cast<sc_dt::sc_bv_base>(view);
  emit("view", family, owner, label, high, low, view.length(), view.to_string(), view.to_string(sc_dt::SC_BIN, true),
       view.to_string(sc_dt::SC_DEC), view.to_int(), view.to_uint(), view.to_long(), view.to_ulong(), view.to_int64(),
       view.to_uint64(), int(view.and_reduce()), int(view.nand_reduce()), int(view.or_reduce()), int(view.nor_reduce()),
       int(view.xor_reduce()), int(view.xnor_reduce()), bits.to_string(sc_dt::SC_BIN, true), printed.str(),
       flatten(dumped.str()));
  emit("signed_compat", family, owner, label, view.to_int(), view.to_uint());
}

template <typename Number>
void exercise_reads(const char* family, const char* owner_name, const Number& owner, const int ranges[][2],
                    const char* const* labels, std::size_t range_count)
{
  for (std::size_t index = 0; index < range_count; ++index)
  {
    const auto view = owner.range(ranges[index][0], ranges[index][1]);
    emit_view(family, owner_name, labels[index], ranges[index][0], ranges[index][1], view);
  }
  const auto whole_call = owner();
  emit_view(family, owner_name, "whole_call", -1, -1, whole_call);
  const auto whole_range = owner.range();
  emit_view(family, owner_name, "whole_range", -1, -1, whole_range);
}

template <typename Number>
void exercise_accessors(const char* owner_name, Number& owner)
{
  using MutableView = decltype(owner.range(11, 4));
  using CallView = decltype(owner(11, 4));
  static_assert(std::is_same<MutableView, CallView>::value, "both range access paths select the same proxy family");
  const int high = 11;
  const int low = 4;

  auto range_view = owner.range(high, low);
  auto range_copy = MutableView(range_view);
  owner.range(high, low) = "10101010";
  emit("copy", owner_name, "range", high, low, range_copy.to_string(sc_dt::SC_BIN, true), owner.to_bin());

  auto call_view = owner(high, low);
  auto call_copy = MutableView(call_view);
  owner(high, low) = "01010101";
  emit("copy", owner_name, "call", high, low, call_copy.to_string(sc_dt::SC_BIN, true), owner.to_bin());

  auto range_whole = owner.range();
  auto call_whole = owner();
  emit("access", owner_name, "range_args", high, low, range_view.length(), range_view.to_string(sc_dt::SC_BIN, true));
  emit("access", owner_name, "call_args", high, low, call_view.length(), call_view.to_string(sc_dt::SC_BIN, true));
  emit("access", owner_name, "range_whole", -1, -1, range_whole.length(), range_whole.to_string(sc_dt::SC_BIN, true));
  emit("access", owner_name, "call_whole", -1, -1, call_whole.length(), call_whole.to_string(sc_dt::SC_BIN, true));
}

template <typename Number>
void exercise_assignment_sources(const char* target_name, const char* source_owner_name)
{
  using Fixed = sc_dt::sc_fixed<16, 8>;
  using Fast = sc_dt::sc_fixed_fast<16, 8>;
  Fixed fixed_source(-5.25);
  Fast fast_source(-5.25);
  sc_dt::sc_bv_base bv_source("10100101");
  sc_dt::sc_lv_base lv_source("01011010");
  const char* character_source = "01101001";
  const bool bool_source[8] = {true, false, true, true, false, false, true, false};
  sc_dt::sc_signed signed_source(8);
  signed_source = -91;
  sc_dt::sc_unsigned unsigned_source(8);
  unsigned_source = 165;
  sc_dt::sc_int_base int_base_source(-91, 8);
  sc_dt::sc_uint_base uint_base_source(165, 8);
  const sc_dt::int64 int64_source = -91;
  const sc_dt::uint64 uint64_source = 165;
  const int int_source = -91;
  const unsigned int uint_source = 165;
  const long long_source = -91;
  const unsigned long ulong_source = 165;
  const char char_source = 'A';

  auto assign = [&](const char* name, const auto& source)
  {
    Number target(0);
    target.range(11, 4) = source;
    const auto view = target.range(11, 4);
    emit("assign", target_name, source_owner_name, name, view.to_string(sc_dt::SC_BIN, true), target.to_bin());
  };

  assign("fixed_subref", fixed_source.range(11, 4));
  assign("fast_subref", fast_source.range(11, 4));
  assign("sc_bv_base", bv_source);
  assign("sc_lv_base", lv_source);
  assign("char_pointer", character_source);
  assign("bool_pointer", bool_source);
  assign("sc_signed", signed_source);
  assign("sc_unsigned", unsigned_source);
  assign("sc_int_base", int_base_source);
  assign("sc_uint_base", uint_base_source);
  assign("int64", int64_source);
  assign("uint64", uint64_source);
  assign("int", int_source);
  assign("unsigned_int", uint_source);
  assign("long", long_source);
  assign("unsigned_long", ulong_source);
  assign("char", char_source);
}

template <typename Number>
void exercise_compound_sources(const char* target_name)
{
  using Fixed = sc_dt::sc_fixed<16, 8>;
  using Fast = sc_dt::sc_fixed_fast<16, 8>;
  Fixed fixed_source(-5.25);
  Fast fast_source(-5.25);
  sc_dt::sc_bv_base bv_source("10100101");
  sc_dt::sc_lv_base lv_source("01011010");

  for (const char operation : {'&', '|', '^'})
  {
    const char* operation_name = operation == '&' ? "and" : operation == '|' ? "or" : "xor";
    auto run = [&](const char* source_name, const auto& source)
    {
      Number target(-2.75);
      auto destination = target.range(11, 4);
      if (operation == '&')
        destination &= source;
      else if (operation == '|')
        destination |= source;
      else
        destination ^= source;
      emit("compound", target_name, operation_name, source_name, destination.to_string(sc_dt::SC_BIN, true),
           target.to_bin());
    };
    run("fixed_subref", fixed_source.range(11, 4));
    run("fast_subref", fast_source.range(11, 4));
    run("sc_bv_base", bv_source);
    run("sc_lv_base", lv_source);
  }
}

int error_mode(const std::string& mode)
{
  const auto previous = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  try
  {
    if (mode == "negative-fixed")
    {
      sc_dt::sc_fixed<16, 8> number(1);
      (void)number.range(-1, 0);
    }
    else if (mode == "upper-fast")
    {
      sc_dt::sc_fixed_fast<16, 8> number(1);
      (void)number.range(16, 0);
    }
    else
    {
      return 4;
    }
  }
  catch (const sc_core::sc_report& report)
  {
    emit("report", mode, report.get_msg_type(), report.get_msg());
    sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
    return 0;
  }
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
  return 5;
}

}  // namespace

/** @brief Emit range proxy reference observations.
 * @param argc Native argument count.
 * @param argv An optional report mode for a separate bounded error process.
 * @return Zero when the native matrix or selected report is emitted.
 */
int sc_main(int argc, char** argv)
{
  if (argc > 1) return error_mode(argv[1]);

  sc_dt::sc_fixed<137, 69> fixed(-13.375);
  sc_dt::sc_fixed_fast<64, 32> fast(-13.375);
  sc_dt::sc_fixed_fast<137, 100> fast_wide("9007199254740992.25");
  emit("width", "fixed137", fixed.wl(), fixed.iwl(), fixed.to_bin());
  emit("width", "fast64", fast.wl(), fast.iwl(), fast.to_bin());
  emit("width", "fast137", fast_wide.wl(), fast_wide.iwl(), fast_wide.to_bin());

  constexpr int fixed_ranges[][2]{{103, 96}, {96, 103}, {7, 0}, {72, 64}};
  constexpr const char* fixed_labels[]{"normal", "reversed", "fractional", "boundary"};
  exercise_reads("fixed_r", "fixed137", fixed, fixed_ranges, fixed_labels, 4);
  constexpr int fast_ranges[][2]{{48, 41}, {41, 48}, {7, 0}, {34, 28}};
  constexpr const char* fast_labels[]{"normal", "reversed", "fractional", "boundary"};
  exercise_reads("fast_r", "fast64", fast, fast_ranges, fast_labels, 4);
  constexpr int fast_wide_ranges[][2]{{85, 78}, {78, 85}, {30, 23}, {40, 34}};
  constexpr const char* fast_wide_labels[]{"normal", "reversed", "fractional", "boundary"};
  exercise_reads("fast_r", "fast137", fast_wide, fast_wide_ranges, fast_wide_labels, 4);

  exercise_accessors("fixed137", fixed);
  exercise_accessors("fast64", fast);
  exercise_accessors("fast137", fast_wide);

  exercise_assignment_sources<sc_dt::sc_fixed<16, 8>>("fixed", "fixed16");
  exercise_assignment_sources<sc_dt::sc_fixed_fast<16, 8>>("fast", "fast16");
  exercise_compound_sources<sc_dt::sc_fixed<16, 8>>("fixed");
  exercise_compound_sources<sc_dt::sc_fixed_fast<16, 8>>("fast");

  std::istringstream scan_stream("11010010");
  auto scan_fixed = fixed.range(11, 4);
  scan_fixed.scan(scan_stream);
  emit("scan", "fixed", scan_fixed.to_string(sc_dt::SC_BIN, true), fixed.to_bin());
  std::istringstream scan_fast_stream("00101101");
  auto scan_fast = fast.range(43, 36);
  scan_fast.scan(scan_fast_stream);
  emit("scan", "fast", scan_fast.to_string(sc_dt::SC_BIN, true), fast.to_bin());

  const auto fixed_view = sc_dt_fixed_subrefs_probe::fixed_const_range(fixed, 103, 96);
  const auto fast_view = sc_dt_fixed_subrefs_probe::fast_const_range(fast, 48, 41);
  emit("compare", "fixed_bv", int(fixed_view == sc_dt::sc_bv_base("11111111")),
       int(fixed_view != sc_dt::sc_bv_base("00000000")));
  emit("compare", "fast_bv", int(fast_view == sc_dt_fixed_subrefs_probe::to_fast_bit_vector(fast_view)),
       int(fast_view != sc_dt::sc_bv_base("00000000")));
  emit("concat", "fixed", sc_dt_fixed_subrefs_probe::concat_fixed(fixed_view, true));
  emit("concat", "fast", sc_dt_fixed_subrefs_probe::concat_fast(fast_view, false));

  const auto fixed_vectors = []()
  {
    sc_dt::sc_fixed<16, 8> number(-5.25);
    const auto view = sc_dt_fixed_subrefs_probe::fixed_const_range(number, 11, 4);
    const sc_dt::sc_bv_base bits(view);
    return std::make_pair(bits, sc_dt::sc_bv<8>(bits));
  }();
  emit("vector_copy", "fixed_r", fixed_vectors.first.to_string(sc_dt::SC_BIN, true),
       fixed_vectors.second.to_string(sc_dt::SC_BIN, true), fixed_vectors.first.length(),
       fixed_vectors.second.length());
  const auto fast_vectors = []()
  {
    sc_dt::sc_fixed_fast<16, 8> number(-5.25);
    const auto view = sc_dt_fixed_subrefs_probe::fast_const_range(number, 11, 4);
    const sc_dt::sc_bv_base bits(view);
    return std::make_pair(bits, sc_dt::sc_bv<8>(bits));
  }();
  emit("vector_copy", "fast_r", fast_vectors.first.to_string(sc_dt::SC_BIN, true),
       fast_vectors.second.to_string(sc_dt::SC_BIN, true), fast_vectors.first.length(), fast_vectors.second.length());

  const auto fixed_traits =
    std::is_same<decltype(std::declval<const sc_dt::sc_fxnum&>().range(1, 0)), sc_dt::sc_fxnum_subref_r>::value;
  const auto fast_traits = std::is_same<decltype(std::declval<const sc_dt::sc_fxnum_fast&>().range(1, 0)),
                                        sc_dt::sc_fxnum_fast_subref_r>::value;
  emit("traits", fixed_traits, fast_traits,
       std::is_constructible<sc_dt::sc_fxnum_subref, sc_dt::sc_fxnum&, int, int>::value,
       std::is_constructible<sc_dt::sc_fxnum_fast_subref, sc_dt::sc_fxnum_fast&, int, int>::value);
  emit("deprecated_api", "available");
  emit("deprecated_macro", int(sc_dt_fixed_subrefs_probe::deprecated_conversions_enabled()));
  return 0;
}
