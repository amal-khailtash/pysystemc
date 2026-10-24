"""Verify public byte heap, time parameter and inactive unwind behavior."""

from amal.eda.systemc import sc_core as sc
import cppyy
import gc
import sys
import weakref

cppyy.cppdef(
    """
    #include <systemc>
    namespace heap_unwind_probe {
    /** Expose protected construction for an inactive marker on a live process. */
    class unwind_probe : public sc_core::sc_unwind_exception {
    public:
      /** Borrow the current process; no active kernel throw status is changed. */
      explicit unwind_probe(sc_core::sc_process_b* process, bool reset)
        : sc_core::sc_unwind_exception(process, reset) {}
      /** Confirm that synthetic construction leaves this marker inactive. */
      bool is_active() const { return active(); }
    };
    /** Throw a copied inactive marker into Python while its GIL remains held. */
    void throw_inactive_unwind(bool reset) {
      unwind_probe original(static_cast<sc_core::sc_process_b*>(sc_core::sc_get_current_process_handle()), reset);
      if (original.is_active()) throw std::runtime_error("synthetic unwind marker unexpectedly active");
      throw sc_core::sc_unwind_exception(original);
    }
    }
    """
)


class Model(sc.sc_module):
    """Own the thread that runs all probes in a fresh SystemC kernel."""

    def __init__(self, name):
        """Register a thread for native-lifetime-sensitive unwind probes."""
        super().__init__(sc.sc_module_name(name))
        self.sc_thread("run", self.run)

    def run(self):
        """Compare direct heap views, constructor defaults and copied exceptions."""
        empty = sc.sc_byte_heap()
        empty_fields = not empty.m_bgn_p and not empty.m_end_p and not empty.m_next_p
        try:
            empty.allocate(bytes_n=1)
        except ValueError:
            uninitialized_rejected = True
        else:
            raise AssertionError("allocate accepted an uninitialized heap")
        empty.initialize()
        default_bytes = empty.allocate(8)
        default_initialized = empty.length() == 0x100000
        default_zero = len(default_bytes) == 8 and all(default_bytes[index] == 0 for index in range(8))
        assert empty_fields and uninitialized_rejected and default_initialized and default_zero
        print("HEAP_DEFAULT_REFERENCE", int(empty_fields), int(default_initialized), int(default_zero))

        heap = sc.sc_byte_heap(40)
        initial_length = heap.length() == 40
        invalid_sizes_rejected = True
        for invalid, error in ((1.5, TypeError), (True, TypeError), (-1, ValueError), (41, ValueError)):
            try:
                heap.allocate(bytes_n=invalid)
            except error:
                continue
            invalid_sizes_rejected = False
            break
        assert invalid_sizes_rejected and heap.length() == 40
        zero_bytes = heap.allocate(bytes_n=0)
        zero_length = len(zero_bytes) == 0
        first = heap.allocate(1)
        first_address = cppyy.addressof(first)
        zero_noop = zero_length
        first_zero = len(first) == 1 and first[0] == 0
        first_aligned = first_address % 8 == 0
        first[0] = 0x31
        second = heap.allocate(8)
        second_address = cppyy.addressof(second)
        second_offset = second_address - first_address
        second_zero = len(second) == 8 and all(second[index] == 0 for index in range(8))
        second[0] = 0x42
        third = heap.allocate(9)
        third_address = cppyy.addressof(third)
        third_offset = third_address - first_address
        reused = heap.allocate(8)
        wrap_reuses_storage = cppyy.addressof(reused) == first_address
        reused[0] = 0x53
        reuse_written = first[0] == 0x53
        after_wrap = heap.allocate(1)
        wrapped_next_offset = cppyy.addressof(after_wrap) - first_address
        heap_values = (
            empty_fields,
            initial_length,
            zero_noop,
            first_zero,
            first_aligned,
            second_offset,
            third_offset,
            wrap_reuses_storage,
            wrapped_next_offset,
            reuse_written,
        )
        assert all(heap_values[:4]) and heap_values[5:7] == (8, 16) and second_zero
        assert heap_values[7:] == (True, 8, True)
        assert len(third) == 9 and len(reused) == 8 and len(after_wrap) == 1
        print("HEAP_REFERENCE", *(int(value) for value in heap_values))

        heap.initialize(64)
        resized_state = heap.length() == 64
        resized_first = heap.allocate(8)
        resized_zero = len(resized_first) == 8 and all(resized_first[index] == 0 for index in range(8))
        resized_aligned = cppyy.addressof(resized_first) % 8 == 0
        assert resized_state and resized_zero and resized_aligned
        print("HEAP_RESIZE_REFERENCE", int(resized_state), int(resized_zero), int(resized_aligned))

        lifecycle_heap = sc.sc_byte_heap(8)
        borrowed_view = lifecycle_heap.allocate(8)
        borrowed_length = len(borrowed_view)
        heap_finalized = []
        weakref.finalize(lifecycle_heap, heap_finalized.append, "released")
        del lifecycle_heap
        gc.collect()
        heap_destroyed = heap_finalized == ["released"] and borrowed_length == 8
        assert heap_destroyed
        print("HEAP_DESTROY_REFERENCE", int(heap_destroyed))

        params = sc.sc_time_params()
        time_values = (
            int(params.time_resolution),
            int(params.time_resolution_specified),
            int(params.time_resolution_fixed),
            int(params.default_time_unit),
            int(params.default_time_unit_specified),
        )
        assert time_values == (1000, 0, 0, 1000, 0)
        print("TIME_PARAMS_REFERENCE", *time_values)
        finalized = []
        weakref.finalize(params, finalized.append, "released")
        del params
        gc.collect()
        assert finalized == ["released"]
        print("TIME_PARAMS_DESTROY_REFERENCE", int(finalized == ["released"]))
        sys.stdout.flush()

        for reset in (False, True):
            try:
                cppyy.gbl.heap_unwind_probe.throw_inactive_unwind(reset)
            except sc.sc_unwind_exception as exception:
                assert exception.is_reset() == reset
                expected_what = "RESET" if reset else "KILL"
                assert exception.what() == expected_what
                print("UNWIND_REFERENCE", exception.what(), int(exception.is_reset()))
            else:
                raise AssertionError("inactive unwind copy did not cross the Python boundary")
        sc.sc_stop()


Model("heap_unwind_public")
sc.sc_start()
