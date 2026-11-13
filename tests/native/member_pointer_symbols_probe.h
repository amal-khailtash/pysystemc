/** @file member_pointer_symbols_probe.h
 * @brief Native runtime member-pointer values for independent assembly checks.
 * @details Values are copied, not byte-cast. Vectors own their elements;
 * assemblies borrow them. Helpers invoke no Python callbacks and do not wait.
 */
#ifndef PYSC_MEMBER_POINTER_SYMBOLS_PROBE_H
#define PYSC_MEMBER_POINTER_SYMBOLS_PROBE_H
#include <systemc>
#include <type_traits>
namespace member_pointer_symbols_probe
{
/** @brief Unrestricted wide value element with two selectable members. */
template <int Width>
struct WideElement
{
  sc_dt::sc_biguint<Width> first;
  sc_dt::sc_biguint<Width> second;
  const int constant = 41;
  explicit WideElement(const char*)
    : first("0x100000000000000000000000000000000011"), second("0x100000000000000000000000000000000035")
  {
  }
};
using Element = WideElement<137>;
using Wide = sc_dt::sc_biguint<137>;
using Pointer = Wide Element::*;
inline Pointer selection = &Element::first;
inline Pointer const fixed = &Element::second;
inline Pointer null_selection = nullptr;
inline const int Element::*const_selection = &Element::constant;
inline int wrong_scalar = 1;
/** @brief Element used to reject an incompatible member-pointer owner. */
struct OtherElement
{
  Wide first;
};
inline Wide OtherElement::*wrong_owner = &OtherElement::first;
/** @brief Base element for native member-pointer conversion to a derived class. */
struct Base
{
  int inherited = 73;
};
/** @brief Derived vector element accepting a native base-member pointer. */
struct Derived : Base
{
  explicit Derived(const char*)
  {
  }
};
inline int Base::*inherited_selection = &Base::inherited;
/** @brief Select a current pointer value without changing earlier copies. */
inline void select_second(bool second)
{
  selection = second ? &Element::second : &Element::first;
}
/** @brief Return a native independent reference assembly from the current symbol.
 * @param vector Borrowed vector, retained by the caller while its assembly lives.
 */
inline auto native_current(sc_core::sc_vector<Element>& vector)
{
  return sc_core::sc_assemble_vector(vector, selection);
}
}  // namespace member_pointer_symbols_probe
#endif
