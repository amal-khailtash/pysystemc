/** @file tlm_analysis_values_probe.h
 *  @brief Native ownership probes for analysis FIFO pointer and wide values.
 *  @details FIFO objects are children of their owner module. Queued pointer
 *  values borrow caller-owned vector storage that remains alive through reads;
 *  arbitrary value payloads are copied into and out of native FIFO storage.
 */
#ifndef PYSYSTEMC_TEST_TLM_ANALYSIS_VALUES_PROBE_H
#define PYSYSTEMC_TEST_TLM_ANALYSIS_VALUES_PROBE_H

#include <tlm_core/tlm_1/tlm_analysis/tlm_analysis_fifo.h>
#include <tlm_core/tlm_1/tlm_analysis/tlm_analysis_triple.h>

#include <cstdint>
#include <stdexcept>
#include <systemc>
#include <vector>

namespace tlm_analysis_values_probe
{

using WideValue = sc_dt::sc_biguint<191>;

/** @brief Describe one pointer dequeued through a native analysis FIFO. */
struct PointerObservation
{
  std::uintptr_t address;
  int first;
  int second;
};

/** @brief Own both arbitrary native FIFO specializations and pointer storage.
 *  @details The `int*` FIFO copies pointer values only; its data belongs to the
 *  owner vector and must stay allocated through all reads. The 191-bit FIFO
 *  copies payload values and needs no external storage.
 */
struct Owner : sc_core::sc_module
{
  tlm::tlm_analysis_fifo<int*> pointer_fifo;
  tlm::tlm_analysis_fifo<WideValue> wide_fifo;
  std::vector<int> pointer_values;

  /** @brief Construct both FIFOs before simulation elaboration completes.
   *  @param name SystemC module name retained through native queue reads.
   */
  explicit Owner(sc_core::sc_module_name name)
    : sc_module(name), pointer_fifo("pointer_fifo"), wide_fifo("wide_fifo"), pointer_values{11, 22, 33, 44}
  {
  }

  /** @brief Return an element pointer without exposing Python pointer arithmetic.
   *  @param index Element offset within stable owner vector storage.
   *  @return Borrowed pointer valid until the owner vector reallocates/dies.
   */
  int* pointer_at(unsigned index)
  {
    return pointer_values.data() + index;
  }

  /** @brief Return a portable identity value for a borrowed pointer. */
  static std::uintptr_t pointer_address(const int* value)
  {
    return reinterpret_cast<std::uintptr_t>(value);
  }
};

/** @brief Pop and inspect the exact pointer value stored by a native FIFO.
 *  @param fifo Native unbounded FIFO whose item type is `int*`.
 *  @return Dequeued address plus values currently visible through that pointer.
 *  @throws std::runtime_error if no non-null pointer is available.
 */
inline PointerObservation pop_pointer(tlm::tlm_analysis_fifo<int*>& fifo)
{
  int* output = nullptr;
  if (!fifo.nb_get(output) || output == nullptr) throw std::runtime_error("int pointer analysis FIFO empty/null");
  return {reinterpret_cast<std::uintptr_t>(output), output[0], output[1]};
}

/** @brief Pop an arbitrary value payload through the exact native nb_get API.
 *  @tparam T Native FIFO payload type, including arbitrary class templates.
 *  @param fifo Native FIFO retaining copied payload values.
 *  @return One copied payload value.
 *  @throws std::runtime_error if the FIFO contains no item.
 */
template <class T>
T pop_value(tlm::tlm_analysis_fifo<T>& fifo)
{
  T output{};
  if (!fifo.nb_get(output)) throw std::runtime_error("native value analysis FIFO empty");
  return output;
}

}  // namespace tlm_analysis_values_probe

#endif  // PYSYSTEMC_TEST_TLM_ANALYSIS_VALUES_PROBE_H
