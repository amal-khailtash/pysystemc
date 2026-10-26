/** @file sc_dt_vector_subrefs_public.cpp
 * @brief Independent native reference for public sc_dt vector subreferences.
 * @details The executable owns every BV and LV parent. Subreference and
 *          concatenation proxies borrow those parents and are consumed before
 *          the owners leave scope. No callbacks or scheduler processes are
 *          registered. C++ exceptions are caught only for explicit error
 *          probes; the process has no Python references or GIL interaction.
 */
#include <iostream>
#include <string>
#include <systemc>

#include "sc_dt_vector_subrefs_probe.h"

namespace
{

using namespace sc_dt_vector_subrefs_probe;

/** @brief Build a deterministic wide bit spelling used by the Python model. */
std::string pattern(int width, const std::string& alphabet, int offset)
{
  std::string result;
  result.reserve(static_cast<std::size_t>(width));
  for (int index = 0; index < width; ++index)
    result.push_back(alphabet[static_cast<std::size_t>((index * 7 + offset) % static_cast<int>(alphabet.size()))]);
  return result;
}

/** @brief Emit one tab-separated record shared with the Python model. */
template <typename... Values>
void emit(const char* family, const char* case_name, const Values&... values)
{
  std::cout << "SDR\t" << family << '\t' << case_name;
  using swallow = int[];
  (void)swallow{0, ((std::cout << '\t' << values), 0)...};
  std::cout << '\n';
}

/** @brief Record range shape, bit text, and public scalar conversions. */
template <typename Proxy>
void emit_range(const char* kind, const char* direction, const Proxy& view)
{
  emit("range", kind, direction, view.length(), view.size(), int(view.reversed()), int(view.is_01()), render(view));
  emit("state", kind, direction, range_state(view));
  emit("traits", kind, direction, traits<Proxy>());
  emit("print", kind, direction, print(view));
}

/** @brief Exercise public mutable slice assignment and compound operators. */
void exercise_mutable_ranges(sc_dt::sc_bv<137>& bv, sc_dt::sc_lv<191>& lv)
{
  auto bv_view = bv.range(47, 32);
  auto bv_copy = bv_subref(bv_view);
  bv_copy = "1010010110100101";
  emit("assign", "bv_copy", render(bv_view), render(bv));
  bv_copy &= "1111000011110000";
  emit("compound", "bv_and", render(bv_view), render(bv));
  bv_copy |= "0000111100001111";
  emit("compound", "bv_or", render(bv_view), render(bv));
  bv_copy ^= "0011001100110011";
  emit("compound", "bv_xor", render(bv_view), render(bv));

  auto lv_view = lv.range(55, 40);
  auto lv_copy = lv_subref(lv_view);
  lv_copy = "ZX01ZX01ZX01ZX01";
  emit("assign", "lv_copy", render(lv_view), render(lv));
  lv_copy &= "11ZZ00XX11ZZ00XX";
  emit("compound", "lv_and", render(lv_view), render(lv));
  lv_copy |= "0011XXZZ0011XXZZ";
  emit("compound", "lv_or", render(lv_view), render(lv));
  lv_copy ^= "01ZX01ZX01ZX01ZX";
  emit("compound", "lv_xor", render(lv_view), render(lv));
}

/** @brief Exercise mutable and nested borrowed concatenation proxies. */
void exercise_concatenations(sc_dt::sc_bv<137>& bv, sc_dt::sc_lv<191>& lv, sc_dt::sc_lv<191>& lv_second)
{
  auto left = bv.range(31, 16);
  auto middle = lv.range(39, 24);
  auto right = lv_second.range(47, 32);
  auto concatenated = sc_dt::concat(left, middle);
  emit("concat", "mutable", concatenated.length(), render(concatenated));
  emit("state", "concat_mutable", concat_state(concatenated));
  emit("traits", "concat_mutable", concat_mutable_traits());
  emit("print", "concat_mutable", print(concatenated));
  const bv_concat mutable_direct(left, middle);
  emit("construct", "concat_mutable_direct", render(mutable_direct));
  concatenated = "101010101010101001ZX01ZX01ZX01ZX";
  emit("concat", "assigned", render(left), render(middle), render(concatenated));
  auto mutable_copy = bv_concat(concatenated);
  mutable_copy = "00110011001100110011001100110011";
  emit("concat", "copy_assigned", render(left), render(middle), render(mutable_copy));

  auto nested_inner = sc_dt::concat(left, middle);
  auto nested_outer = sc_dt::concat(nested_inner, right);
  emit("concat", "nested", nested_outer.length(), render(nested_outer));

  const sc_dt::sc_bv_base& const_bv = bv;
  const sc_dt::sc_lv_base& const_lv = lv;
  const auto read_left = readonly_bv_range(const_bv, 31, 16);
  const auto read_right = readonly_lv_range(const_lv, 39, 24);
  const auto readonly_concat = sc_dt::concat(read_left, read_right);
  emit("concat", "readonly", readonly_concat.length(), render(readonly_concat), int(readonly_concat.is_01()));
  emit("state", "concat_readonly", concat_state(readonly_concat));
  emit("traits", "concat_readonly", concat_readonly_traits());
  emit("print", "concat_readonly", print(readonly_concat));
  const bv_concat_r readonly_direct(read_left, read_right);
  emit("construct", "concat_readonly_direct", render(readonly_direct));
  const auto readonly_copy = bv_concat_r(read_left, read_right);
  emit("construct", "concat_readonly_copy", render(readonly_copy));
  emit("construct", "concat_mutable_copy", render(bv_concat(concatenated)));
}

/** @brief Reset a proxy destination, call one exact native assignment overload, and emit it. */
template <typename Destination, typename Value>
void emit_assignment(const char* family, const char* label, Destination& destination, const Value& value)
{
  destination = std::string(static_cast<std::size_t>(destination.length()), '0').c_str();
  destination = value;
  emit("assignment", family, label, render(destination));
}

/** @brief Exercise every inventoried overload on BV and LV subreferences. */
template <typename Owner>
void exercise_subref_assignments(const char* family, Owner& destination_owner, Owner& source_owner)
{
  auto destination = destination_owner.range(15, 0);
  const auto source_write = source_owner.range(15, 0);
  const Owner& readonly_owner = source_owner;
  const auto source_read = readonly_owner.range(15, 0);
  const char* text16 = "1010010110100101";
  const int int_value = 0x5A35;
  const unsigned int uint_value = 0xA53C;
  const long long_value = -0x13579;
  const unsigned long ulong_value = 0x12345678UL;
  const sc_dt::int64 int64_value = static_cast<sc_dt::int64>(-0x123456789LL);
  const sc_dt::uint64 uint64_value = static_cast<sc_dt::uint64>(0x123456789ABCDEF0ULL);
  emit_assignment(family, "proxy_template", destination, assignment_proxy_value());
  emit_assignment(family, "subref_readonly_copy", destination, source_read);
  emit_assignment(family, "subref_mutable_copy", destination, source_write);
  emit_assignment(family, "bool_pointer", destination, assignment_bool_bits());
  emit_assignment(family, "char_pointer", destination, text16);
  emit_assignment(family, "logic_pointer", destination, assignment_logic_bits());
  emit_assignment(family, "sc_unsigned", destination, assignment_unsigned_value());
  emit_assignment(family, "sc_signed", destination, assignment_signed_value());
  emit_assignment(family, "sc_uint_base", destination, assignment_uint_value());
  emit_assignment(family, "sc_int_base", destination, assignment_int_value());
  emit_assignment(family, "unsigned_long", destination, ulong_value);
  emit_assignment(family, "long", destination, long_value);
  emit_assignment(family, "unsigned_int", destination, uint_value);
  emit_assignment(family, "int", destination, int_value);
  emit_assignment(family, "uint64", destination, uint64_value);
  emit_assignment(family, "int64", destination, int64_value);
}

/** @brief Exercise every inventoried overload on a mutable BV/LV concatenation. */
void exercise_concat_assignments()
{
  sc_dt::sc_bv<32> destination_bv("00000000000000000000000000000000");
  sc_dt::sc_lv<32> destination_lv("00000000000000000000000000000000");
  sc_dt::sc_bv<32> source_bv("10100101101001011100110000111100");
  sc_dt::sc_lv<32> source_lv("10100101101001011100110000111100");
  auto destination = sc_dt::concat(destination_bv.range(31, 16), destination_lv.range(31, 16));
  auto source_copy = sc_dt::concat(source_bv.range(31, 16), source_lv.range(31, 16));
  const char* text32 = "10100101101001011100110000111100";
  const int int_value = 0x5A35;
  const unsigned int uint_value = 0xA53C;
  const long long_value = -0x13579;
  const unsigned long ulong_value = 0x12345678UL;
  const sc_dt::int64 int64_value = static_cast<sc_dt::int64>(-0x123456789LL);
  const sc_dt::uint64 uint64_value = static_cast<sc_dt::uint64>(0x123456789ABCDEF0ULL);
  emit_assignment("concat", "proxy_template", destination, assignment_proxy_value());
  emit_assignment("concat", "concref_copy", destination, source_copy);
  emit_assignment("concat", "bool_pointer", destination, assignment_bool_bits());
  emit_assignment("concat", "char_pointer", destination, text32);
  emit_assignment("concat", "logic_pointer", destination, assignment_logic_bits());
  emit_assignment("concat", "sc_unsigned", destination, assignment_unsigned_value());
  emit_assignment("concat", "sc_signed", destination, assignment_signed_value());
  emit_assignment("concat", "sc_uint_base", destination, assignment_uint_value());
  emit_assignment("concat", "sc_int_base", destination, assignment_int_value());
  emit_assignment("concat", "unsigned_long", destination, ulong_value);
  emit_assignment("concat", "long", destination, long_value);
  emit_assignment("concat", "unsigned_int", destination, uint_value);
  emit_assignment("concat", "int", destination, int_value);
  emit_assignment("concat", "uint64", destination, uint64_value);
  emit_assignment("concat", "int64", destination, int64_value);
}

/** @brief Probe proxy bit/word accessors and mutators without leaving owners. */
template <typename Proxy>
void exercise_proxy_storage(const char* family, const char* kind, Proxy& view)
{
  emit("storage", family, kind, "get_bit", int(view.get_bit(0)));
  view.set_bit(0, static_cast<typename Proxy::value_type>(1));
  emit("storage", family, kind, "set_bit", render(view));
  view.set_word(0, 0xA5U);
  emit("storage", family, kind, "set_word", view.get_word(0), view.get_cword(0), render(view));
  view.set_cword(0, 0x5AU);
  emit("storage", family, kind, "set_cword", view.get_word(0), view.get_cword(0), render(view));
  view.clean_tail();
  emit("storage", family, kind, "clean_tail", render(view));
}

/** @brief Clone and delete a public proxy clone while its borrowed owners live. */
template <typename Proxy>
void exercise_clone(const char* family, const char* kind, const Proxy& view)
{
  Proxy* clone = view.clone();
  emit("clone", family, kind, render(*clone));
  delete clone;
}

/** @brief Exercise clone and low-level storage surfaces across all proxy families. */
void exercise_low_level_surfaces()
{
  sc_dt::sc_bv<64> bv("0101010101010101010101010101010101010101010101010101010101010101");
  sc_dt::sc_lv<64> lv("01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX");
  auto bv_write = bv.range(31, 16);
  auto lv_write = lv.range(31, 16);
  auto bv_read = readonly_bv_range(bv, 31, 16);
  auto lv_read = readonly_lv_range(lv, 31, 16);
  exercise_proxy_storage("sc_subref", "bv", bv_write);
  exercise_proxy_storage("sc_subref", "lv", lv_write);
  exercise_proxy_storage("sc_subref_r", "bv", bv_read);
  exercise_proxy_storage("sc_subref_r", "lv", lv_read);
  exercise_clone("sc_subref", "bv", bv_write);
  exercise_clone("sc_subref", "lv", lv_write);
  exercise_clone("sc_subref_r", "bv", bv_read);
  exercise_clone("sc_subref_r", "lv", lv_read);

  auto concat_write = sc_dt::concat(bv.range(31, 16), lv.range(31, 16));
  auto concat_read = sc_dt::concat(bv_read, lv_read);
  exercise_proxy_storage("sc_concref", "mixed", concat_write);
  exercise_proxy_storage("sc_concref_r", "mixed", concat_read);
  exercise_clone("sc_concref", "mixed", concat_write);
  exercise_clone("sc_concref_r", "mixed", concat_read);
}

}  // namespace

int sc_main(int, char**)
{
  sc_dt::sc_bv<137> bv(pattern(137, "01", 3).c_str());
  sc_dt::sc_lv<191> lv(pattern(191, "01ZX", 1).c_str());
  sc_dt::sc_lv<191> lv_second(pattern(191, "01ZX", 2).c_str());

  const auto bv_source_range = bv.range(31, 16);
  const bv_subref bv_direct(bv, 31, 16);
  const bv_subref bv_copy(bv_source_range);
  const bv_subref_r bv_readonly_direct(bv, 31, 16);
  const auto lv_source_range = lv.range(31, 16);
  const lv_subref lv_direct(lv, 31, 16);
  const lv_subref lv_copy(lv_source_range);
  const lv_subref_r lv_readonly_direct(lv, 31, 16);
  emit("construct", "bv_mutable_direct", render(bv_direct));
  emit("construct", "bv_mutable_copy", render(bv_copy));
  emit("construct", "bv_readonly_direct", render(bv_readonly_direct));
  emit("construct", "lv_mutable_direct", render(lv_direct));
  emit("construct", "lv_mutable_copy", render(lv_copy));
  emit("construct", "lv_readonly_direct", render(lv_readonly_direct));

  auto live_view = bv.range(111, 96);
  const std::string before_owner_change = render(live_view);
  bv.range(111, 96) = "0011001100110011";
  emit("borrowed", "parent_mutation", before_owner_change, render(live_view));

  for (const auto& endpoints : {std::pair<int, int>{31, 16}, std::pair<int, int>{16, 31}})
  {
    const char* direction = endpoints.first > endpoints.second ? "forward" : "reversed";
    emit_range("bv137", direction, bv.range(endpoints.first, endpoints.second));
    emit_range("lv191", direction, lv.range(endpoints.first, endpoints.second));
    emit_range("bv137_readonly", direction, readonly_bv_range(bv, endpoints.first, endpoints.second));
    emit_range("lv191_readonly", direction, readonly_lv_range(lv, endpoints.first, endpoints.second));
  }

  const auto numeric_bv = bv.range(63, 48);
  emit("convert", "bv", numeric_bv.to_uint64(), numeric_bv.to_int64(), numeric_bv.to_string());
  const auto numeric_lv = lv.range(63, 48);
  emit("convert", "lv", numeric_lv.to_uint64(), numeric_lv.to_int64(), numeric_lv.to_string());
  emit("construct", "compile_time", int(construction_traits()));
  emit("error", "invalid_range_rejected", int(rejects_invalid_bv_range(bv)));
  emit("concat", "nested_native_helper", nested_read_concat(bv, lv, lv_second));

  exercise_mutable_ranges(bv, lv);
  exercise_concatenations(bv, lv, lv_second);

  auto scanned = bv.range(95, 80);
  const std::string scan_text = "1011001110001111";
  emit("scan", "bv", scan(scanned, scan_text), render(bv));
  auto lv_scanned = lv.range(95, 80);
  const std::string lv_scan_text = "ZX01ZX01ZX01ZX01";
  emit("scan", "lv", scan(lv_scanned, lv_scan_text), render(lv));

  auto concat_scanned = sc_dt::concat(bv.range(111, 96), lv.range(111, 96));
  const std::string concat_scan_text = "11001100110011000011XXZZ0011XXZZ";
  std::istringstream concat_input(concat_scan_text);
  concat_scanned.scan(concat_input);
  emit("scan", "concat", render(concat_scanned), render(bv), render(lv));

  sc_dt::sc_bv<64> bv_destination("0");
  sc_dt::sc_bv<64> bv_source(pattern(64, "01", 2).c_str());
  sc_dt::sc_lv<64> lv_destination("0");
  sc_dt::sc_lv<64> lv_source(pattern(64, "01ZX", 3).c_str());
  exercise_subref_assignments("bv_subref", bv_destination, bv_source);
  exercise_subref_assignments("lv_subref", lv_destination, lv_source);
  exercise_concat_assignments();
  exercise_low_level_surfaces();

  return 0;
}
