/** @file vector_policy_error_edges.cpp
 * @brief Native reference for policy aliases, conversion and const rules.
 * @details The vector owns its test element; policy values borrow its member
 *          or element. Every access is synchronous, with no callbacks, GIL
 *          operations, or kernel reset.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <type_traits>
#include <vector>

#include "sc_core_vector_access_probe.h"

namespace
{

using Element = sc_core_vector_access_probe::Element;
using Member = sc_core::sc_member_access<Element, int>;
using ConstMember = Member::const_policy;
using NestedElement = std::pair<int, std::vector<int>>;
using NestedMember = sc_core::sc_member_access<NestedElement, std::vector<int>>;
using NestedConstMember = NestedMember::const_policy;
using Direct = sc_core::sc_direct_access<Element>;
using ConstDirect = Direct::const_policy;

static_assert(std::is_same<Member::element_type, Element>::value, "member element alias");
static_assert(std::is_same<Member::type, int>::value, "member type alias");
static_assert(std::is_same<Member::policy, Member>::value, "member policy alias identity");
static_assert(std::is_same<Member::non_const_policy, Member>::value, "member mutable alias identity");
static_assert(std::is_same<Member::const_policy, ConstMember>::value, "member const alias identity");
static_assert(std::is_same<NestedMember::element_type, NestedElement>::value, "nested element type parsing");
static_assert(std::is_same<NestedMember::access_type, std::vector<int>>::value, "nested member type parsing");
static_assert(std::is_same<NestedConstMember::access_type, const std::vector<int>>::value,
              "nested const member conversion");
static_assert(std::is_constructible<ConstMember, const Member&>::value, "mutable-to-const member conversion");
static_assert(!std::is_constructible<Member, const ConstMember&>::value, "const-to-mutable rejection");
static_assert(std::is_constructible<Direct, const Member&>::value, "member-to-direct conversion");
static_assert(std::is_constructible<ConstDirect, const Member&>::value, "member-to-const-direct conversion");

/** @brief Emit one stable policy observation. */
template <typename... Fields>
void emit(const Fields&... fields)
{
  std::cout << "VPOLICY";
  ((std::cout << '\t' << fields), ...);
  std::cout << '\n';
}

/** @brief Require a native semantic check to hold before emitting records. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

}  // namespace

/** @brief Run native policy construction and borrowed-member checks. */
int sc_main(int, char**)
{
  constexpr bool aliases_match =
    std::is_same<Member::policy, Member>::value && std::is_same<Member::non_const_policy, Member>::value &&
    std::is_same<Member::const_policy, ConstMember>::value && std::is_same<Member::type, int>::value;
  emit("aliases", int(aliases_match));

  NestedElement nested_element{7, {11, 23}};
  NestedMember nested_member(&NestedElement::second);
  std::vector<int>* nested_value = nested_member.get(&nested_element);
  const bool nested_address_matches = nested_value == &nested_element.second;
  nested_value->push_back(37);
  std::string nested_values;
  for (std::size_t index = 0; index < nested_value->size(); ++index)
  {
    if (index != 0) nested_values += ',';
    nested_values += std::to_string((*nested_value)[index]);
  }
  emit("nested", int(nested_address_matches), nested_element.first, nested_value->size(), nested_values);

  sc_core::sc_vector<Element> elements("policy_error_elements", 1);
  Element& element = elements[0];
  element.value = 29;

  Member member(&Element::value);
  Member copied(member);
  Member non_const(member);
  ConstMember const_member(member);
  Direct direct(member);
  ConstDirect const_direct(member);

  const bool addresses_match = sc_core_vector_access_probe::same_member(member.get(&element), element) &&
                               sc_core_vector_access_probe::same_member(copied.get(&element), element) &&
                               sc_core_vector_access_probe::same_member(non_const.get(&element), element) &&
                               sc_core_vector_access_probe::same_member(const_member.get(&element), element);
  const bool direct_matches = sc_core_vector_access_probe::same_element(direct.get(&element), &element) &&
                              sc_core_vector_access_probe::same_element(const_direct.get(&element), &element);
  const int before = sc_core_vector_access_probe::read_member(member.get(&element));
  *member.get(&element) = 73;
  const int after = sc_core_vector_access_probe::read_member(const_member.get(&element));
  verify(addresses_match && direct_matches && before == 29 && after == 73, "policy conversion or member mutation");
  emit("live", int(addresses_match), int(direct_matches), before, after);
  return 0;
}
