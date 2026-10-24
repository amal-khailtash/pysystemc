/** @file vector_creator.cpp
 * @brief Explicit callable conversion for arbitrary native vector elements.
 * @details The vector owns created elements. Python callbacks are borrowed only
 * during synchronous initialization and run with the GIL acquired. Python
 * transfers returned native objects to vector ownership and retains callback
 * hosts for that vector's lifetime. Native initialization rollback deletes any
 * elements created before an exception; no kernel globals are reset.
 */
#include <functional>
#include <systemc>

namespace pysystemc_vectors
{
/** @brief Instantiate an explicit creator signature without restricting element types. */
template <class Element>
struct creator
{
  /** @brief Initialize a vector with a synchronous Python-backed creator.
   * @param vector Owning native vector.
   * @param size Requested number of elements, subject to native initialization rules.
   * @param callback Borrowed callback returning newly allocated native elements.
   * @details Returned elements transfer ownership to the vector. The GIL remains
   * held for initialization; cppyy callback conversion errors and native reports
   * propagate, with native rollback of partially constructed vectors.
   */
  static void initialize(sc_core::sc_vector<Element>& vector, std::size_t size,
                         std::function<Element*(const char*, std::size_t)> callback)
  {
    vector.init(size, std::move(callback));
  }
};
/** @brief Preserve a copied native creator through an explicit callable signature. */
template <class Element, class Creator>
struct native_creator
{
  /** @brief Invoke the native templated initialization overload with an owning creator copy.
   * @details GIL remains held. The vector owns returned native allocations; the
   * creator is consumed synchronously. Native reports and rollback propagate.
   */
  static void initialize(sc_core::sc_vector<Element>& vector, std::size_t size, Creator callback)
  {
    vector.init(size, std::move(callback));
  }
};
}  // namespace pysystemc_vectors
