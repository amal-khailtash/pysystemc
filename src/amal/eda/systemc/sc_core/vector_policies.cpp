/** @file vector_policies.cpp
 * @brief Native access-policy construction avoiding member-pointer conversion.
 * @details Policies borrow element/member pointers and never own elements. GIL
 * stays held for construction and access. Native const-conversion restrictions
 * and copy semantics apply; no callbacks or scheduler entry occur.
 */
#include <systemc>

namespace pysystemc_vectors
{
/** @brief Construct access policy copies with their exact native const rules. */
template <class Element, class Member>
struct member_policy
{
  using policy = sc_core::sc_member_access<Element, Member>;
  using non_const_policy = typename policy::non_const_policy;
  /** @brief Copy an identical native policy without pointer-to-member conversion. */
  static policy copy(const policy& source)
  {
    return source;
  }
  /** @brief Convert a mutable native policy using its declared constructor. */
  static policy convert(const non_const_policy& source)
  {
    return policy(source);
  }
};

/** @brief Invoke the native templated direct-policy conversion constructor. */
template <class Element>
struct direct_policy
{
  /** @brief Convert a borrowed policy while preserving native SFINAE const restrictions. */
  template <class Other>
  static sc_core::sc_direct_access<Element> convert(const Other& source)
  {
    return sc_core::sc_direct_access<Element>(source);
  }
};
/** @brief Expose iterator arrow dereference without cppyy attribute forwarding. */
template <class Iterator>
struct iterator_pointer
{
  /** @brief Return a borrowed native element/member pointer from operator->.
   * @details The iterator and vector/assembly owners must remain alive. GIL held;
   * no bounds checks are added, so end iterators must never be dereferenced.
   */
  static auto get(const Iterator& iterator)
  {
    return iterator.operator->();
  }
};
}  // namespace pysystemc_vectors
