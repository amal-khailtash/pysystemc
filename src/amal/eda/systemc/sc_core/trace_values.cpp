/** @file trace_values.cpp
 * @brief Stable native value storage for exact trace-file virtual dispatch.
 * @details Python owns each value holder and retains it with registered trace
 * files through closure. Native trace files borrow the contained value. Calls
 * hold the GIL; no Python callbacks or scheduler entry occur. Native conversion,
 * registration and allocation errors propagate unchanged.
 */
#include <cstdint>
#include <systemc>
#include <type_traits>
#include <utility>
#include <vector>

namespace pysystemc_tracing
{
/** @brief Dispatch a borrowed char address through SystemC's exact char overload.
 * @param file Open trace file; native tracing handles a null file as usual.
 * @param address Address of at least one live char, or zero for a null pointer.
 * @param name Native trace name.
 * @param width Requested native char trace width; the native default is used
 * when omitted.
 * @details Python wrappers retain the typed ctypes pointer or array through
 * matching file closure. With a native-created file, the caller retains that
 * owner through closure. The address is borrowed, never copied or retained here.
 * Calls hold the GIL and invoke no Python callbacks. Native reports and
 * conversion exceptions propagate unchanged.
 */
inline void trace_char_pointer(sc_core::sc_trace_file* file, std::uintptr_t address, const std::string& name,
                               int width = 8 * sizeof(char))
{
  using char_trace_overload = void (*)(sc_core::sc_trace_file*, const char*, const std::string&, int);
  const auto trace = static_cast<char_trace_overload>(&sc_core::sc_trace);
  trace(file, reinterpret_cast<const char*>(address), name, width);
}

/** @brief Own one unrestricted native value until its trace consumers close.
 * @tparam Value A type accepted by the native trace-file overload set.
 */
template <class Value>
struct trace_value
{
  Value value;
  /** @brief Copy/move an initial native value into independently owned storage. */
  explicit trace_value(Value initial) : value(std::move(initial))
  {
  }
};

/** @brief Select native virtual trace overloads without converting reference storage.
 * @tparam Value The holder's exact native type; no precompiled type list applies.
 */
template <class Value>
struct value_dispatch
{
  /** @brief Register stable borrowed storage with its exact native scalar type.
   * @param file Live trace file; retain the holder until file closure.
   * @param holder Python-owned native value whose member address remains stable.
   * @param name Native trace name.
   * @details GIL held; native report and conversion errors propagate.
   */
  static void trace(sc_core::sc_trace_file& file, const trace_value<Value>& holder, const std::string& name)
  {
    file.trace(holder.value, name);
  }
  /** @brief Register integer storage with the caller's native bit-width argument.
   * @param file Live trace file borrowing the holder's member storage.
   * @param holder Owned value retained through file closure.
   * @param name Native trace name.
   * @param width Native integer trace width; native validity rules apply.
   */
  static void trace(sc_core::sc_trace_file& file, const trace_value<Value>& holder, const std::string& name, int width)
  {
    file.trace(holder.value, name, width);
  }
};

/** @brief Dispatch stable holders through the native free tracing overloads.
 * @tparam Value Exact native type of the independently owned value.
 * @details Calls hold the GIL. Files borrow storage; Python retains the holder
 * through matching closure. Native reports and conversion errors propagate.
 */
template <class Value>
struct free_value_dispatch
{
  /** @brief Register an exact native reference, using native default widths. */
  static void trace(sc_core::sc_trace_file* file, const trace_value<Value>& holder, const std::string& name)
  {
    sc_core::sc_trace(file, holder.value, name);
  }
  /** @brief Forward a native integer width without replacing reference storage. */
  static void trace(sc_core::sc_trace_file* file, const trace_value<Value>& holder, const std::string& name, int width)
  {
    sc_core::sc_trace(file, holder.value, name, width);
  }
};

/** @brief Select signal tracing without dropping explicit width arguments.
 * @tparam Value Exact unrestricted native signal datatype.
 * @details Structural signal refresh retains the existing native process/GIL
 * contract. The caller keeps signals alive through tracing and simulation.
 */
template <class Value>
struct signal_dispatch
{
  /** @brief Trace native storage with its default native width. */
  static void trace(sc_core::sc_trace_file* file, const sc_core::sc_signal_in_if<Value>& signal,
                    const std::string& name)
  {
    sc_core::trace_signal<Value>(file, signal, name);
  }
  /** @brief Forward a width to native tracing while preserving structural refresh. */
  static void trace(sc_core::sc_trace_file* file, const sc_core::sc_signal_in_if<Value>& signal,
                    const std::string& name, int width)
  {
    static_assert(std::is_same_v<Value, char> || std::is_same_v<Value, short> || std::is_same_v<Value, int> ||
                    std::is_same_v<Value, long>,
                  "native signal tracing accepts explicit widths only for char, short, int and long");
    if (file) sc_core::trace_signal<Value, int>(file, signal, name, width);
  }
};

/** @brief Preserve native input/output port registration before elaboration.
 * @tparam Value Unrestricted port datatype, instantiated by cppyy on demand.
 * @details Ports and bound interfaces remain caller-owned through tracing.
 * Calls hold the GIL, retain no callbacks and propagate native reports.
 */
template <class Value>
struct port_dispatch
{
  /** @brief Register an input port using native delayed elaboration tracing. */
  static void input(sc_core::sc_trace_file* file, const sc_core::sc_in<Value>& port, const std::string& name)
  {
    sc_core::sc_trace(file, port, name);
  }
  /** @brief Register an inout/output port using the native port overload. */
  static void inout(sc_core::sc_trace_file* file, const sc_core::sc_inout<Value>& port, const std::string& name)
  {
    sc_core::sc_trace(file, port, name);
  }
};

/** @brief Own immutable enumeration labels and their native null-terminated view.
 * @details The file borrows label pointers. Retain this object until closure;
 * copying is disabled because pointers must reference this object's strings.
 */
class enum_labels
{
  std::vector<std::string> names_;
  std::vector<const char*> pointers_;

 public:
  /** @brief Copy labels into stable storage and append the required null pointer. */
  explicit enum_labels(std::vector<std::string> names) : names_(std::move(names))
  {
    for (const auto& name : names_) pointers_.push_back(name.c_str());
    pointers_.push_back(nullptr);
  }
  enum_labels(const enum_labels&) = delete;
  enum_labels& operator=(const enum_labels&) = delete;
  /** @brief Borrow the label array while this owner remains alive. */
  const char** data() const
  {
    return const_cast<const char**>(pointers_.data());
  }
};
/** @brief Preserve the exact unsigned-integer enumeration trace signature. */
struct enum_dispatch
{
  /** @brief Register stable value and label storage with the native virtual method.
   * @details GIL held; file borrows both owners through closure. Native errors propagate.
   */
  static void trace(sc_core::sc_trace_file& file, const trace_value<unsigned int>& holder, const std::string& name,
                    const enum_labels& labels)
  {
    file.trace(holder.value, name, labels.data());
  }
  /** @brief Preserve native free-function enum tracing and its deprecation report. */
  static void free_trace(sc_core::sc_trace_file* file, const trace_value<unsigned int>& holder, const std::string& name,
                         const enum_labels& labels)
  {
    sc_core::sc_trace(file, holder.value, name, labels.data());
  }
};
}  // namespace pysystemc_tracing
