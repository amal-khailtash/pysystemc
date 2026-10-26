/** @file sc_dt_fixed_bitrefs_public.cpp
 * @brief Native reference matrix for public fixed-point bit references.
 * @details Emits const and mutable view observations, assignment and compound
 *          results, stream operations, proxy copies, width behavior, and the
 *          sc_bitref_conv_r specialization for comparison with Python.
 */
#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>

#include "sc_dt_fixed_bitrefs_probe.h"

namespace
{

template <typename... Fields>
void emit(const Fields&... fields)
{
  std::cout << "FXBIT";
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

template <typename Number>
void exercise_mutable_accessors(const char* name, Number& number, int index)
{
  auto method_proxy = number.bit(index);
  using MutableProxy = decltype(method_proxy);
  static_assert(std::is_same<MutableProxy, sc_dt::sc_fxnum_bitref>::value ||
                  std::is_same<MutableProxy, sc_dt::sc_fxnum_fast_bitref>::value,
                "a mutable number must return the matching writable proxy");
  const bool initial = method_proxy;
  number[index] = !initial;
  const bool item_write_is_visible = method_proxy;
  method_proxy = initial;
  const bool method_write_is_visible = number[index];
  emit("access", name, index, initial, item_write_is_visible, method_write_is_visible, number.to_bin());
}

template <typename Number>
void exercise_operations(const char* target_name, bool fast_target)
{
  using FixedSource = sc_dt::sc_fixed<8, 4>;
  using FastSource = sc_dt::sc_fixed_fast<8, 4>;
  constexpr std::array<const char*, 4> source_names{"fixed", "fast", "sc_bit", "bool"};
  constexpr std::array<char, 4> source_values{1, 0, 1, 0};
  constexpr std::array<char, 4> operations{'=', '&', '|', '^'};
  constexpr std::array<const char*, 4> operation_names{"assign", "and", "or", "xor"};

  for (std::size_t source_index = 0; source_index < source_names.size(); ++source_index)
  {
    for (std::size_t operation_index = 0; operation_index < operations.size(); ++operation_index)
    {
      Number target(0);
      FixedSource fixed_source(0);
      FastSource fast_source(0);
      sc_dt::sc_bit bit_source(false);
      const bool right = source_values[source_index] != 0;
      const bool initial = operations[operation_index] == '=' || operations[operation_index] == '|' ? false : true;
      target[1] = initial;
      fixed_source[1] = right;
      fast_source[1] = right;
      bit_source = right;

      auto apply = [&](auto& left, const auto& rhs)
      {
        switch (operations[operation_index])
        {
        case '=':
          left = rhs;
          break;
        case '&':
          left &= rhs;
          break;
        case '|':
          left |= rhs;
          break;
        default:
          left ^= rhs;
          break;
        }
      };

      auto target_proxy = target[1];
      if (source_index == 0)
        apply(target_proxy, fixed_source[1]);
      else if (source_index == 1)
        apply(target_proxy, fast_source[1]);
      else if (source_index == 2)
        apply(target_proxy, bit_source);
      else
        apply(target_proxy, right);

      emit("op", target_name, operation_names[operation_index], source_names[source_index], initial, right,
           bool(target[1]), target.to_bin());
    }
  }

  (void)fast_target;
}

template <typename Number>
void exercise_views(const char* name, Number& number, const std::array<int, 5>& indexes)
{
  const Number& read_only = number;
  for (int index : indexes)
  {
    auto const_proxy = read_only.bit(index);
    auto mutable_proxy = number[index];
    std::ostringstream print_stream;
    std::ostringstream dump_stream;
    const_proxy.print(print_stream);
    const_proxy.dump(dump_stream);
    emit("view", name, index, bool(const_proxy), print_stream.str(), flatten(dump_stream.str()), bool(mutable_proxy));
  }

  const auto source_proxy = number[indexes[1]];
  using MutableProxy = decltype(source_proxy);
  MutableProxy saved_proxy(source_proxy);
  number[indexes[1]] = true;
  emit("copy", name, indexes[1], bool(saved_proxy), number.to_bin());

  std::istringstream scan_input("0");
  auto scan_proxy = number[indexes[1]];
  scan_proxy.scan(scan_input);
  emit("scan", name, indexes[1], bool(scan_proxy), number.to_bin());

  exercise_mutable_accessors(name, number, indexes[2]);
}

int report_error_mode(const std::string& mode)
{
  const auto previous = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  try
  {
    if (mode == "bad-index-fixed")
    {
      sc_dt::sc_fixed<16, 8> number(1);
      (void)number.bit(-1);
    }
    else if (mode == "bad-index-fast")
    {
      sc_dt::sc_fixed_fast<16, 8> number(1);
      (void)number.bit(16);
    }
    else if (mode == "fast-wide")
    {
      sc_dt::sc_fixed_fast<137, 100> number("9007199254740992.25");
      emit("wide-fast", number.to_bin());
      sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
      return 0;
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

/** @brief Emit the native proxy reference matrix.
 * @param argc Native argument count.
 * @param argv Native arguments; an optional error mode isolates native reports.
 * @return Zero after the comparison records are emitted.
 */
int sc_main(int argc, char** argv)
{
  if (argc > 1) return report_error_mode(argv[1]);

  sc_dt::sc_fixed<137, 69> wide(-13.375);
  sc_dt::sc_fixed_fast<64, 32> fast(-13.375);
  emit("width", "fixed", wide.wl(), wide.iwl(), wide.to_bin(), wide.to_double());
  emit("width", "fast", fast.wl(), fast.iwl(), fast.to_bin(), fast.to_double());
  sc_dt::sc_fixed<137, 100> precision_fixed("9007199254740992.25");
  sc_dt::sc_fixed_fast<137, 100> precision_fast("9007199254740992.25");
  emit("precision", precision_fixed.to_bin(), precision_fast.to_bin());
  exercise_mutable_accessors("fast137", precision_fast, 90);

  exercise_views("fixed", wide, std::array<int, 5>{0, 7, 8, 68, 136});
  exercise_views("fast", fast, std::array<int, 5>{0, 30, 31, 32, 63});

  exercise_operations<sc_dt::sc_fixed<8, 4>>("fixed", false);
  exercise_operations<sc_dt::sc_fixed_fast<8, 4>>("fast", true);

  sc_dt::sc_bv_base bits(2);
  bits[0] = true;
  bits[1] = false;
  for (int index = 0; index < 2; ++index)
  {
    const auto conversions = sc_dt_fixed_bitrefs_probe::bitref_conversions(bits, index);
    emit("conv", index, conversions.first, conversions.second, bool(bits[index]));
  }

  emit("proxy_access", "fixed_mutable_is_constructible",
       std::is_constructible<sc_dt::sc_fxnum_bitref, sc_dt::sc_fxnum&, int>::value, "fixed_readonly_is_constructible",
       std::is_constructible<sc_dt::sc_fxnum_bitref_r, sc_dt::sc_fxnum&, int>::value, "fast_mutable_is_constructible",
       std::is_constructible<sc_dt::sc_fxnum_fast_bitref, sc_dt::sc_fxnum_fast&, int>::value,
       "fast_readonly_is_constructible",
       std::is_constructible<sc_dt::sc_fxnum_fast_bitref_r, sc_dt::sc_fxnum_fast&, int>::value,
       "fixed_mutable_copyable", std::is_constructible<sc_dt::sc_fxnum_bitref, const sc_dt::sc_fxnum_bitref&>::value,
       "fixed_readonly_copyable",
       std::is_constructible<sc_dt::sc_fxnum_bitref_r, const sc_dt::sc_fxnum_bitref_r&>::value, "fast_mutable_copyable",
       std::is_constructible<sc_dt::sc_fxnum_fast_bitref, const sc_dt::sc_fxnum_fast_bitref&>::value,
       "fast_readonly_copyable",
       std::is_constructible<sc_dt::sc_fxnum_fast_bitref_r, const sc_dt::sc_fxnum_fast_bitref_r&>::value,
       "fixed_mutable_destructible", std::is_destructible<sc_dt::sc_fxnum_bitref>::value, "fixed_readonly_destructible",
       std::is_destructible<sc_dt::sc_fxnum_bitref_r>::value, "fast_mutable_destructible",
       std::is_destructible<sc_dt::sc_fxnum_fast_bitref>::value, "fast_readonly_destructible",
       std::is_destructible<sc_dt::sc_fxnum_fast_bitref_r>::value);

  return 0;
}
