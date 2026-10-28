/**
 * @file native_blocking.h
 * @brief Preserve scheduler unwind when Python invokes an arbitrary native callable.
 */
#pragma once

#include <functional>
#include <optional>
#include <type_traits>

namespace sc_core
{
/** @brief Execute borrowed native work with the GIL released, capturing reset/kill for Python cleanup. */
bool python_guarded_operation(const std::function<void()>& operation);

/** @brief Reject invalid callable signatures before instantiating executable bridge methods. */
template <class Operation, class = void>
struct python_native_blocking_traits
{
  /** @brief A non-callable native object has no supported blocking invocation. */
  static constexpr bool supported()
  {
    return false;
  }
};

/** @brief Inspect callable return contracts without creating invalid optional/reference storage. */
template <class Operation>
struct python_native_blocking_traits<Operation, std::void_t<std::invoke_result_t<const Operation&>>>
{
  using result_type = std::invoke_result_t<const Operation&>;
  /** @brief Accept only void or copyable value returns, never borrowed pointers/references. */
  static constexpr bool supported()
  {
    return !std::is_reference_v<result_type> && !std::is_pointer_v<result_type> &&
           (std::is_void_v<result_type> || std::is_copy_constructible_v<result_type>);
  }
};

/** @brief Own an independent native result until Python retrieves a value copy. */
template <class Result>
struct python_blocking_result
{
  std::optional<Result> storage;
  bool completed = false;

  /** @brief Return a value copy; incomplete calls must unwind rather than access storage. */
  Result value() const
  {
    return storage.value();
  }
};

/** @brief Completion token for a native operation with no return value. */
template <>
struct python_blocking_result<void>
{
  bool completed = false;

  /** @brief Return no value after completed native execution. */
  void value() const
  {
  }
};

/**
 * @brief Invoke native functors without letting cppyy translate scheduler control exceptions.
 * @details The caller releases the GIL and retains the functor and its borrowed arguments through
 * suspension. The functor must execute in C++, not reenter Python to invoke an unguarded native
 * operation. Reset/kill is saved for the existing process dispatcher; Python must raise its unwind
 * signal before returning. Ordinary native exceptions propagate. Values are independently copied;
 * pointer/reference returns require an explicit copying native functor. Move-only results are
 * rejected before invocation; borrowed storage inside a value retains its normal native lifetime.
 */
template <class Operation>
struct python_native_blocking
{
  using result_type = std::invoke_result_t<const Operation&>;

  /** @brief Borrow and execute the native callable, returning owned storage and completion status. */
  static python_blocking_result<result_type> invoke(const Operation& operation)
  {
    static_assert(python_native_blocking_traits<Operation>::supported(), "unsupported native blocking return contract");
    python_blocking_result<result_type> result;
    result.completed = python_guarded_operation(
      [&]
      {
        if constexpr (std::is_void_v<result_type>)
          operation();
        else
        {
          const auto value = operation();
          result.storage.emplace(value);
        }
      });
    return result;
  }
};
}  // namespace sc_core
