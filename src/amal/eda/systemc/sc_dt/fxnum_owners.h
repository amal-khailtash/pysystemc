/** @file Owned public facades for protected native fixed-point base construction. */
#pragma once
#include <systemc>
namespace pysystemc_datatypes
{
#define PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, Type)                        \
  /** @brief Copy a native source into configured owned numeric storage.                 \
   * @details The observer remains borrowed through destruction. Hold the GIL for Python \
   * entry; native precision, conversion, allocation and report rules apply.             \
   * No Python callbacks or source references are retained. */                           \
  Owner(Type source, const sc_dt::sc_fxtype_params& params, sc_dt::sc_enc encoding,      \
        const sc_dt::sc_fxcast_switch& cast, Observer* observer = nullptr)               \
    : Base(source, params, encoding, cast, observer)                                     \
  {                                                                                      \
  }
#define PYSYSTEMC_FXNUM_OWNER_SOURCES(Owner, Base, Observer)                       \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, int)                         \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, unsigned int)                \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, long)                        \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, unsigned long)               \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, float)                       \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, double)                      \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, const char*)                 \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, const sc_dt::sc_fxval&)      \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, const sc_dt::sc_fxval_fast&) \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, const sc_dt::sc_fxnum&)      \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, const sc_dt::sc_fxnum_fast&) \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, sc_dt::int64)                \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, sc_dt::uint64)               \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, const sc_dt::sc_int_base&)   \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, const sc_dt::sc_uint_base&)  \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, const sc_dt::sc_signed&)     \
  PYSYSTEMC_FXNUM_OWNER_SOURCE(Owner, Base, Observer, const sc_dt::sc_unsigned&)
/** @brief Own native arbitrary-precision fixed-point base numeric storage.
 * @details Construction forwards directly to protected native constructors.
 * The public destructor owns the complete derived object, never a deleted base
 * pointer. Observers remain borrowed and must outlive the number. Calls from
 * Python hold the GIL; native reports and callback build settings apply. Native
 * private default/copy construction stays disabled. No Python state is retained.
 */
class FxnumOwner : public sc_dt::sc_fxnum
{
 public:
  using sc_dt::sc_fxnum::operator=;
  /** @brief Create configured zero storage borrowing a live optional observer. */
  FxnumOwner(const sc_dt::sc_fxtype_params& params, sc_dt::sc_enc encoding, const sc_dt::sc_fxcast_switch& cast,
             sc_dt::sc_fxnum_observer* observer = nullptr)
    : sc_dt::sc_fxnum(params, encoding, cast, observer)
  {
  }
  PYSYSTEMC_FXNUM_OWNER_SOURCES(FxnumOwner, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer)
  /** @brief Release owned storage while the borrowed observer remains live. */
  ~FxnumOwner() = default;
  FxnumOwner() = delete;
  FxnumOwner(const FxnumOwner&) = delete;
  /** @brief Return the borrowed native observer without acquiring ownership. */
  sc_dt::sc_fxnum_observer* observer() const
  {
    return sc_dt::sc_fxnum::observer();
  }
};
/** @brief Own native double-precision fixed-point base numeric storage.
 * @details Direct construction preserves native fast precision and observer
 * identity. Keep observers live through destruction, hold the GIL for Python
 * calls and retain native report policies. Default/copy construction is disabled.
 * No Python callbacks or source references are retained by this facade.
 */
class FxnumFastOwner : public sc_dt::sc_fxnum_fast
{
 public:
  using sc_dt::sc_fxnum_fast::operator=;
  /** @brief Create configured fast zero storage borrowing an optional observer. */
  FxnumFastOwner(const sc_dt::sc_fxtype_params& params, sc_dt::sc_enc encoding, const sc_dt::sc_fxcast_switch& cast,
                 sc_dt::sc_fxnum_fast_observer* observer = nullptr)
    : sc_dt::sc_fxnum_fast(params, encoding, cast, observer)
  {
  }
  PYSYSTEMC_FXNUM_OWNER_SOURCES(FxnumFastOwner, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer)
  /** @brief Release fast storage while the borrowed observer remains live. */
  ~FxnumFastOwner() = default;
  FxnumFastOwner() = delete;
  FxnumFastOwner(const FxnumFastOwner&) = delete;
  /** @brief Return the borrowed fast observer without acquiring ownership. */
  sc_dt::sc_fxnum_fast_observer* observer() const
  {
    return sc_dt::sc_fxnum_fast::observer();
  }
};
#undef PYSYSTEMC_FXNUM_OWNER_SOURCE
#undef PYSYSTEMC_FXNUM_OWNER_SOURCES
}  // namespace pysystemc_datatypes
