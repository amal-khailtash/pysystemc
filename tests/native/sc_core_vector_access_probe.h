/** @file sc_core_vector_access_probe.h
 * @brief Arbitrary element and native observations for vector access tests.
 * @details sc_vector owns every Element it creates. Iterator and policy objects
 * borrow that vector and its elements; these helpers keep no Python references,
 * callbacks or pointers beyond each synchronous call. They run on the caller's
 * thread, do not acquire the GIL, and do not throw.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_VECTOR_ACCESS_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_VECTOR_ACCESS_PROBE_H

#include <sysc/kernel/sc_object.h>
#include <sysc/utils/sc_vector.h>

#include <iterator>
#include <type_traits>
#include <utility>

namespace sc_core_vector_access_probe
{

/** @brief Native element with public storage for direct and member policies. */
class Element final : public sc_core::sc_object
{
 public:
  int value;    /**< Integer field selected by the member access policy. */
  int identity; /**< Stable value used to distinguish borrowed elements. */

  /** @brief Construct a SystemC object with zeroed test fields.
   * @param name Object name owned by the containing sc_vector.
   */
  explicit Element(const char* name) : sc_object(name), value(0), identity(0)
  {
  }

  /** @brief Count vector-owned element destruction for the lifecycle check. */
  ~Element() override
  {
    ++destroyed_count();
  }

  /** @brief Return the process-local destruction count. */
  static int destroyed()
  {
    return destroyed_count();
  }

  /** @brief Reset the process-local count before a fresh vector is created. */
  static void reset_destroyed()
  {
    destroyed_count() = 0;
  }

 private:
  /** @brief Hold the process-local element destruction counter. */
  static int& destroyed_count()
  {
    static int count = 0;
    return count;
  }
};

using Direct = sc_core::sc_direct_access<Element>;
using ConstDirect = sc_core::sc_direct_access<const Element>;
using Member = sc_core::sc_member_access<Element, int>;
using ConstMember = sc_core::sc_member_access<const Element, const int>;
using DirectIterator = sc_core::sc_vector_iter<Element>;
using ConstDirectIterator = sc_core::sc_vector_iter<const Element>;
using MemberIterator = sc_core::sc_vector_iter<Element, Member>;
using ConstMemberIterator = sc_core::sc_vector_iter<const Element, ConstMember>;

static_assert(std::is_same<Direct::element_type, Element>::value, "direct element_type");
static_assert(std::is_same<Direct::type, Element>::value, "direct type");
static_assert(std::is_same<Direct::plain_type, Element>::value, "direct plain_type");
static_assert(std::is_same<Direct::policy, Direct>::value, "direct policy");
static_assert(std::is_same<Direct::non_const_policy, Direct>::value, "direct non_const_policy");
static_assert(std::is_same<Direct::const_policy, ConstDirect>::value, "direct const_policy");
static_assert(std::is_same<ConstDirect::plain_type, Element>::value, "const direct plain_type");
static_assert(std::is_same<Member::element_type, Element>::value, "member element_type");
static_assert(std::is_same<Member::access_type, int>::value, "member access_type");
static_assert(std::is_same<Member::member_type, int(Element::*)>::value, "member member_type");
static_assert(std::is_same<Member::type, int>::value, "member type");
static_assert(std::is_same<Member::plain_type, int>::value, "member plain_type");
static_assert(std::is_same<Member::plain_elem_type, Element>::value, "member plain_elem_type");
static_assert(std::is_same<Member::policy, Member>::value, "member policy");
static_assert(std::is_same<Member::non_const_policy, Member>::value, "member non_const_policy");
static_assert(std::is_same<Member::const_policy, ConstMember>::value, "member const_policy");
static_assert(std::is_same<ConstMember::plain_type, int>::value, "const member plain_type");
static_assert(std::is_same<ConstMember::plain_elem_type, Element>::value, "const member plain_elem_type");
static_assert(std::is_same<DirectIterator::iterator_category, std::random_access_iterator_tag>::value,
              "iterator category");
static_assert(std::is_same<DirectIterator::difference_type, std::ptrdiff_t>::value, "iterator difference_type");
static_assert(std::is_same<DirectIterator::value_type, Element>::value, "direct iterator value_type");
static_assert(std::is_same<DirectIterator::reference, Element&>::value, "direct iterator reference");
static_assert(std::is_same<DirectIterator::pointer, Element*>::value, "direct iterator pointer");
static_assert(std::is_same<MemberIterator::value_type, int>::value, "member iterator value_type");
static_assert(std::is_same<MemberIterator::reference, int&>::value, "member iterator reference");
static_assert(std::is_same<MemberIterator::pointer, int*>::value, "member iterator pointer");
static_assert(std::is_same<decltype(std::declval<sc_core::sc_vector<Element>&>().begin()), DirectIterator>::value,
              "mutable vector begin");
static_assert(
  std::is_same<decltype(std::declval<const sc_core::sc_vector<Element>&>().cbegin()), ConstDirectIterator>::value,
  "const vector cbegin");

/** @brief Supply the native pointer-to-member needed to construct Member.
 * @return Pointer into an Element; valid while that Element remains alive.
 */
inline int Element::*value_member()
{
  return &Element::value;
}

/** @brief Compare borrowed element addresses without transferring ownership. */
inline bool same_element(const Element* actual, const Element* expected)
{
  return actual == expected;
}

/** @brief Compare a borrowed member address with its owning element field. */
inline bool same_member(const int* actual, const Element& expected)
{
  return actual == &expected.value;
}

/** @brief Read a borrowed integer member returned by a public policy. */
inline int read_member(const int* value)
{
  return *value;
}

/** @brief Return the number of vector-owned elements destroyed so far. */
inline int destroyed_elements()
{
  return Element::destroyed();
}

/** @brief Reset the native lifecycle counter for an independent test vector. */
inline void reset_destroyed_elements()
{
  Element::reset_destroyed();
}

}  // namespace sc_core_vector_access_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_VECTOR_ACCESS_PROBE_H
