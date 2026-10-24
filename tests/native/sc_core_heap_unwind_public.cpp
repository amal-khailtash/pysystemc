/** @file sc_core_heap_unwind_public.cpp
 * @brief Native references for public byte heap, time parameter and unwind APIs.
 * @details Heap pointers are observed only while their owning heap allocation is live. The unwind probe
 *          runs inside a native process, copies an inactive exception and catches it before exit.
 */
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <systemc>

namespace
{
using namespace sc_core;

/**
 * @brief Expose protected unwind state while retaining native process lifetime rules.
 * @details The target process is borrowed from the running kernel thread. Instances and copies stay local to that
 *          process; inactive markers are destroyed before the thread returns. No Python reference or callback is kept.
 */
class unwind_probe : public sc_unwind_exception
{
 public:
  /** @brief Bind a temporary unwind marker to the current live process. */
  explicit unwind_probe(sc_process_b* process, bool reset) : sc_unwind_exception(process, reset)
  {
  }

  /** @brief Inspect whether this exception still owns an active unwind marker. */
  bool is_active() const
  {
    return active();
  }
};

/** @brief Throw one deliberately inactive copied marker for the caller to inspect. */
void throw_inactive_unwind(bool reset)
{
  unwind_probe original(static_cast<sc_process_b*>(sc_get_current_process_handle()), reset);
  if (original.is_active()) throw std::runtime_error("synthetic unwind marker unexpectedly active");
  throw sc_unwind_exception(original);
}

/** @brief Catch the synthetic copied marker without changing the kernel throw status. */
void inspect_unwind(bool reset)
{
  try
  {
    throw_inactive_unwind(reset);
  }
  catch (const sc_core::sc_unwind_exception& exception)
  {
    if (exception.is_reset() != reset) throw std::runtime_error("sc_unwind_exception reset flag mismatch");
    std::cout << "UNWIND_REFERENCE " << exception.what() << " " << exception.is_reset() << "\n";
  }
}

/**
 * @brief Inspect allocations while their owning heap buffer remains live.
 * @details The caller owns heap and keeps it alive through this call. Pointers never escape the function. It runs on
 *          the SystemC process thread and throws std::runtime_error on a mismatch; it acquires no GIL and retains no
 *          Python callback.
 */
void inspect_heap(sc_core::sc_byte_heap& heap, bool empty_fields, bool zero_noop)
{
  using namespace sc_core;
  const bool initial_state =
    heap.length() == 40 && heap.m_next_p == heap.m_bgn_p + 8 && heap.m_end_p == heap.m_bgn_p + 40;
  char* first = heap.m_bgn_p;
  const bool first_zero = first[0] == 0;
  const bool first_aligned = reinterpret_cast<std::uintptr_t>(first) % 8 == 0;
  first[0] = 0x31;
  char* second = heap.allocate(8);
  const int second_offset = static_cast<int>(second - heap.m_bgn_p);
  second[0] = 0x42;
  char* third = heap.allocate(9);
  const int third_offset = static_cast<int>(third - heap.m_bgn_p);
  char* reused = heap.allocate(8);
  const bool wrap_reuses_storage = reused == first;
  const int wrapped_next_offset = static_cast<int>(heap.m_next_p - heap.m_bgn_p);
  reused[0] = 0x53;
  const bool reuse_written = first[0] == 0x53;

  if (!empty_fields || !initial_state || !zero_noop || !first_zero || !first_aligned || second_offset != 8 ||
      third_offset != 16 || !wrap_reuses_storage || wrapped_next_offset != 8 || !reuse_written)
    throw std::runtime_error("sc_byte_heap allocation contract mismatch");
  std::cout << "HEAP_REFERENCE " << empty_fields << " " << initial_state << " " << zero_noop << " " << first_zero << " "
            << first_aligned << " " << second_offset << " " << third_offset << " " << wrap_reuses_storage << " "
            << wrapped_next_offset << " " << reuse_written << "\n";
}

/**
 * @brief Inspect a newly initialized replacement buffer while it remains live.
 * @details The caller owns heap; pointers remain local and are not used after return. The function runs on the kernel
 *          process thread, throws std::runtime_error on mismatch, and retains no Python reference or callback.
 */
void inspect_resized_heap(sc_core::sc_byte_heap& heap)
{
  const bool resized_state = heap.length() == 64 && heap.m_next_p == heap.m_bgn_p && heap.m_end_p == heap.m_bgn_p + 64;
  char* resized_first = heap.allocate(8);
  const bool resized_zero = resized_first[0] == 0 && resized_first[7] == 0;
  const bool resized_aligned = reinterpret_cast<std::uintptr_t>(resized_first) % 8 == 0;
  if (!resized_state || !resized_zero || !resized_aligned)
    throw std::runtime_error("sc_byte_heap reinitialization contract mismatch");
  std::cout << "HEAP_RESIZE_REFERENCE " << resized_state << " " << resized_zero << " " << resized_aligned << "\n";
}

/**
 * @brief Emit all public fields initialized by the default time parameter constructor.
 * @details A unique_ptr owns the parameter object and reset invokes its destructor before the lifetime record is
 *          emitted. No callbacks or Python references are involved.
 */
void inspect_time_params()
{
  auto params = std::make_unique<sc_core::sc_time_params>();
  std::cout << "TIME_PARAMS_REFERENCE " << params->time_resolution << " " << params->time_resolution_specified << " "
            << params->time_resolution_fixed << " " << params->default_time_unit << " "
            << params->default_time_unit_specified << "\n";
  params.reset();
  std::cout << "TIME_PARAMS_DESTROY_REFERENCE 1\n";
}

/** @brief Run probes inside a live process and allow normal native destruction afterward. */
class model : public sc_core::sc_module
{
 public:
  SC_HAS_PROCESS(model);

  /** @brief Register one thread for the live-process unwind probe. */
  explicit model(sc_core::sc_module_name name) : sc_module(name)
  {
    SC_THREAD(run);
  }

  /** @brief Execute independent value probes and stop the kernel. */
  void run()
  {
    using namespace sc_core;
    sc_byte_heap empty;
    const bool empty_fields = !empty.m_bgn_p && !empty.m_end_p && !empty.m_next_p;
    empty.initialize();
    char* default_bytes = empty.allocate(8);
    const bool default_initialized = empty.length() == 0x100000;
    const bool default_zero = default_bytes[0] == 0 && default_bytes[7] == 0;
    if (!empty_fields || !default_initialized || !default_zero)
      throw std::runtime_error("sc_byte_heap default initialization contract mismatch");
    std::cout << "HEAP_DEFAULT_REFERENCE " << empty_fields << " " << default_initialized << " " << default_zero << "\n";
    sc_byte_heap heap(40);
    char* zero_bytes = heap.allocate(0);
    char* first = heap.allocate(1);
    const bool zero_noop = zero_bytes == first;
    inspect_heap(heap, empty_fields, zero_noop);
    heap.initialize(64);
    inspect_resized_heap(heap);
    bool heap_destroyed = false;
    {
      sc_byte_heap destruction_probe(8);
      char* borrowed = destruction_probe.allocate(8);
      heap_destroyed = borrowed != nullptr;
    }
    if (!heap_destroyed) throw std::runtime_error("sc_byte_heap destructor probe failed");
    std::cout << "HEAP_DESTROY_REFERENCE 1\n";
    inspect_time_params();
    inspect_unwind(false);
    inspect_unwind(true);
    sc_core::sc_stop();
  }
};
}  // namespace

/** @brief Compare public helper values without relying on a nonzero simulation duration. */
int sc_main(int, char**)
{
  model probe("heap_unwind_public");
  sc_core::sc_start();
  return 0;
}
