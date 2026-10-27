"""Check priority comparator tokens while reporting unavailable typedef exposure."""

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

for native_type in (sc_core.sc_ppq_base, cppyy.gbl.sc_core.sc_ppq_base):
    try:
        getattr(native_type, "compare_fn_t")
    except AttributeError:
        pass
    else:
        raise AssertionError("typedef became directly exposed; update contract disposition")

probe = cppyy.gbl.fifth_comparator_probe
comparator = probe.token()
assert probe.invoke(comparator) == 23
assert probe.invoke(probe.compare) == 23
print("COMPARATOR_ALIAS_PARTIAL 2 TOKEN_RESULTS 23 23")
