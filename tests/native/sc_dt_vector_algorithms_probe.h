/** @file sc_dt_vector_algorithms_probe.h
 * @brief Narrow helpers for public SystemC vector algorithm proxies.
 * @details Returned integer subreferences borrow their parent storage, which
 * must remain alive while the proxy is used. Concatenation references point
 * into SystemC's temporary pool and borrow both operands; consume them before
 * allocating another concatenation. Helpers retain no Python objects or
 * callbacks, do not enter the scheduler, and are called with the GIL held.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_VECTOR_ALGORITHMS_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_VECTOR_ALGORITHMS_PROBE_H

#include <systemc>

namespace sc_dt_vector_algorithms_probe
{

/** @brief Return a read-only unsigned slice that borrows its source vector.
 * @param source Live unsigned vector containing the selected bits.
 * @param high Inclusive most significant bit index.
 * @param low Inclusive least significant bit index.
 * @return Read-only native subreference; it becomes invalid with source.
 */
inline sc_dt::sc_uint_subref_r uint_subref(const sc_dt::sc_uint_base& source, int high, int low)
{
  return source.range(high, low);
}

/** @brief Return a read-only signed slice that borrows its source vector.
 * @param source Live signed vector containing the selected bits.
 * @param high Inclusive most significant bit index.
 * @param low Inclusive least significant bit index.
 * @return Read-only native subreference; it becomes invalid with source.
 */
inline sc_dt::sc_int_subref_r int_subref(const sc_dt::sc_int_base& source, int high, int low)
{
  return source.range(high, low);
}

/** @brief Make a borrowed SystemC concatenation of two live native values.
 * @param left Live left operand retained by the caller.
 * @param right Live right operand retained by the caller.
 * @return Reference into SystemC's concat pool; consume before another concat.
 */
inline const sc_dt::sc_concatref& concat(const sc_dt::sc_value_base& left, const sc_dt::sc_value_base& right)
{
  return (left, right);
}

}  // namespace sc_dt_vector_algorithms_probe

#endif  // PYSYSTEMC_TEST_SC_DT_VECTOR_ALGORITHMS_PROBE_H
