/** @file trace_values.cpp
 * @brief Stable native value storage for exact trace-file virtual dispatch.
 * @details Python owns each value holder and retains it with registered trace
 * files through closure. Native trace files borrow the contained value. Calls
 * hold the GIL; no Python callbacks or scheduler entry occur. Native conversion,
 * registration and allocation errors propagate unchanged.
 */
#include <systemc>
#include <utility>
#include <vector>

namespace pysystemc_tracing
{
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
};
}  // namespace pysystemc_tracing
