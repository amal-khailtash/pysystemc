/** @file Native conversion access for SystemC vector bit proxies.
 * @details Views borrow live parent vectors. Clones allocate shells requiring
 * caller deletion and still borrow parents. All helpers hold the GIL, never
 * suspend or retain Python references; native reports propagate.
 */
#pragma once
#include <memory>
#include <sstream>
#include <systemc>
#include <type_traits>
namespace vector_bitref_probe
{
/** @brief Preserve implicit native conversion to the trait's bit type. */
template <class T>
typename T::traits_type::bit_type converted(const sc_dt::sc_bitref_r<T>& view)
{
  return view;
}
/** @brief Print a view to independent string storage. */
template <class T>
std::string printed(const sc_dt::sc_bitref_r<T>& view)
{
  std::ostringstream out;
  view.print(out);
  return out.str();
}
/** @brief Verify native public aliases for binary and four-state parents. */
inline bool traits()
{
  return std::is_same<sc_dt::sc_bitref_r<sc_dt::sc_lv_base>::traits_type, sc_dt::sc_lv_base::traits_type>::value &&
         std::is_same<sc_dt::sc_bitref_r<sc_dt::sc_bv_base>::traits_type, sc_dt::sc_bv_base::traits_type>::value &&
         std::is_same<sc_dt::sc_bitref_r<sc_dt::sc_bv_base>::value_type, bool>::value &&
         std::is_same<sc_dt::sc_bitref_r<sc_dt::sc_lv_base>::value_type, sc_dt::sc_logic_value_t>::value &&
         std::is_same<sc_dt::sc_bitref<sc_dt::sc_bv_base>::value_type, bool>::value &&
         std::is_same<sc_dt::sc_bitref_r<sc_dt::sc_lv_base>::bit_type, sc_dt::sc_logic>::value &&
         std::is_same<sc_dt::sc_bitref_r<sc_dt::sc_bv_base>::bit_type, sc_dt::sc_logic>::value &&
         std::is_same<sc_dt::sc_bitref<sc_dt::sc_lv_base>::value_type, sc_dt::sc_logic_value_t>::value;
}
}  // namespace vector_bitref_probe
