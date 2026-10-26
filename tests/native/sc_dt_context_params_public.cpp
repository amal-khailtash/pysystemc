/** @file sc_dt_context_params_public.cpp
 *  @brief Native reference matrix for fixed-point contexts and parameters.
 *  @details Each invocation owns its SystemC kernel and all values are local
 *  to `sc_main`; no context or parameter storage escapes this process.
 */
#include <iostream>
#include <stdexcept>
#include <string>

#include "sc_dt_context_params_probe.h"

namespace
{
using namespace sc_dt;
using sc_dt_context_params_probe::dumped;
using sc_dt_context_params_probe::flatten;
using sc_dt_context_params_probe::printed;

void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

void emit(const char* name, const sc_fxtype_params& value)
{
  std::cout << "FX " << name << ' ' << value.to_string() << ' ' << value.wl() << ' ' << value.iwl() << ' '
            << static_cast<int>(value.q_mode()) << ' ' << static_cast<int>(value.o_mode()) << ' ' << value.n_bits()
            << ' ' << flatten(printed(value)) << ' ' << flatten(dumped(value)) << '\n';
}

void emit(const char* name, const sc_length_param& value)
{
  std::cout << "LENGTH " << name << ' ' << value.to_string() << ' ' << value.len() << ' ' << flatten(printed(value))
            << ' ' << flatten(dumped(value)) << '\n';
}

void emit(const char* name, const sc_fxcast_switch& value)
{
  std::cout << "CAST " << name << ' ' << value.to_string() << ' ' << flatten(printed(value)) << ' '
            << flatten(dumped(value)) << '\n';
}

template <class T, class Context>
void context_matrix(const char* family, T seed, T replacement)
{
  const T baseline;
  std::cout << "CTX " << family << " BASE " << baseline.to_string() << '\n';
  Context later(seed, SC_LATER);
  seed = replacement;
  verify(later.value() != seed, "context did not retain its construction-time copy");
  const T later_default;
  verify(later_default == baseline, "later context changed the parameter default constructor");
  std::cout << "CTX " << family << " LATER " << Context::default_value().to_string() << ' ' << later.value().to_string()
            << ' ' << later_default.to_string() << '\n';
  later.begin();
  verify(Context::default_value() == later.value(), "begin did not select the owned context value");
  const T active_default;
  verify(active_default == later.value(), "parameter default constructor ignored the active context");
  std::cout << "CTX " << family << " ACTIVE " << Context::default_value().to_string() << ' '
            << later.value().to_string() << ' ' << active_default.to_string() << '\n';
  {
    Context now(replacement, SC_NOW);
    verify(Context::default_value() == now.value(), "SC_NOW did not select its value");
    const T nested_default;
    verify(nested_default == now.value(), "parameter default constructor ignored the nested SC_NOW context");
    std::cout << "CTX " << family << " NESTED " << Context::default_value().to_string() << ' '
              << now.value().to_string() << ' ' << nested_default.to_string() << '\n';
  }
  verify(Context::default_value() == later.value(), "nested destruction did not restore the outer context");
  later.end();
  verify(Context::default_value() == baseline, "end did not restore the original default");
  std::cout << "CTX " << family << " RESTORED " << Context::default_value().to_string() << '\n';
  {
    Context active(seed, SC_LATER);
    active.begin();
    verify(Context::default_value() == active.value(), "second begin did not select the active value");
  }
  verify(Context::default_value() == baseline, "active context destruction did not restore the default");
  std::cout << "CTX " << family << " DESTROYED " << Context::default_value().to_string() << '\n';
}

void fixed_type_matrix()
{
  sc_fxtype_params defaults;
  sc_fxtype_params widths(17, 5);
  sc_fxtype_params modes(SC_RND, SC_SAT, 3);
  sc_fxtype_params full(19, 7, SC_RND_CONV, SC_WRAP_SM, 2);
  sc_fxtype_params copied(full);
  sc_fxtype_params changed_widths(full, 23, 11);
  sc_fxtype_params changed_modes(full, SC_RND_MIN_INF, SC_SAT_SYM, 4);
  sc_fxtype_params without_context{sc_without_context()};
  emit("default", defaults);
  emit("widths", widths);
  emit("modes", modes);
  emit("full", full);
  emit("copy", copied);
  emit("copy_widths", changed_widths);
  emit("copy_modes", changed_modes);
  emit("without_context", without_context);
  verify(copied == full && !(copied != full), "fixed type copy or equality mismatch");
  verify(widths != full && !(widths == full), "fixed type inequality mismatch");
  sc_fxtype_params assigned;
  assigned = full;
  assigned = assigned;
  verify(assigned == full, "fixed type assignment mismatch");
  emit("assigned", assigned);
  widths.wl(21);
  widths.iwl(8);
  widths.q_mode(SC_TRN_ZERO);
  widths.o_mode(SC_SAT_ZERO);
  widths.n_bits(5);
  verify(widths.wl() == 21 && widths.iwl() == 8 && widths.q_mode() == SC_TRN_ZERO && widths.o_mode() == SC_SAT_ZERO &&
           widths.n_bits() == 5,
         "fixed type setters/getters mismatch");
  emit("mutated", widths);
}

void length_matrix()
{
  sc_length_param defaults;
  sc_length_param integer(13);
  sc_length_param copied(integer);
  sc_length_param without_context{sc_without_context()};
  verify(copied == integer && !(copied != integer), "length copy or equality mismatch");
  verify(defaults != integer && !(defaults == integer), "length inequality mismatch");
  sc_length_param assigned;
  assigned = integer;
  assigned = assigned;
  verify(assigned == integer, "length assignment mismatch");
  emit("default", defaults);
  emit("integer", integer);
  emit("copy", copied);
  emit("without_context", without_context);
  emit("assigned", assigned);
  integer.len(27);
  verify(integer.len() == 27, "length setter/getter mismatch");
  emit("mutated", integer);
}

void cast_matrix()
{
  sc_fxcast_switch defaults;
  sc_fxcast_switch selected(SC_OFF);
  sc_fxcast_switch copied(selected);
  sc_fxcast_switch without_context{sc_without_context()};
  verify(copied == selected && !(copied != selected), "cast switch copy or equality mismatch");
  verify(defaults != selected && !(defaults == selected), "cast switch inequality mismatch");
  sc_fxcast_switch assigned;
  assigned = selected;
  assigned = assigned;
  verify(assigned == selected, "cast switch assignment mismatch");
  emit("default", defaults);
  emit("selected", selected);
  emit("copy", copied);
  emit("without_context", without_context);
  emit("assigned", assigned);
}

}  // namespace

int sc_main(int, char**)
{
  using namespace sc_dt;
  fixed_type_matrix();
  length_matrix();
  cast_matrix();
  context_matrix<sc_fxtype_params, sc_fxtype_context>("FX", sc_fxtype_params(29, 13),
                                                      sc_fxtype_params(31, 15, SC_RND, SC_SAT, 1));
  context_matrix<sc_length_param, sc_length_context>("LENGTH", sc_length_param(14), sc_length_param(18));
  context_matrix<sc_fxcast_switch, sc_fxcast_context>("CAST", sc_fxcast_switch(SC_OFF), sc_fxcast_switch(SC_ON));
  std::cout << "SC_DT_CONTEXT_PARAMS_PUBLIC_OK\n";
  return 0;
}
