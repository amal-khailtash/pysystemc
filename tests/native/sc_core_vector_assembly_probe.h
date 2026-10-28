/** @file sc_core_vector_assembly_probe.h
 * @brief Native fixtures and narrow adapters for public sc_vector assemblies.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_VECTOR_ASSEMBLY_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_VECTOR_ASSEMBLY_PROBE_H

#include <cstddef>
#include <string>
#include <systemc>
#include <type_traits>

namespace sc_core_vector_assembly_probe
{

/** @brief Module with two member ports and a member channel for assembly tests.
 * @details The vector owns each module and its ports/channel. Its bound
 * interfaces remain borrowed from caller-owned vectors. The registered method
 * callback is native and does not use Python or the GIL.
 */
class Element final : public sc_core::sc_module
{
 public:
  inline static int destroyed = 0;

  sc_core::sc_in<bool> input;
  sc_core::sc_in<bool> spare;
  sc_core::sc_signal<bool> observed;
  int samples = 0;
  bool last = false;

  SC_HAS_PROCESS(Element);

  /** @brief Construct one vector-owned module and register its sampler.
   * @param name Native child-module name provided by sc_vector.
   * @details sc_module consumes the temporary name in the constructor; vector
   * ownership controls module lifetime and the sensitivity port is bound later.
   */
  explicit Element(sc_core::sc_module_name name) : sc_module(name), input("input"), spare("spare"), observed("observed")
  {
    SC_METHOD(sample);
    sensitive << input;
  }

  /** @brief Count destruction for vector ownership checks. */
  ~Element() override
  {
    ++destroyed;
  }

  /** @brief Sample the bound interface and publish the value on the member channel. */
  void sample()
  {
    last = input.read();
    ++samples;
    observed.write(last);
  }
};

/** @brief sc_object owner whose public signal member forms an assembly source. */
class ChannelOwner final : public sc_core::sc_object
{
 public:
  sc_core::sc_signal<bool> value;

  /** @brief Construct an owned channel member with a child name.
   * @param name Name assigned by the owning sc_vector.
   */
  explicit ChannelOwner(const char* name) : sc_object(name), value((std::string(name) + "_value").c_str())
  {
  }
};

using ElementVector = sc_core::sc_vector<Element>;
using SignalVector = sc_core::sc_vector<sc_core::sc_signal<bool>>;
using ChannelVector = sc_core::sc_vector<ChannelOwner>;
using InputAssembly = sc_core::sc_vector_assembly<Element, sc_core::sc_in<bool>>;
using ObservedAssembly = sc_core::sc_vector_assembly<Element, sc_core::sc_signal<bool>>;
using SpareAssembly = sc_core::sc_vector_assembly<Element, sc_core::sc_in<bool>>;
using ChannelAssembly = sc_core::sc_vector_assembly<ChannelOwner, sc_core::sc_signal<bool>>;

/** @brief Assemble the input member using the native public free function.
 * @param elements Borrowed vector which must outlive the returned assembly.
 * @return Assembly proxy borrowing the vector and its input member pointer.
 */
inline InputAssembly input_assembly(ElementVector& elements)
{
  return sc_core::sc_assemble_vector(elements, &Element::input);
}

/** @brief Assemble the spare port member using the native public vector method.
 * @param elements Borrowed vector which must outlive the returned assembly.
 * @return Assembly proxy borrowing the vector and its spare member pointer.
 */
inline SpareAssembly spare_assembly(ElementVector& elements)
{
  return elements.assemble(&Element::spare);
}

/** @brief Assemble the observed channel member using the native vector method.
 * @param elements Borrowed vector which must outlive the returned assembly.
 * @return Assembly proxy borrowing the vector and its channel member pointer.
 */
inline ObservedAssembly observed_assembly(ElementVector& elements)
{
  return elements.assemble(&Element::observed);
}

/** @brief Assemble signal members used as source interfaces for bind overloads.
 * @param channels Borrowed vector which must outlive the returned assembly.
 * @return Assembly proxy borrowing the vector and each member signal.
 */
inline ChannelAssembly channel_assembly(ChannelVector& channels)
{
  return sc_core::sc_assemble_vector(channels, &ChannelOwner::value);
}

/** @brief Preserve const qualification so cppyy can exercise const overloads.
 * @param assembly Assembly borrowed for this call.
 * @return Const reference valid while the caller-owned assembly remains alive.
 */
inline const InputAssembly& const_input_assembly(const InputAssembly& assembly)
{
  return assembly;
}

/** @brief Return whether all public assembly aliases retain their exact native types. */
inline bool aliases_match_native_types()
{
  static_assert(std::is_same<InputAssembly::base_type, ElementVector>::value, "base_type alias");
  static_assert(std::is_same<InputAssembly::element_type, Element>::value, "element_type alias");
  static_assert(std::is_same<InputAssembly::access_type, sc_core::sc_in<bool>>::value, "access_type alias");
  static_assert(std::is_same<InputAssembly::member_type, sc_core::sc_in<bool> Element::*>::value, "member_type alias");
  static_assert(
    std::is_same<InputAssembly::iterator,
                 sc_core::sc_vector_iter<Element, sc_core::sc_member_access<Element, sc_core::sc_in<bool>>>>::value,
    "iterator alias");
  static_assert(
    std::is_same<InputAssembly::const_iterator,
                 sc_core::sc_vector_iter<const Element,
                                         sc_core::sc_member_access<const Element, const sc_core::sc_in<bool>>>>::value,
    "const_iterator alias");
  static_assert(std::is_same<InputAssembly::reference, sc_core::sc_in<bool>&>::value, "reference alias");
  static_assert(std::is_same<InputAssembly::const_reference, const sc_core::sc_in<bool>&>::value,
                "const_reference alias");
  static_assert(std::is_same<InputAssembly::pointer, sc_core::sc_in<bool>*>::value, "pointer alias");
  static_assert(std::is_same<InputAssembly::const_pointer, const sc_core::sc_in<bool>*>::value, "const_pointer alias");
  static_assert(std::is_same<InputAssembly::size_type, ElementVector::size_type>::value, "size_type alias");
  static_assert(std::is_same<InputAssembly::difference_type, ElementVector::difference_type>::value,
                "difference_type alias");
  return true;
}

/** @brief Compare an assembled port reference with a module member address. */
inline bool input_identity(const InputAssembly& assembly, std::size_t index, const ElementVector& elements)
{
  return &assembly[index] == &elements[index].input;
}

/** @brief Compare an assembled spare-port reference with its module member. */
inline bool spare_identity(const SpareAssembly& assembly, std::size_t index, const ElementVector& elements)
{
  return &assembly[index] == &elements[index].spare;
}

/** @brief Compare an assembled channel reference with a module member address. */
inline bool observed_identity(const ObservedAssembly& assembly, std::size_t index, const ElementVector& elements)
{
  return &assembly[index] == &elements[index].observed;
}

/** @brief Verify that get_elements exposes the matching native object pointers. */
inline bool input_elements_identity(const InputAssembly& assembly, const ElementVector& elements)
{
  const auto& objects = assembly.get_elements();
  if (objects.size() != assembly.size()) return false;
  for (std::size_t index = 0; index < objects.size(); ++index)
    if (objects[index] != &elements[index].input) return false;
  return true;
}

/** @brief Verify channel assembly object pointers remain the original members. */
inline bool channel_elements_identity(const ChannelAssembly& assembly, const ChannelVector& channels)
{
  const auto& objects = assembly.get_elements();
  if (objects.size() != assembly.size()) return false;
  for (std::size_t index = 0; index < objects.size(); ++index)
    if (objects[index] != &channels[index].value) return false;
  return true;
}

/** @brief Return element sampler fields through a borrowed vector. */
inline int sample_count(const ElementVector& elements, std::size_t index)
{
  return elements[index].samples;
}

/** @brief Return the last sampled value through a borrowed vector. */
inline bool last_sample(const ElementVector& elements, std::size_t index)
{
  return elements[index].last;
}

/** @brief Return the address of one channel assembly element for identity checks. */
inline const void* channel_address(const ChannelAssembly& assembly, std::size_t index)
{
  return &assembly[index];
}

}  // namespace sc_core_vector_assembly_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_VECTOR_ASSEMBLY_PROBE_H
