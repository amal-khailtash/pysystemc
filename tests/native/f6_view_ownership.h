/** @file f6_view_ownership.h
 *  @brief Return exact const fixed-point proxy types for ownership tests.
 *  @details Each proxy owns its native proxy shell and borrows the supplied
 *  fixed-point number. Callers must keep that number alive for every proxy
 *  access, then destroy the proxy before ending its C++ owner lifetime. These
 *  helpers retain no Python references, callbacks, or buffers. Calls are
 *  synchronous with the GIL held and do not enter the SystemC scheduler.
 */
#ifndef PYSYSTEMC_TEST_F6_VIEW_OWNERSHIP_H
#define PYSYSTEMC_TEST_F6_VIEW_OWNERSHIP_H

#include <sysc/datatypes/fx/sc_fixed.h>

namespace f6_view_ownership
{

/** @brief Return a read-only bit proxy borrowing a fixed-point number.
 *  @param number Live fixed-point owner retained by the caller.
 *  @param index Storage-bit index accepted by native SystemC.
 *  @return Owned const proxy shell borrowing number.
 *  @throws sc_core::sc_report If the native index is invalid.
 */
inline sc_dt::sc_fxnum_bitref_r fixed_const_bit(const sc_dt::sc_fxnum& number, int index)
{
  return number.bit(index);
}

/** @brief Return a read-only bit proxy borrowing a fast fixed-point number.
 *  @param number Live fast fixed-point owner retained by the caller.
 *  @param index Storage-bit index accepted by native SystemC.
 *  @return Owned const proxy shell borrowing number.
 *  @throws sc_core::sc_report If the native index is invalid.
 */
inline sc_dt::sc_fxnum_fast_bitref_r fast_const_bit(const sc_dt::sc_fxnum_fast& number, int index)
{
  return number.bit(index);
}

/** @brief Return a read-only range proxy borrowing a fixed-point number.
 *  @param number Live fixed-point owner retained by the caller.
 *  @param high Inclusive high storage-bit index.
 *  @param low Inclusive low storage-bit index.
 *  @return Owned const proxy shell borrowing number.
 *  @throws sc_core::sc_report If native range validation fails.
 */
inline sc_dt::sc_fxnum_subref_r fixed_const_range(const sc_dt::sc_fxnum& number, int high, int low)
{
  return number.range(high, low);
}

/** @brief Return a read-only range proxy borrowing a fast fixed-point number.
 *  @param number Live fast fixed-point owner retained by the caller.
 *  @param high Inclusive high storage-bit index.
 *  @param low Inclusive low storage-bit index.
 *  @return Owned const proxy shell borrowing number.
 *  @throws sc_core::sc_report If native range validation fails.
 */
inline sc_dt::sc_fxnum_fast_subref_r fast_const_range(const sc_dt::sc_fxnum_fast& number, int high, int low)
{
  return number.range(high, low);
}

}  // namespace f6_view_ownership

#endif  // PYSYSTEMC_TEST_F6_VIEW_OWNERSHIP_H
