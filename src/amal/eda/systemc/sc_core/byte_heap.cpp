/** @file byte_heap.cpp
 * @brief Preserve borrowed heap byte pointers instead of cppyy string conversion.
 */
#include <cstddef>
#include <systemc>

#include "sysc/utils/sc_temporary.h"

namespace pysystemc_public
{
/**
 * @brief Check whether the heap owns initialized storage without converting its `char*` field to a Python string.
 * @param heap Borrowed heap; ownership remains with the caller.
 * @return True when native initialization has installed a storage pointer.
 * @details This pointer-value check does not inspect or retain storage. It has no callback, exception, or GIL-release
 *          path and is called synchronously while the Python wrapper keeps the GIL held.
 */
bool byte_heap_is_initialized(const sc_core::sc_byte_heap& heap)
{
  return heap.m_bgn_p != nullptr;
}

/**
 * @brief Allocate native temporary storage and expose it as a mutable byte pointer.
 * @param heap Borrowed initialized heap; retain it while accessing the result.
 * @param size Requested byte count, passed unchanged to the native `allocate(std::size_t)` overload.
 * @return Borrowed unsigned-byte pointer to the native char allocation, without copying.
 * @pre The heap is initialized and the native eight-byte-rounded size fits its capacity. The Python wrapper checks
 *      these conditions before calling this adapter.
 * @details The heap retains ownership; the pointer is valid only until heap reinitialization or destruction, and
 *          later wrapping allocations may overwrite its contents. Native eight-byte alignment and wrap/reuse
 *          behavior are unchanged. The Python call keeps the GIL held, invokes no callbacks, and does not suspend
 *          the SystemC kernel. No Python references are retained. The native heap cursor is not thread-safe, so
 *          callers must serialize concurrent native access. The allocator has no exception path for callers
 *          satisfying the precondition.
 */
unsigned char* byte_heap_allocate(sc_core::sc_byte_heap& heap, std::size_t size)
{
  return reinterpret_cast<unsigned char*>(heap.allocate(size));
}
}  // namespace pysystemc_public
