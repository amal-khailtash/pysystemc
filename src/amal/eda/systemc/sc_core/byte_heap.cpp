/** @file byte_heap.cpp
 * @brief Preserve borrowed heap byte pointers instead of cppyy string conversion.
 */
#include <systemc>

#include "sysc/utils/sc_temporary.h"

namespace pysystemc_public
{
/** @brief Allocate native temporary storage and expose it as a mutable byte pointer.
 * @param heap Borrowed initialized heap; retain it while accessing the result.
 * @param size Requested byte count; Python validates it against heap capacity.
 * @return Borrowed unsigned-byte view of the native char allocation, without copying.
 * @details Native eight-byte alignment and wrap/reuse are unchanged. The GIL
 * remains held; no callbacks occur. Reinitializing or destroying the heap
 * invalidates the view; later wrapping allocations overwrite its contents.
 */
unsigned char* byte_heap_allocate(sc_core::sc_byte_heap& heap, std::size_t size)
{
  return reinterpret_cast<unsigned char*>(heap.allocate(size));
}
}  // namespace pysystemc_public
