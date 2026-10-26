/** @file sc_dt_values_public.cpp
 * @brief Independent native reference for small sc_dt values and scalar aliases.
 * @details The process owns its temporary streams and retains no external
 * resources or callbacks. Constant addresses are inspected only while the
 * SystemC library remains loaded in this process.
 */
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <systemc>
#include <vector>

#include "sc_dt_values_probe.h"

namespace
{

/** @brief Print every declared enumerator and its native string form. */
template <typename Enum, std::size_t N>
void print_enum(const char* type_name, const std::array<std::pair<const char*, Enum>, N>& values)
{
  for (const auto& entry : values)
  {
    std::cout << "SC_DT_ENUM " << type_name << ' ' << entry.first << ' ' << sc_dt_values_probe::enum_value(entry.second)
              << '\n';
  }
}

/** @brief Print exact fixed-point format overload results for an enum group. */
template <typename Enum, std::size_t N>
void print_formats(const char* type_name, const std::array<std::pair<const char*, Enum>, N>& values)
{
  for (const auto& entry : values)
  {
    std::cout << "SC_DT_FORMAT " << type_name << ' ' << entry.first << ' ' << sc_dt_values_probe::format(entry.second)
              << '\n';
    std::cout << "SC_DT_PUBLIC_FORMAT " << type_name << ' ' << entry.first << ' '
              << sc_dt_values_probe::format(entry.second) << '\n';
  }
}

/** @brief Print one math result without losing fractional native values. */
template <typename Value>
void print_math(const char* name, Value value)
{
  std::cout << "SC_DT_MATH " << name << ' ' << std::fixed << std::setprecision(1) << static_cast<double>(value)
            << std::defaultfloat << '\n';
}

/** @brief Exercise the two abs and three each min/max scalar call patterns. */
void check_math_helpers()
{
  print_math("sc_abs_int", sc_dt::sc_abs(-19));
  print_math("sc_abs_double", sc_dt::sc_abs(-2.5));
  print_math("sc_min_low_int", sc_dt::sc_min(3, 9));
  print_math("sc_min_negative_int", sc_dt::sc_min(-4, 2));
  print_math("sc_min_double", sc_dt::sc_min(1.25, 0.75));
  print_math("sc_max_high_int", sc_dt::sc_max(3, 9));
  print_math("sc_max_negative_int", sc_dt::sc_max(-4, 2));
  print_math("sc_max_double", sc_dt::sc_max(1.25, 0.75));

  sc_dt::sc_bigint<191> magnitude = 1;
  magnitude <<= 180;
  magnitude += 12345;
  const sc_dt::sc_bigint<191> big = -magnitude;
  const sc_dt::sc_bigint<191> midpoint = magnitude / 2;
  const auto big_abs = sc_dt::sc_abs(big);
  const auto big_min = sc_dt::sc_min(big, midpoint);
  const auto big_max = sc_dt::sc_max(big, midpoint);
  std::cout << "SC_DT_MATH sc_abs_big191 " << big_abs.to_string(sc_dt::SC_DEC) << '\n';
  std::cout << "SC_DT_MATH sc_min_big191 " << big_min.to_string(sc_dt::SC_DEC) << '\n';
  std::cout << "SC_DT_MATH sc_max_big191 " << big_max.to_string(sc_dt::SC_DEC) << '\n';
}

/** @brief Return one of the eight library-owned logic constants by index. */
const sc_dt::sc_logic& logic_constant(int group, int index)
{
  if (group == 0)
  {
    switch (index)
    {
    case 0:
      return sc_dt::SC_LOGIC_0;
    case 1:
      return sc_dt::SC_LOGIC_1;
    case 2:
      return sc_dt::SC_LOGIC_X;
    default:
      return sc_dt::SC_LOGIC_Z;
    }
  }
  switch (index)
  {
  case 0:
    return sc_dt::sc_logic_0;
  case 1:
    return sc_dt::sc_logic_1;
  case 2:
    return sc_dt::sc_logic_X;
  default:
    return sc_dt::sc_logic_Z;
  }
}

/** @brief Emit each library constant's value and native logic rendering. */
void check_logic_constants()
{
  const char* names[] = {"SC_LOGIC_0", "SC_LOGIC_1", "SC_LOGIC_X", "SC_LOGIC_Z",
                         "sc_logic_0", "sc_logic_1", "sc_logic_X", "sc_logic_Z"};
  std::set<std::uintptr_t> addresses;
  for (int group = 0; group < 2; ++group)
  {
    for (int index = 0; index < 4; ++index)
    {
      const auto& value = logic_constant(group, index);
      addresses.insert(sc_dt_values_probe::logic_address(group, index));
      std::cout << "SC_DT_LOGIC " << names[group * 4 + index] << ' ' << sc_dt_values_probe::logic_value(value) << ' '
                << sc_dt_values_probe::format_logic(value.value()) << '\n';
    }
  }
  bool matching_values = true;
  for (int index = 0; index < 4; ++index)
  {
    matching_values = matching_values && (logic_constant(0, index).value() == logic_constant(1, index).value());
  }
  std::cout << "SC_DT_LOGIC_IDENTITIES distinct=" << (addresses.size() == 8) << " aliases_equal=" << matching_values
            << '\n';
}

/** @brief Compare stream manipulators across decimal, octal and hexadecimal modes. */
void check_stream_helpers()
{
  const int bases[] = {10, 8, 16};
  const char* names[] = {"dec", "oct", "hex"};
  for (int base : bases)
  {
    for (bool show_base : {false, true})
    {
      std::ostringstream stream;
      sc_dt_values_probe::configure_stream(stream, base, show_base);
      const auto selected = sc_dt::sc_io_base(stream, sc_dt::SC_BIN);
      const bool selected_show_base = sc_dt::sc_io_show_base(stream);
      std::cout << "SC_DT_IO "
                << names[base == 10  ? 0
                         : base == 8 ? 1
                                     : 2]
                << " show=" << show_base << " base=" << static_cast<int>(selected)
                << " show_result=" << selected_show_base << '\n';
    }
  }
}

/** @brief Print alias limits, widths, full-width round trips and narrowing. */
void check_numeric_aliases()
{
  using limits_i64 = std::numeric_limits<sc_dt::int64>;
  using limits_u64 = std::numeric_limits<sc_dt::uint64>;
  using limits_digit = std::numeric_limits<sc_dt::sc_digit>;
  const sc_dt::int64 min_i64 = limits_i64::min();
  const sc_dt::int64 max_i64 = limits_i64::max();
  const sc_dt::uint64 max_u64 = limits_u64::max();
  const sc_dt::sc_digit max_digit = limits_digit::max();
  const sc_dt::uint64 wide = (sc_dt::uint64{1} << 40) + 0x12345;
  std::cout << "SC_DT_NUMERIC int64=" << sizeof(sc_dt::int64) << ':' << min_i64 << ':' << max_i64
            << " uint64=" << sizeof(sc_dt::uint64) << ':' << max_u64 << " sc_digit=" << sizeof(sc_dt::sc_digit) << ':'
            << limits_digit::min() << ':' << max_digit << " roundtrip=" << sc_dt_values_probe::echo_int64(min_i64)
            << ':' << sc_dt_values_probe::echo_int64(max_i64) << ':' << sc_dt_values_probe::echo_uint64(max_u64) << ':'
            << sc_dt_values_probe::echo_digit(max_digit) << " narrow=" << sc_dt_values_probe::narrow_digit(wide)
            << '\n';
}

}  // namespace

/** @brief Print the native values, formats, constant identities and alias limits. */
int sc_main(int, char**)
{
  print_enum("sc_enc", std::array<std::pair<const char*, sc_dt::sc_enc>, 2>{
                         {{"SC_TC_", sc_dt::SC_TC_}, {"SC_US_", sc_dt::SC_US_}}});
  print_enum("sc_fmt",
             std::array<std::pair<const char*, sc_dt::sc_fmt>, 2>{{{"SC_F", sc_dt::SC_F}, {"SC_E", sc_dt::SC_E}}});
  print_enum("sc_logic_value_t",
             std::array<std::pair<const char*, sc_dt::sc_logic_value_t>, 4>{
               {{"Log_0", sc_dt::Log_0}, {"Log_1", sc_dt::Log_1}, {"Log_Z", sc_dt::Log_Z}, {"Log_X", sc_dt::Log_X}}});
  std::cout << "SC_DT_CONTEXT_ENUM SC_NOW " << sc_dt_values_probe::enum_value(sc_dt::SC_NOW) << " SC_LATER "
            << sc_dt_values_probe::enum_value(sc_dt::SC_LATER) << '\n';
  print_enum("sc_numrep", std::array<std::pair<const char*, sc_dt::sc_numrep>, 12>{{{"SC_NOBASE", sc_dt::SC_NOBASE},
                                                                                    {"SC_BIN", sc_dt::SC_BIN},
                                                                                    {"SC_OCT", sc_dt::SC_OCT},
                                                                                    {"SC_DEC", sc_dt::SC_DEC},
                                                                                    {"SC_HEX", sc_dt::SC_HEX},
                                                                                    {"SC_BIN_US", sc_dt::SC_BIN_US},
                                                                                    {"SC_OCT_US", sc_dt::SC_OCT_US},
                                                                                    {"SC_HEX_US", sc_dt::SC_HEX_US},
                                                                                    {"SC_BIN_SM", sc_dt::SC_BIN_SM},
                                                                                    {"SC_OCT_SM", sc_dt::SC_OCT_SM},
                                                                                    {"SC_HEX_SM", sc_dt::SC_HEX_SM},
                                                                                    {"SC_CSD", sc_dt::SC_CSD}}});
  print_enum("sc_o_mode", std::array<std::pair<const char*, sc_dt::sc_o_mode>, 5>{{{"SC_SAT", sc_dt::SC_SAT},
                                                                                   {"SC_SAT_ZERO", sc_dt::SC_SAT_ZERO},
                                                                                   {"SC_SAT_SYM", sc_dt::SC_SAT_SYM},
                                                                                   {"SC_WRAP", sc_dt::SC_WRAP},
                                                                                   {"SC_WRAP_SM", sc_dt::SC_WRAP_SM}}});
  print_enum("sc_q_mode",
             std::array<std::pair<const char*, sc_dt::sc_q_mode>, 7>{{{"SC_RND", sc_dt::SC_RND},
                                                                      {"SC_RND_ZERO", sc_dt::SC_RND_ZERO},
                                                                      {"SC_RND_MIN_INF", sc_dt::SC_RND_MIN_INF},
                                                                      {"SC_RND_INF", sc_dt::SC_RND_INF},
                                                                      {"SC_RND_CONV", sc_dt::SC_RND_CONV},
                                                                      {"SC_TRN", sc_dt::SC_TRN},
                                                                      {"SC_TRN_ZERO", sc_dt::SC_TRN_ZERO}}});
  print_enum("sc_switch", std::array<std::pair<const char*, sc_dt::sc_switch>, 2>{
                            {{"SC_OFF", sc_dt::SC_OFF}, {"SC_ON", sc_dt::SC_ON}}});

  print_formats("sc_enc", std::array<std::pair<const char*, sc_dt::sc_enc>, 2>{
                            {{"SC_TC_", sc_dt::SC_TC_}, {"SC_US_", sc_dt::SC_US_}}});
  print_formats("sc_fmt",
                std::array<std::pair<const char*, sc_dt::sc_fmt>, 2>{{{"SC_F", sc_dt::SC_F}, {"SC_E", sc_dt::SC_E}}});
  print_formats("sc_numrep", std::array<std::pair<const char*, sc_dt::sc_numrep>, 12>{{{"SC_NOBASE", sc_dt::SC_NOBASE},
                                                                                       {"SC_BIN", sc_dt::SC_BIN},
                                                                                       {"SC_OCT", sc_dt::SC_OCT},
                                                                                       {"SC_DEC", sc_dt::SC_DEC},
                                                                                       {"SC_HEX", sc_dt::SC_HEX},
                                                                                       {"SC_BIN_US", sc_dt::SC_BIN_US},
                                                                                       {"SC_OCT_US", sc_dt::SC_OCT_US},
                                                                                       {"SC_HEX_US", sc_dt::SC_HEX_US},
                                                                                       {"SC_BIN_SM", sc_dt::SC_BIN_SM},
                                                                                       {"SC_OCT_SM", sc_dt::SC_OCT_SM},
                                                                                       {"SC_HEX_SM", sc_dt::SC_HEX_SM},
                                                                                       {"SC_CSD", sc_dt::SC_CSD}}});
  print_formats("sc_o_mode",
                std::array<std::pair<const char*, sc_dt::sc_o_mode>, 5>{{{"SC_SAT", sc_dt::SC_SAT},
                                                                         {"SC_SAT_ZERO", sc_dt::SC_SAT_ZERO},
                                                                         {"SC_SAT_SYM", sc_dt::SC_SAT_SYM},
                                                                         {"SC_WRAP", sc_dt::SC_WRAP},
                                                                         {"SC_WRAP_SM", sc_dt::SC_WRAP_SM}}});
  print_formats("sc_q_mode",
                std::array<std::pair<const char*, sc_dt::sc_q_mode>, 7>{{{"SC_RND", sc_dt::SC_RND},
                                                                         {"SC_RND_ZERO", sc_dt::SC_RND_ZERO},
                                                                         {"SC_RND_MIN_INF", sc_dt::SC_RND_MIN_INF},
                                                                         {"SC_RND_INF", sc_dt::SC_RND_INF},
                                                                         {"SC_RND_CONV", sc_dt::SC_RND_CONV},
                                                                         {"SC_TRN", sc_dt::SC_TRN},
                                                                         {"SC_TRN_ZERO", sc_dt::SC_TRN_ZERO}}});
  print_formats("sc_switch", std::array<std::pair<const char*, sc_dt::sc_switch>, 2>{
                               {{"SC_OFF", sc_dt::SC_OFF}, {"SC_ON", sc_dt::SC_ON}}});

  check_logic_constants();
  check_stream_helpers();
  check_numeric_aliases();
  check_math_helpers();
  return 0;
}
