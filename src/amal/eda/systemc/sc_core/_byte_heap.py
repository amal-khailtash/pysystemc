"""Expose native temporary heap allocations as mutable borrowed byte views."""

import cppyy


def allocate(heap, bytes_n):
    """Allocate temporary bytes without copying native storage into a string.

    Args:
        heap: Live initialized native sc_byte_heap (bound as self).
        bytes_n: Nonnegative requested byte count fitting the heap after alignment.

    Returns:
        Borrowed native unsigned-byte view of exactly size bytes, supporting
        indexing, mutation and native pointer conversion.

    Raises:
        TypeError: Size is not an integer.
        ValueError: Heap is uninitialized or aligned request exceeds capacity.

    Keep the heap alive through every access. Reinitialization and destruction
    invalidate all views. Native wrapping allocations reuse storage and can
    overwrite earlier views. This call holds GIL and preserves 8-byte alignment.
    The native heap cursor is not thread-safe; callers must serialize access
    when native threads can use the same heap.
    """
    size = bytes_n
    if isinstance(size, bool) or not isinstance(size, int):
        raise TypeError("allocation size must be an integer")
    initialized = cppyy.gbl.pysystemc_public.byte_heap_is_initialized(heap)
    if size < 0 or not initialized or (size + 7) // 8 * 8 > heap.length():
        raise ValueError("allocation requires an initialized heap with enough capacity")
    view = cppyy.gbl.pysystemc_public.byte_heap_allocate(heap, size)
    view.reshape((size,))
    return view
