/** @file sc_core_byte_heap_audit.h
 * @brief Direct native reference for zero-capacity sc_byte_heap allocation.
 */
#ifndef PYSYSTEMC_TESTS_NATIVE_SC_CORE_BYTE_HEAP_AUDIT_H
#define PYSYSTEMC_TESTS_NATIVE_SC_CORE_BYTE_HEAP_AUDIT_H

#include <cstdint>
#include <systemc>

namespace sc_core_byte_heap_audit
{
/**
 * @brief Check the native zero-byte allocation pointer and cursor state.
 * @param heap Borrowed initialized zero-capacity heap, retained by the caller for this synchronous probe.
 * @return True when native `allocate(0)` returns the storage pointer and leaves the cursor at that pointer.
 * @pre `heap` is initialized with zero capacity. The helper performs no pointer arithmetic on a null pointer.
 * @details This probe calls the upstream `sc_byte_heap::allocate(std::size_t)` overload directly and compares pointer
 *          values only; it never accesses the returned storage. It retains no Python reference and invokes no callback.
 */
inline bool zero_capacity_allocation_matches_native_storage(sc_core::sc_byte_heap& heap)
{
  char* const storage = heap.m_bgn_p;
  if (storage == nullptr || heap.m_end_p != storage || heap.m_next_p != storage) return false;
  char* const allocation = heap.allocate(0);
  return allocation == storage && heap.m_end_p == storage && heap.m_next_p == storage;
}
}  // namespace sc_core_byte_heap_audit

#endif  // PYSYSTEMC_TESTS_NATIVE_SC_CORE_BYTE_HEAP_AUDIT_H
