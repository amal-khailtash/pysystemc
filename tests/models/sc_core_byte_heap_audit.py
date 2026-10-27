"""Check empty byte heap allocations against the native SystemC allocator."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_byte_heap_audit.h"))


def main():
    """Compare an empty view with the native pointer and reject an uninitialized heap."""
    heap = sc.sc_byte_heap(0)
    assert cppyy.gbl.pysystemc_public.byte_heap_is_initialized(heap)
    native_matches_storage = cppyy.gbl.sc_core_byte_heap_audit.zero_capacity_allocation_matches_native_storage(heap)
    view = heap.allocate(0)
    assert len(view) == 0
    assert native_matches_storage
    print("BYTE_HEAP_ZERO_CAPACITY", native_matches_storage, len(view))

    rejected_zero_heap_sizes = True
    for size in (-1, 1):
        try:
            heap.allocate(size)
        except ValueError:
            continue
        rejected_zero_heap_sizes = False
        break
    assert rejected_zero_heap_sizes
    print("BYTE_HEAP_ZERO_CAPACITY_BOUNDS", rejected_zero_heap_sizes)

    uninitialized = sc.sc_byte_heap()
    try:
        uninitialized.allocate(0)
    except ValueError:
        print("BYTE_HEAP_UNINITIALIZED_ZERO", True)
    else:
        raise AssertionError("allocate accepted an uninitialized heap")


if __name__ == "__main__":
    main()
