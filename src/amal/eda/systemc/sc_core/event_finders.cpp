/** @file event_finders.cpp
 * @brief Instantiate native event finders without cppyy member-pointer conversion.
 */
#include <systemc>
namespace pysystemc_public
{
/** Own a native finder pointer slot for cached_create's reference-to-pointer argument.
 * Returned finder/event/port references borrow their native owners. Destroy the
 * cache after consumers, with the GIL held on the kernel thread. No callbacks retained.
 */
class event_finder_cache
{
 public:
  sc_core::sc_event_finder* pointer = nullptr;
  event_finder_cache() = default;
  event_finder_cache(const event_finder_cache&) = delete;
  event_finder_cache& operator=(const event_finder_cache&) = delete;
  /** Delete the cached finder, leaving its borrowed port/interface unchanged. */
  ~event_finder_cache()
  {
    delete pointer;
  }
};
/** Allocate an owning finder for an arbitrary interface/event member.
 * @param port Borrowed port; caller must retain it and its interface beyond finder lifetime.
 * @tparam IF Native interface type, instantiated on demand.
 * @tparam DeclaringIF Native class declaring the selected event method.
 * @tparam Method Exact native const event member pointer, selected at compile time.
 * @return New native finder; Python adopts ownership, no callback references retained.
 * @details GIL held, kernel thread only. Native construction/report exceptions propagate.
 */
template <class IF, class DeclaringIF, const sc_core::sc_event& (DeclaringIF::*Method)() const>
sc_core::sc_event_finder_t<IF>* make_event_finder(const sc_core::sc_port_base& port)
{
  return new sc_core::sc_event_finder_t<IF>(port, Method);
}

/** Forward exact native cached allocation/reuse semantics for an owning pointer slot.
 * Arguments are borrowed; cache owns the finder. Same-port cache reuse is required
 * by the native assertion contract. GIL held; exceptions propagate, no callbacks retained.
 */
template <class IF, class DeclaringIF, const sc_core::sc_event& (DeclaringIF::*Method)() const>
sc_core::sc_event_finder& cached_event_finder(event_finder_cache& cache, const sc_core::sc_port_base& port)
{
  return sc_core::sc_event_finder::cached_create<IF>(cache.pointer, port, Method);
}
}  // namespace pysystemc_public
