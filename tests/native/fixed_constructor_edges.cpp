/** @file fixed_constructor_edges.cpp
 * @brief Native reference for remaining fixed-point constructor paths.
 * @details Compares scalar, fixed-value, policy and observer behavior with the
 *          Python model. Native values own their representation; observers
 *          are borrowed only while the matching number is alive. All helpers
 *          are synchronous and require no Python callback or retained GIL.
 */
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>

#include "sc_dt_fixed_templates_probe.h"

namespace probe = fixed_templates_probe;

namespace
{
/** @brief Render value, width, policies, observer identity and callback counts. */
template <class Number, class Observer>
std::string render(const Number& value, const Observer& observer)
{
  std::ostringstream out;
  out << value.to_string(sc_dt::SC_DEC) << '|' << value.wl() << '|' << value.iwl() << '|'
      << static_cast<int>(value.q_mode()) << '|' << static_cast<int>(value.o_mode()) << '|'
      << (value.cast_switch() == sc_dt::SC_ON) << '|' << probe::observer_matches(value, &observer) << '|'
      << observer.constructed << '|' << observer.reads << '|' << observer.writes;
  return out.str();
}

/** @brief Emit one stable constructor record for Python parity. */
void emit(const char* label, const std::string& value)
{
  std::cout << "FCE\t" << label << '\t' << value << '\n';
}

/** @brief Check source and policy constructors for arbitrary-precision templates. */
void template_cases()
{
  using Number = sc_dt::sc_fixed<61, 50>;
  probe::Observer observer;
  const sc_dt::sc_fxcast_switch cast(sc_dt::SC_OFF);

  Number small(7, &observer);
  emit("small_int", render(small, observer));
  Number floating(1.375, &observer);
  emit("floating", render(floating, observer));
  Number native_scalar(static_cast<sc_dt::uint64>((1ULL << 40) + 3), &observer);
  emit("native_scalar", render(native_scalar, observer));
  const sc_dt::sc_fxval fixed_source("2.375");
  Number fixed_value(fixed_source, &observer);
  emit("fixed_value", render(fixed_value, observer));
  const sc_dt::sc_fxval cast_source("3.625");
  Number cast_value(cast_source, cast, &observer);
  emit("template_source_cast_observer", render(cast_value, observer));
}

/** @brief Check configurable policies and unsigned fast-template conversion. */
void configured_cases()
{
  probe::Observer observer;
  sc_dt::sc_fix modes(1.9375, sc_dt::SC_RND_CONV, sc_dt::SC_SAT, &observer);
  emit("config_modes", render(modes, observer));

  probe::Observer width_observer;
  sc_dt::sc_fix width_modes(2.375, 19, 11, sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2, &width_observer);
  emit("config_width_modes_n", render(width_modes, width_observer));

  probe::FastObserver fast_observer;
  using FastNumber = sc_dt::sc_fixed_fast<41, 32>;
  FastNumber fast(static_cast<sc_dt::uint64>((1ULL << 30) + 9), &fast_observer);
  emit("fast_native_scalar", render(fast, fast_observer));
}

/** @brief Check default/copy behavior for a native fixed-template value. */
void default_copy_case()
{
  using Number = sc_dt::sc_fixed<23, 11>;
  Number default_value;
  std::ostringstream default_out;
  default_out << default_value.to_string(sc_dt::SC_DEC) << '|' << default_value.wl() << '|' << default_value.iwl();
  emit("default", default_out.str());

  Number source(7.5);
  Number copied_value(source);
  source = 0;
  std::ostringstream copy_out;
  copy_out << copied_value.to_string(sc_dt::SC_DEC) << '|' << copied_value.wl() << '|' << copied_value.iwl();
  emit("copy", copy_out.str());
}
}  // namespace

static_assert(!std::is_constructible_v<sc_dt::sc_fixed<29, 13>, int, const sc_dt::sc_fxcast_switch&, int>,
              "an unrelated trailing constructor argument must be rejected natively");

/** @brief Emit records used to compare native SystemC constructor semantics. */
int sc_main(int, char**)
{
  template_cases();
  configured_cases();
  default_copy_case();
  return 0;
}
