/** @file sc_dt_policy_matrix.cpp
 * @brief Native reference for fixed-point quantization and overflow policy behavior.
 * @details Each family uses local runtime-configured numbers. The sole invalid
 * configuration, unsigned SC_WRAP_SM, is required to raise a SystemC report and
 * is recorded explicitly. No Python callbacks or kernel resets are involved.
 */
#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#ifndef SC_INCLUDE_FX
#define SC_INCLUDE_FX
#endif
#include <systemc>

namespace
{
using namespace sc_dt;

struct QuantizationCase
{
  const char* label;
  const char* value;
};

constexpr std::array<sc_q_mode, 7> quantizers = {SC_RND,      SC_RND_ZERO, SC_RND_MIN_INF, SC_RND_INF,
                                                 SC_RND_CONV, SC_TRN,      SC_TRN_ZERO};
constexpr std::array<sc_o_mode, 5> overflow_modes = {SC_SAT, SC_SAT_ZERO, SC_SAT_SYM, SC_WRAP, SC_WRAP_SM};
constexpr std::array<QuantizationCase, 8> quantization_cases = {{{"tie_pos_even", "1.03125"},
                                                                 {"tie_pos_odd", "1.09375"},
                                                                 {"tie_neg_even", "-1.03125"},
                                                                 {"tie_neg_odd", "-1.09375"},
                                                                 {"below_pos", "1.03124"},
                                                                 {"above_pos", "1.03126"},
                                                                 {"below_neg", "-1.03124"},
                                                                 {"above_neg", "-1.03126"}}};

/** @brief Render the native value, configured policies and last-cast flags. */
template <class Number>
std::string render(const Number& value)
{
  std::ostringstream output;
  output << value.to_string(SC_DEC) << '\t' << value.wl() << '\t' << value.iwl() << '\t'
         << static_cast<int>(value.q_mode()) << '\t' << static_cast<int>(value.o_mode()) << '\t' << value.n_bits()
         << '\t' << static_cast<int>(value.cast_switch() == SC_ON) << '\t'
         << static_cast<int>(value.quantization_flag()) << '\t' << static_cast<int>(value.overflow_flag());
  return output.str();
}

/** @brief Emit one value with the same tab-separated schema as the Python model. */
template <class Number>
void emit_value(const char* family, sc_q_mode q_mode, sc_o_mode o_mode, int n_bits, int cast, const char* label,
                const Number& value)
{
  std::cout << "F2\tvalue\t" << family << '\t' << static_cast<int>(q_mode) << '\t' << static_cast<int>(o_mode) << '\t'
            << n_bits << '\t' << cast << '\t' << label << '\t' << render(value) << '\n';
}

/** @brief Assign one decimal source through an explicit cast switch and report it. */
template <class Number>
void assign_case(const char* family, sc_q_mode q_mode, sc_o_mode o_mode, int n_bits, int cast, const char* label,
                 const char* source)
{
  const sc_fxcast_switch cast_mode(cast == 1 ? SC_ON : SC_OFF);
  Number number("0", 8, 4, q_mode, o_mode, n_bits, cast_mode);
  number = source;
  emit_value(family, q_mode, o_mode, n_bits, cast, label, number);
}

/** @brief Exercise one slow or fast signed/unsigned fixed-point family. */
template <class Number>
void run_family(const char* family, bool is_signed)
{
  for (const auto q_mode : quantizers)
  {
    for (const auto o_mode : overflow_modes)
    {
      if (!is_signed && o_mode == SC_WRAP_SM)
      {
        bool rejected = false;
        try
        {
          Number invalid("0", 8, 4, q_mode, o_mode, 2);
        }
        catch (const sc_core::sc_report&)
        {
          rejected = true;
        }
        if (!rejected) throw std::runtime_error("unsigned SC_WRAP_SM was accepted");
        std::cout << "F2\treject\t" << family << '\t' << static_cast<int>(q_mode) << '\t' << static_cast<int>(o_mode)
                  << "\t2\tunsigned_wrap_sm\t1\n";
        continue;
      }

      for (const auto& item : quantization_cases)
        assign_case<Number>(family, q_mode, o_mode, 2, 1, item.label, item.value);

      const char* positive_limit = is_signed ? "8" : "16";
      const char* negative_limit = is_signed ? "-8.0625" : "-0.0625";
      assign_case<Number>(family, q_mode, o_mode, 2, 1, "overflow_positive", positive_limit);
      assign_case<Number>(family, q_mode, o_mode, 2, 1, "overflow_negative", negative_limit);
      assign_case<Number>(family, q_mode, o_mode, 2, 1, "lossy_positive", "17.03125");
      assign_case<Number>(family, q_mode, o_mode, 2, 0, "lossy_positive", "17.03125");
      assign_case<Number>(family, q_mode, o_mode, 2, 1, "lossy_negative", "-9.03125");
      assign_case<Number>(family, q_mode, o_mode, 2, 0, "lossy_negative", "-9.03125");

      if (o_mode == SC_WRAP || o_mode == SC_WRAP_SM)
      {
        const char* probe = is_signed ? "11.0625" : "19.0625";
        assign_case<Number>(family, q_mode, o_mode, 0, 1, "n_bits_probe", probe);
        const int n_bits = o_mode == SC_WRAP_SM ? 3 : 2;
        assign_case<Number>(family, q_mode, o_mode, n_bits, 1, "n_bits_probe", probe);
      }
    }
  }
}

}  // namespace

int sc_main(int, char**)
{
  const auto old_actions = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  try
  {
    run_family<sc_fix>("sc_fix", true);
    run_family<sc_ufix>("sc_ufix", false);
    run_family<sc_fix_fast>("sc_fix_fast", true);
    run_family<sc_ufix_fast>("sc_ufix_fast", false);
  }
  catch (...)
  {
    sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, old_actions);
    throw;
  }
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, old_actions);
  std::cout << "SC_DT_POLICY_MATRIX_OK\n";
  return 0;
}
