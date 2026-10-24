/** @file bind.cpp
 * @brief Named native binding values and portable indexed placeholders.
 * @details Bound values follow std::bind copying and reference-wrapper rules.
 * Native callable copies are owned; pointers and reference wrappers borrow their
 * targets. Python retains input proxies for kernel lifetime. Callback entry
 * acquires the GIL; guarded waits and scheduler unwind remain supported.
 */
#include <functional>
#include <optional>
#include <systemc>
#include <type_traits>
#include <utility>

namespace sc_core
{
/** @brief A portable std::bind placeholder with arbitrary positive index.
 * @tparam Index One-based argument position.
 */
template <int Index>
struct python_placeholder
{
  static_assert(Index > 0, "placeholder index must be positive");
  static constexpr int index = Index;
};

/** @brief Own the native binding without exposing implementation-specific types.
 * @tparam Function Native callable value or function pointer.
 * @tparam Arguments Decayed bound values, reference wrappers or placeholders.
 */
template <class Function, class... Arguments>
struct python_bound
{
  using binding_type = decltype(sc_bind(std::declval<Function>(), std::declval<Arguments>()...));
  binding_type function;

  /** @brief Copy inputs following native std::bind value/reference semantics. */
  python_bound(Function callback, Arguments... arguments)
    : function(sc_bind(std::move(callback), std::move(arguments)...))
  {
  }

  /** @brief Invoke the binding and restore pending Python reset/kill unwind.
   * @details Arguments are borrowed during invocation. Returned references remain
   * borrowed. Python callback errors propagate; no scheduler unwind is hidden.
   */
  template <class Binding, class... Values>
  static decltype(auto) invoke(Binding& binding, Values&&... values)
  {
    using result_type = decltype(binding(std::forward<Values>(values)...));
    if constexpr (std::is_void_v<result_type>)
    {
      dispatch_python([&] { binding(std::forward<Values>(values)...); });
    }
    else if constexpr (std::is_reference_v<result_type>)
    {
      std::remove_reference_t<result_type>* result = nullptr;
      dispatch_python([&] { result = std::addressof(binding(std::forward<Values>(values)...)); });
      return static_cast<result_type>(*result);
    }
    else
    {
      std::optional<result_type> result;
      dispatch_python([&] { result.emplace(binding(std::forward<Values>(values)...)); });
      return result_type(std::move(*result));
    }
  }
  /** @brief Invoke a mutable native binder with borrowed call arguments. */
  template <class... Values>
  decltype(auto) operator()(Values&&... values)
  {
    return invoke(function, std::forward<Values>(values)...);
  }
  /** @brief Preserve native const binder calls, including nested bind expressions. */
  template <class... Values>
  decltype(auto) operator()(Values&&... values) const
  {
    return invoke(function, std::forward<Values>(values)...);
  }
};
}  // namespace sc_core

namespace std
{
/** @brief Register the portable placeholder for native std::bind argument substitution. */
template <int Index>
struct is_placeholder<sc_core::python_placeholder<Index>> : integral_constant<int, Index>
{
};
/** @brief Preserve nested binding evaluation rather than passing the binder itself. */
template <class Function, class... Arguments>
struct is_bind_expression<sc_core::python_bound<Function, Arguments...>> : true_type
{
};
}  // namespace std
