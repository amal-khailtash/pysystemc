/** @file Exact native range conversions with independent result storage. */
#pragma once

#include <sysc/datatypes/bit/sc_bv_base.h>
namespace pysystemc_datatypes
{
/** @brief Copy a range using its native binary-vector conversion operator.
 * @details The source view and its parent remain borrowed during the call.
 * The returned vector owns independent native storage. Call with the GIL held;
 * no waits/callbacks/references are retained. Native allocation and configured
 * report exceptions propagate with their original policies.
 */
template <class View>
sc_dt::sc_bv_base range_to_vector(const View& view)
{
  return static_cast<sc_dt::sc_bv_base>(view);
}
}  // namespace pysystemc_datatypes
