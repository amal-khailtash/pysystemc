/** @file sc_core_attribute_families.cpp
 * @brief Native attribute family reference; collections borrow attribute pointers.
 */
#include <systemc>
#include <type_traits>
#include <vector>
/** @brief Check default/value/copy constructors for arbitrary native-valid T. */
template <class T>
void typed(const T& value)
{
  sc_core::sc_attribute<T> empty("default"), attribute("value", value), copy(attribute);
  if (copy.value != value || copy.name() != "value") throw std::runtime_error("typed copy");
  attribute.value = value;
  if constexpr (std::is_same_v<T, int>)
  {
    attribute.value = 23;
    if (attribute.value != 23 || copy.value != 17) throw std::runtime_error("value copy independence");
  }
}
/** @brief Check base/collection copy, iterators and removal without deleting attributes. */
int sc_main(int, char**)
{
  using namespace sc_core;
  static_assert(std::is_same_v<sc_attr_cltn::elem_type, sc_attr_base*>);
  static_assert(std::is_same_v<sc_attr_cltn::iterator, std::vector<sc_attr_base*>::iterator>);
  static_assert(std::is_same_v<sc_attr_cltn::const_iterator, std::vector<sc_attr_base*>::const_iterator>);
  sc_attr_base base("base"), base_copy(base);
  if (base_copy.name() != "base") return 2;
  typed<int>(17);
  typed<std::string>("text");
  typed<sc_dt::sc_biguint<137>>(29);
  sc_attribute<int> a("a", 0), b("b", 1), c("c", 2);
  sc_attr_cltn collection;
  if (collection.push_back(nullptr) || !collection.push_back(&a) || !collection.push_back(&b) ||
      !collection.push_back(&c) || collection.push_back(&a) || collection["missing"])
    return 3;
  {
    sc_attr_cltn copy(collection);
    const sc_attr_cltn& view = copy;
    if (view["a"] != &a) return 4;
    std::vector<std::string> names;
    for (auto i = view.begin(); i != view.end(); ++i) names.push_back((*i)->name());
    if (names != std::vector<std::string>{"a", "b", "c"} || collection.remove("a") != &a ||
        collection.remove("missing") || copy.size() != 3 || collection.size() != 2)
      return 5;
    collection.remove_all();
    if (collection.size() != 0) return 6;
  }
  if (a.value != 0 || b.value != 1 || c.value != 2) return 7;
  std::cout << "ATTRIBUTE_FAMILY_REFERENCE base 17 text 29 a b c 3 2 0 0 1 2\n";
  return 0;
}
