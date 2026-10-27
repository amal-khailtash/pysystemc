/** @file sc_core_copy_only_values.h
 * @brief Copy-only transaction used to verify native callback value ownership.
 */
#pragma once

#include <stdexcept>

namespace copy_only
{
/** @brief A value with a copy constructor but no default constructor or assignment.
 * @details Instances own scalar storage; live counts measure native destruction.
 * Copy rejection throws before construction and never mutates the source.
 * Access counters on the simulation thread with the GIL held for Python calls.
 */
struct transaction
{
  inline static int live = 0;
  inline static bool reject_copy = false;
  int value;

  /** @brief Construct caller-owned native storage. */
  explicit transaction(int input) : value(input)
  {
    ++live;
  }
  transaction() = delete;
  /** @brief Copy into independently owned storage or reject before mutation. */
  transaction(const transaction& input) : value(input.value)
  {
    if (reject_copy) throw std::runtime_error("copy-only transaction rejected");
    ++live;
  }
  transaction& operator=(const transaction&) = delete;
  /** @brief Release owned storage without callbacks or exceptions. */
  ~transaction()
  {
    --live;
  }
};
}  // namespace copy_only
