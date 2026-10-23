/** @file sc_core_small_gaps_probe.h
 * @brief Native protected event-list copying and active-catch exception consumers.
 */
#ifndef PYSYSTEMC_SMALL_GAPS_PROBE_H
#define PYSYSTEMC_SMALL_GAPS_PROBE_H
#include <stdexcept>
#include <systemc>
namespace small_gaps_probe
{
/** @brief Expose the public base copy constructor with an owning derived list.
 * @details Events remain borrowed; no callback storage or GIL change occurs.
 */
struct List : sc_core::sc_event_list
{
  /** @brief Copy borrowed events through the exact base constructor. */
  explicit List(const sc_core::sc_event_list& source) : sc_event_list(source)
  {
  }
  /** @brief Release the owned list while referenced events remain caller-owned. */
  ~List() = default;
};
/** @brief Translate an active native exception into an owning report pointer.
 * @param kind Select runtime_error, const-char, unknown integer or native report.
 * @return A newly allocated report for the caller to delete, or null if suppressed.
 * @details Calls sc_handle_exception only within a native catch. GIL remains
 * held; no Python callback is retained. Native report actions apply.
 */
inline sc_core::sc_report* exception(int kind)
{
  try
  {
    if (kind == 0) throw std::runtime_error("native runtime failure");
    if (kind == 1) throw "native text failure";
    if (kind == 2) throw 42;
    SC_REPORT_ERROR("small-gaps", "native report failure");
  }
  catch (...)
  {
    return sc_core::sc_handle_exception();
  }
  return nullptr;
}
}  // namespace small_gaps_probe
#endif
