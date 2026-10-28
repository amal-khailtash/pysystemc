/** @file sc_core_vector_access_public.cpp
 * @brief Native reference for direct, member and random-access vector policies.
 * @details Each sc_vector owns and destroys its elements. Policies and iterators
 * borrow the vector or elements for this synchronous native audit. No Python
 * callbacks or GIL operations occur.
 */
#include <iostream>
#include <stdexcept>
#include <systemc>

#include "sc_core_vector_access_probe.h"

namespace
{

/** @brief Stop the reference when an observed native contract differs. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

}  // namespace

/** @brief Exercise policy constructors, aliases, identity and iterator operators. */
int sc_main(int, char**)
{
  using namespace sc_core;
  using namespace sc_core_vector_access_probe;

  reset_destroyed_elements();
  int direct_identity = 0;
  int member_value = 0;
  int iterator_checks = 0;
  int const_checks = 0;
  {
    sc_vector<Element> elements("native_vector", 3);
    elements[0].value = 11;
    elements[0].identity = 101;
    elements[1].value = 22;
    elements[1].identity = 202;
    elements[2].value = 33;
    elements[2].identity = 303;

    Direct direct;
    Direct direct_copy(direct);
    ConstDirect const_direct(direct);
    verify(same_element(direct.get(&elements[1]), &elements[1]), "direct get identity");
    verify(same_element(direct_copy.get(&elements[1]), &elements[1]), "direct non-const policy constructor");
    verify(same_element(const_direct.get(&elements[1]), &elements[1]), "direct const policy constructor");
    direct_identity = direct.get(&elements[1])->identity;

    Member member(value_member());
    Member member_copy(member);
    ConstMember const_member(member);
    verify(same_member(member.get(&elements[1]), elements[1]), "member get identity");
    verify(same_member(member_copy.get(&elements[1]), elements[1]), "member non-const policy constructor");
    verify(same_member(const_member.get(&elements[1]), elements[1]), "member const policy constructor");
    member_value = *member.get(&elements[1]);

    auto direct_begin = elements.begin();
    auto direct_end = elements.end();
    ConstDirectIterator const_direct_begin(direct_begin);
    verify(const_direct_begin == direct_begin && const_direct_begin[1].identity == 202,
           "direct mutable-to-const iterator conversion");

    auto view = elements.assemble(value_member());
    auto begin = view.begin();
    auto end = view.end();
    auto cursor = begin;
    auto old = cursor++;
    verify(*old == 11 && *cursor == 22, "member iterator post-increment and dereference");
    verify(++cursor == end - 1 && *cursor == 33, "member iterator pre-increment");
    verify(cursor-- == end - 1 && *cursor == 22, "member iterator post-decrement");
    verify(--cursor == begin && *cursor == 11, "member iterator pre-decrement");
    verify(*(begin + 2) == 33 && *(end - 2) == 22, "member iterator plus and minus");
    auto advanced = begin;
    advanced += 2;
    verify(*advanced == 33, "member iterator plus assignment");
    advanced -= 1;
    verify(*advanced == 22, "member iterator minus assignment");
    verify(begin[2] == 33 && *begin.operator->() == 11, "member iterator indexing and arrow");
    verify(begin == begin && begin != end && begin < end && begin <= end && end > begin && end >= begin,
           "member iterator comparisons");
    verify(end - begin == 3, "member iterator distance");
    iterator_checks = 1;

    auto const_view_begin = static_cast<const decltype(view)&>(view).cbegin();
    auto const_view_end = static_cast<const decltype(view)&>(view).cend();
    verify(const_view_begin == begin && const_view_end - const_view_begin == 3 && const_view_begin[1] == 22,
           "member const iterator aliases and conversion");
    const_checks = 1;
  }
  verify(destroyed_elements() == 3, "sc_vector element destruction");

  std::cout << "VECTOR_ACCESS_REFERENCE direct=" << direct_identity << " member=" << member_value
            << " iterator=" << iterator_checks << " const=" << const_checks << " destroyed=" << destroyed_elements()
            << '\n';
  return 0;
}
