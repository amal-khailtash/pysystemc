/** @file sc_core_vector_creator_probe.h
 * @brief Native vector element lifetime evidence for creator failure and ownership.
 */
#ifndef PYSYSTEMC_VECTOR_CREATOR_PROBE_H
#define PYSYSTEMC_VECTOR_CREATOR_PROBE_H
#include <stdexcept>
#include <systemc>

namespace vector_creator_probe
{
/** @brief Native object with independently counted vector-owned destruction. */
struct Element : sc_core::sc_object
{
  inline static int destroyed = 0;
  int index = -1;
  /** @brief Construct one named element with caller-supplied index. */
  explicit Element(const char* name, int value = -1) : sc_object(name), index(value)
  {
  }
  /** @brief Record vector-driven destruction exactly once per allocated element. */
  ~Element() override
  {
    ++destroyed;
  }
};
/** @brief Fail after two allocations to exercise native vector rollback. */
struct FailingCreator
{
  /** @brief Allocate elements zero/one and throw before allocating element two. */
  Element* operator()(const char* name, std::size_t index) const
  {
    if (index == 2) throw std::runtime_error("creator failure");
    return new Element(name, static_cast<int>(index));
  }
};
}  // namespace vector_creator_probe
#endif
