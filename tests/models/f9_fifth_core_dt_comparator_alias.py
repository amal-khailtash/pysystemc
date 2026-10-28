"""Check priority comparator tokens and exact native pointer-value exposure."""

from amal.eda.systemc import sc_core

import cppyy

cppyy.cppdef(
    r"""
namespace fifth_comparator_probe
{
/** @brief Borrow opaque inputs without dereferencing or owning them.
 * @details Synchronous native call; no Python callback or GIL release is used.
 */
int compare(const void*, const void*)
{
  return 23;
}
/** @brief Return a static native comparator token with process lifetime. */
sc_core::sc_ppq_base::compare_fn_t token()
{
  return &compare;
}
/** @brief Invoke a borrowed comparator during the synchronous native call.
 * @details Caller retains the callable; cppyy acquires GIL if adaptation invokes
 * Python. Null payload pointers are valid because this comparator ignores them.
 */
int invoke(sc_core::sc_ppq_base::compare_fn_t comparator)
{
  return comparator(nullptr, nullptr);
}
}  // namespace fifth_comparator_probe
"""
)

assert sc_core.CFT is sc_core.sc_ppq_base.compare_fn_t
assert "CFT" in sc_core.__all__
holder = sc_core.sc_ppq_base.compare_fn_t.from_function("fifth_comparator_probe::compare")
assert holder.valid() and holder == sc_core.sc_ppq_base.compare_fn_t(holder)
assert holder != sc_core.sc_ppq_base.compare_fn_t()

probe = cppyy.gbl.fifth_comparator_probe
comparator = probe.token()
assert probe.invoke(comparator) == 23
assert probe.invoke(probe.compare) == 23
print("COMPARATOR_ALIAS_VALUES 2 TOKEN_RESULTS 23 23")
