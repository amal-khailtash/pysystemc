/** @file report_handler_pointer.h
 * @brief Borrowed exact native report-handler pointer tokens.
 */
#pragma once

#include <sysc/utils/sc_report_handler.h>

namespace pysystemc
{
/** @brief Store a native report-handler pointer without Python callable conversion.
 * @details Pointer is borrowed and does not own or retain a Python callback.
 * Native handlers must remain loaded; a Python dispatcher additionally requires
 * its callback retention managed by set_report_handler. Use on simulation thread
 * with GIL held; construction/destruction never call the handler.
 */
struct report_handler_pointer
{
  sc_core::sc_report_handler_proc value = nullptr;
  /** @brief Construct a null token without retaining callbacks. */
  report_handler_pointer() = default;
  /** @brief Borrow an exact native handler pointer for native bridge returns. */
  report_handler_pointer(sc_core::sc_report_handler_proc pointer) : value(pointer)
  {
  }
  /** @brief Copy a borrowed native handler pointer with identical identity. */
  report_handler_pointer(const report_handler_pointer&) = default;
  /** @brief Check whether the token identifies native handler code. */
  bool valid() const
  {
    return value != nullptr;
  }
  /** @brief Compare exact native handler identity, including null. */
  bool operator==(const report_handler_pointer& other) const
  {
    return value == other.value;
  }
  /** @brief Compare exact native handler inequality. */
  bool operator!=(const report_handler_pointer& other) const
  {
    return value != other.value;
  }
};
}  // namespace pysystemc
