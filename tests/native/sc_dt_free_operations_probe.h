/** @file Native value construction and exact free compound operator probes. */
#pragma once
#include <cstdint>
#include <systemc>
#include <type_traits>
namespace datatype_free_probe
{
/** @brief Construct native arbitrary/fast fixed storage, owning copied values.
 * @details The caller owns results; no callbacks or waits are introduced.
 */
template <class T>
T fixed_value(double value)
{
  constexpr bool fast = std::is_base_of_v<sc_dt::sc_fxnum_fast, T>;
  return T(value, fast ? 19 : 137, fast ? 9 : 65, sc_dt::SC_TRN, sc_dt::SC_WRAP, 0);
}
/** @brief Invoke exact public free operators on borrowed native proxy bases.
 * @details Caller retains both values. Returns borrowed left storage address.
 * Native four-state reports propagate; calls hold the GIL and do not suspend.
 */
template <class X, class Y>
struct Compound
{
  static std::uintptr_t apply(int operation, X& left, const Y& right)
  {
    auto& px = static_cast<sc_dt::sc_proxy<X>&>(left);
    const auto& py = static_cast<const sc_dt::sc_proxy<Y>&>(right);
    X* result = nullptr;
    if (operation == 0)
      result = &sc_dt::operator&=(px, py);
    else if (operation == 1)
      result = &sc_dt::operator|=(px, py);
    else
      result = &sc_dt::operator^=(px, py);
    return reinterpret_cast<std::uintptr_t>(result);
  }
};
}  // namespace datatype_free_probe
