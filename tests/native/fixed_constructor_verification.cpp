/** @file fixed_constructor_verification.cpp
 * @brief Native reference for selected fixed-point owner constructors.
 * @details Checks equivalent source and policy overloads, borrowed observer
 *          addresses, exact wide integer values, and value copies surviving
 *          destruction of their owning fixed-point number.
 */
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>

#include "sc_dt_fixed_templates_probe.h"
#include "sc_dt_fxnums_probe.h"

namespace observer_probe = fixed_templates_probe;
namespace owner_probe = sc_dt_fxnums_probe;

namespace
{
/** @brief Render exact value, fixed-point policy, cast, and observer state. */
template <class Number, class Observer>
std::string render(const Number& value, const Observer& observer)
{
  std::ostringstream out;
  out << value.to_string(sc_dt::SC_DEC) << '|' << value.wl() << '|' << value.iwl() << '|'
      << static_cast<int>(value.q_mode()) << '|' << static_cast<int>(value.o_mode()) << '|' << value.n_bits() << '|'
      << (value.cast_switch() == sc_dt::SC_ON) << '|' << observer_probe::observer_matches(value, &observer);
  return out.str();
}

/** @brief Emit one stable native-comparable value or rejection record. */
void emit(const char* label, const std::string& value)
{
  std::cout << "FCV\t" << label << '\t' << value << '\n';
}

/** @brief Emit an exact native snapshot record with its two fields separated. */
void emit(const char* label, const std::string& integer, const std::string& binary)
{
  std::cout << "FCV\t" << label << '\t' << integer << '\t' << binary << '\n';
}

/** @brief Verify public fixed-template constructor overloads. */
void fixed_template_cases(const std::string& wide_integer)
{
  observer_probe::Observer observer;
  sc_dt::sc_fxcast_switch cast(sc_dt::SC_OFF);
  using Number = sc_dt::sc_fixed<137, 100>;
  Number cast_only(cast, &observer);
  emit("template_cast_only", render(cast_only, observer));
  Number wide(wide_integer.c_str(), &observer);
  emit("template_wide_integer", render(wide, observer));
}

/** @brief Verify parameter-only and source-plus-parameters dynamic constructors. */
void configurable_cases(const std::string& wide_integer)
{
  observer_probe::Observer observer;
  const sc_dt::sc_fxtype_params params(137, 100, sc_dt::SC_TRN, sc_dt::SC_WRAP, 0);
  const sc_dt::sc_fxcast_switch cast(sc_dt::SC_OFF);
  sc_dt::sc_fix parameter_only(params, cast, &observer);
  emit("config_parameter_only", render(parameter_only, observer));
  sc_dt::sc_fix source(wide_integer.c_str(), params, cast, &observer);
  emit("config_wide_integer", render(source, observer));
}

/** @brief Compare slow and fast owned-number constructors and value copies. */
template <class Number, class Observer>
void owner_case(const char* label, const std::string& integer, int width, int iwl, Observer& observer)
{
  const sc_dt::sc_fxtype_params params(width, iwl, sc_dt::SC_TRN, sc_dt::SC_WRAP, 0);
  const sc_dt::sc_fxcast_switch cast(sc_dt::SC_OFF);
  sc_dt::sc_fxval value_copy("0");
  {
    Number number(integer.c_str(), params, sc_dt::SC_TC_, cast, &observer);
    emit(label, render(number, observer));
    value_copy = number.value();
  }
  emit((std::string(label) + "_snapshot").c_str(), value_copy.to_string(sc_dt::SC_DEC),
       value_copy.to_string(sc_dt::SC_BIN));
}
}  // namespace

/** @brief Emit native results for all three public constructor contracts. */
int sc_main(int, char**)
{
  using namespace sc_dt;
  static_assert(!std::is_default_constructible_v<owner_probe::Fxnum>);
  static_assert(!std::is_default_constructible_v<owner_probe::FxnumFast>);
  const std::string wide_integer = "1237940039285380274899203449";
  const std::string fast_integer = "1099511776907";

  fixed_template_cases(wide_integer);
  configurable_cases(wide_integer);

  observer_probe::Observer slow_observer;
  owner_case<owner_probe::Fxnum>("owner_slow", wide_integer, 137, 100, slow_observer);
  observer_probe::FastObserver fast_observer;
  owner_case<owner_probe::FxnumFast>("owner_fast", fast_integer, 53, 45, fast_observer);
  return 0;
}
