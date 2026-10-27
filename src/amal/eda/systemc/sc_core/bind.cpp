/** @file bind.cpp
 * @brief Named native binding values and portable indexed placeholders.
 * @details Bound values follow std::bind copying and reference-wrapper rules.
 * Native callable copies are owned; pointers and reference wrappers borrow their
 * targets. The Python adapter retains input proxies and callbacks for the kernel
 * lifetime. Direct calls from Python already hold the GIL. Python-backed callbacks
 * entered from native code re-enter through cppyy, which acquires the GIL; native
 * callbacks do not require it. Callable exceptions propagate, and guarded waits
 * preserve scheduler unwind. Invoke Python-backed callbacks only through cppyy's
 * callback bridge from native threads.
 */
#include <functional>
#include <memory>
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
 * @details The binding owns values copied by native std::bind. Pointers and
 * reference wrappers borrow their targets, which must outlive every invocation.
 * The Python adapter retains callback and argument proxies for kernel lifetime.
 * Calls made directly from Python run with the GIL held. Python-backed callbacks
 * entered from native code acquire it through cppyy; native callbacks do not
 * require it. Callable exceptions propagate, including scheduler unwind.
 */
template <class Function, class... Arguments>
struct python_bound
{
  using binding_type = decltype(sc_bind(std::declval<Function>(), std::declval<Arguments>()...));
  binding_type function;

  /** @brief Construct the native binding from callable and bound arguments.
   * @param callback Callable copied or moved into native std::bind.
   * @param arguments Bound values copied/moved or borrowed according to native
   * std::bind and reference-wrapper rules.
   * @throws Any exception raised while copying or moving the native binding.
   */
  python_bound(Function callback, Arguments... arguments)
    : function(sc_bind(std::move(callback), std::move(arguments)...))
  {
  }

  /** @brief Invoke the binding and restore pending Python reset/kill unwind.
   * @param binding Mutable or const native std::bind object.
   * @param values Call arguments forwarded to native std::bind for this call.
   * @return The native result by value, or the original borrowed reference.
   * @throws Callable exceptions and pending native scheduler unwind.
   * @details Call arguments are borrowed during invocation. Returned references
   * remain borrowed. Python callbacks entered from native code rely on cppyy to
   * acquire the GIL; do not enter Python callbacks from native threads by another
   * route. Native callback code follows its own SystemC thread requirements.
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
      if constexpr (std::is_move_constructible_v<result_type>)
      {
        dispatch_python([&] { result.emplace(binding(std::forward<Values>(values)...)); });
        return result_type(std::move(*result));
      }
      else
      {
        static_assert(std::is_copy_constructible_v<result_type>,
                      "python_bound value results must be movable or copyable");
        dispatch_python(
          [&]
          {
            const result_type value = binding(std::forward<Values>(values)...);
            result.emplace(value);
          });
        return result_type(static_cast<const result_type&>(*result));
      }
    }
  }
  /** @brief Invoke a mutable native binder with borrowed call arguments.
   * @param values Call arguments forwarded to the native binder.
   * @return Native value or borrowed reference returned by the binder.
   * @throws Callable exceptions and pending native scheduler unwind.
   */
  template <class... Values>
  decltype(auto) operator()(Values&&... values)
  {
    return invoke(function, std::forward<Values>(values)...);
  }
  /** @brief Preserve native const binder calls, including nested bind expressions.
   * @param values Call arguments forwarded to the native binder.
   * @return Native value or borrowed reference returned by the binder.
   * @throws Callable exceptions and pending native scheduler unwind.
   */
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
