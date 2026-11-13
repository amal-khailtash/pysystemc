/** @file member_pointer_symbols.cpp
 * @brief Independent native runtime-pointer assembly reference.
 * @details No binding adapters are used. Value copies keep their selection when
 * the source pointer changes; vectors outlive every borrowed assembly.
 */
#include <iostream>
#include <stdexcept>

#include "member_pointer_symbols_probe.h"

/** @brief Check genuine runtime native pointers through both SystemC APIs. */
int sc_main(int, char**)
{
  using namespace member_pointer_symbols_probe;
  sc_core::sc_vector<Element> values("native_symbols", 2);
  Pointer saved = selection;
  auto first = values.assemble(saved);
  select_second(true);
  auto second = sc_core::sc_assemble_vector(values, selection);
  if (saved == selection || &first[0] != &values[0].first || &second[1] != &values[1].second)
    throw std::logic_error("current pointer/value-copy identity");
  auto fixed_values = values.assemble(fixed);
  if (&fixed_values[0] != &values[0].second || null_selection != nullptr)
    throw std::logic_error("const symbol/null pointer");
  auto constants = sc_core::sc_assemble_vector(values, const_selection);
  if (constants[0] != 41) throw std::logic_error("const member");
  sc_core::sc_vector<Derived> derived("native_inherited", 1);
  int Derived::*converted = inherited_selection;
  if (derived.assemble(converted)[0] != 73) throw std::logic_error("inherited conversion");
  static_assert(!std::is_convertible_v<decltype(wrong_owner), Pointer>);
  static_assert(!std::is_convertible_v<decltype(const_selection), int Element::*>);
  std::cout << "MEMBER_SYMBOL_NATIVE first=" << first[0].to_string(sc_dt::SC_DEC)
            << " second=" << second[1].to_string(sc_dt::SC_DEC) << " const=41 inherited=73\n";
  return 0;
}
