/** @file spawn_result.cpp
 * @brief Typed dynamic-process results preserving Python scheduler unwind.
 * @details The kernel owns callable copies. Result storage is borrowed and must
 * remain alive until process termination. Python retains its proxy and callback
 * for the interpreter's kernel lifetime; cppyy acquires the GIL for callbacks.
 */
#include <functional>
#include <optional>
#include <systemc>
#include <utility>

namespace sc_core
{
/** @brief A native result functor that restores pending reset/kill control flow.
 * @tparam Result Arbitrary native value type, assigned using its native operator.
 * @details The callback copy retains its Python adapter. No callback exception
 * is suppressed; dispatch_python restores original native scheduler unwind.
 */
template <typename Result>
struct python_result_functor
{
  using result_type = Result;
  std::function<Result()> callback;

  /** @brief Execute one callback and return its owning result value. */
  Result operator()()
  {
    std::optional<Result> result;
    dispatch_python([&] { result.emplace(callback()); });
    return std::move(*result);
  }
};

/** @brief Spawn an arbitrary typed result callback into borrowed result storage.
 * @param result Caller-owned native lvalue, retained through process termination.
 * @param callback Retained no-argument callable returning a native-compatible value.
 * @param name Optional native process name.
 * @param options Borrowed configuration consumed synchronously during creation.
 * @return Native process handle; does not transfer result ownership.
 * @details GIL is held at registration. Callback entry reacquires it and guarded
 * waits release it. Native callback errors and reset/kill propagate unchanged.
 */
template <typename Result>
sc_process_handle python_spawn_result(Result& result, std::function<Result()> callback, const char* name = "",
                                      const sc_spawn_options* options = nullptr)
{
  return sc_spawn(&result, python_result_functor<Result>{std::move(callback)}, name, options);
}
}  // namespace sc_core
