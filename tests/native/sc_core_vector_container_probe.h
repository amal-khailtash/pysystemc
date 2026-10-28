/** @file sc_core_vector_container_probe.h
 *  @brief Native fixtures for auditing sc_vector and sc_vector_base.
 *  @details The vectors own their elements and names. Port bindings borrow
 *  SystemC channels for the lifetime of their containing module. The custom
 *  creator is a synchronous native callable and retains no Python callback,
 *  so it has no Python reference or GIL lifetime requirements.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_VECTOR_CONTAINER_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_VECTOR_CONTAINER_PROBE_H

#include <cstddef>
#include <systemc>
#include <type_traits>
#include <vector>

namespace sc_core_vector_container_probe
{

using IntegerSignalVector = sc_core::sc_vector<sc_core::sc_signal<int>>;

/** @brief Track fixture element destruction without class static data members. */
inline int& destroyed_counter()
{
  static int count = 0;
  return count;
}

/** @brief A vector element with ports, a child signal, and a stable ordinal.
 *  @details SystemC owns this module through its parent vector. Its ports bind
 *  to channels owned by the parent fixture. It registers no callbacks and
 *  accesses no Python or GIL state.
 */
class Element final : public sc_core::sc_module
{
 public:
  sc_core::sc_signal<int> payload;
  int ordinal;

  /** @brief Construct an element using its vector-provided name.
   *  @param name Native SystemC object name retained for the element lifetime.
   */
  explicit Element(const char* name) : Element(name, -1)
  {
  }

  /** @brief Construct a custom-created element with a fixture ordinal.
   *  @param name Native SystemC object name retained for the element lifetime.
   *  @param index Index passed by sc_vector's creator callback.
   */
  Element(const char* name, std::size_t index)
    : sc_module(sc_core::sc_module_name(name))
    , payload(sc_core::sc_gen_unique_name("payload"))
    , ordinal(static_cast<int>(index))
  {
  }

  /** @brief Record native destruction for vector ownership checks. */
  ~Element() override
  {
    ++destroyed_counter();
  }
};

/** @brief Port-bearing vector element used only under a parent module.
 *  @details SystemC owns this module through its element vector. The three
 *  input ports borrow bound signal interfaces from the fixture's sibling
 *  channel vectors. Construction occurs synchronously during elaboration.
 */
class PortElement final : public sc_core::sc_module
{
 public:
  sc_core::sc_in<bool> direct;
  sc_core::sc_in<bool> ranged;
  sc_core::sc_in<bool> assembled;

  /** @brief Construct a named port bundle under the fixture hierarchy.
   *  @param name Native SystemC element name owned by the vector.
   */
  explicit PortElement(const char* name)
    : sc_module(sc_core::sc_module_name(name))
    , direct(sc_core::sc_gen_unique_name("direct"))
    , ranged(sc_core::sc_gen_unique_name("ranged"))
    , assembled(sc_core::sc_gen_unique_name("assembled"))
  {
  }
};

/** @brief Allocate a default port-bearing module for assembly tests. */
struct PortElementCreator
{
  /** @brief Create one module with the vector-generated name.
   *  @param name Native element name.
   *  @param index Zero-based index, unused by this creator.
   *  @return Newly allocated module owned by sc_vector.
   */
  PortElement* operator()(const char* name, std::size_t index) const
  {
    (void)index;
    return new PortElement(name);
  }
};

/** @brief A named module containing one signal for assembly-source binding.
 *  @details The vector owns this module and signal; input ports in its parent
 *  fixture borrow the signal interface. The module is constructed under the
 *  fixture during elaboration.
 */
class ChannelElement final : public sc_core::sc_module
{
 public:
  sc_core::sc_signal<bool> channel;

  /** @brief Construct one channel element under its vector hierarchy.
   *  @param name Native SystemC element name.
   */
  explicit ChannelElement(const char* name)
    : sc_module(sc_core::sc_module_name(name)), channel(sc_core::sc_gen_unique_name("channel"))
  {
  }
};

/** @brief Allocate one named channel element for an sc_vector. */
struct ChannelElementCreator
{
  /** @brief Create a channel module; the vector takes ownership.
   *  @param name Generated vector element name.
   *  @param index Zero-based index, unused by this creator.
   *  @return Newly allocated channel module.
   */
  ChannelElement* operator()(const char* name, std::size_t index) const
  {
    (void)index;
    return new ChannelElement(name);
  }
};

/** @brief sc_vector creator that records the requested element index.
 *  @details Calls are synchronous during vector construction or init. The
 *  returned element is owned by sc_vector; the creator stores no pointer.
 */
struct ElementCreator
{
  /** @brief Allocate one custom Element using SystemC's supplied name.
   *  @param name Generated vector element name.
   *  @param index Zero-based vector index.
   *  @return Newly allocated element; ownership transfers to sc_vector.
   */
  Element* operator()(const char* name, std::size_t index) const
  {
    return new Element(name, index);
  }
};

/** @brief Creator for an arbitrary-width sc_signal vector specialization.
 *  @details The creator returns each signal to its owning vector and retains
 *  no external resources or Python callback state.
 */
struct IntegerSignalCreator
{
  /** @brief Allocate and initialize one integer-valued signal.
   *  @param name Generated vector element name.
   *  @param index Zero-based vector index used for the initial value.
   *  @return Newly allocated signal; ownership transfers to sc_vector.
   */
  sc_core::sc_signal<int>* operator()(const char* name, std::size_t index) const
  {
    return new sc_core::sc_signal<int>(name, static_cast<int>(index * 7));
  }
};

using DirectMember = sc_core::sc_in<bool> PortElement::*;
using RangedMember = sc_core::sc_in<bool> PortElement::*;
using AssembledMember = sc_core::sc_in<bool> PortElement::*;

/** @brief Return the direct-port member pointer used by sc_vector::assemble. */
inline DirectMember direct_member()
{
  return &PortElement::direct;
}

/** @brief Return the range-port member pointer used by sc_vector::assemble. */
inline RangedMember ranged_member()
{
  return &PortElement::ranged;
}

/** @brief Return the assembled-port member pointer used by sc_vector::assemble. */
inline AssembledMember assembled_member()
{
  return &PortElement::assembled;
}

/** @brief Delete a standalone element returned by sc_vector::create_element.
 *  @param element Newly allocated element whose ownership has not transferred
 *  to a vector.
 */
inline void destroy_element(Element* element)
{
  delete element;
}

/** @brief Read element ordinals without changing vector ownership. */
template <typename T>
int ordinal(const sc_core::sc_vector<T>& elements, unsigned index)
{
  return elements.at(index).ordinal;
}

/** @brief Read one signal through the const sc_vector subscript overload. */
template <typename T>
int const_subscript_value(const sc_core::sc_vector<T>& values, unsigned index)
{
  return values[index].read();
}

/** @brief Read one signal through the const sc_vector::at overload. */
template <typename T>
int const_at_value(const sc_core::sc_vector<T>& values, unsigned index)
{
  return values.at(index).read();
}

/** @brief Return a const vector reference for Python overload resolution.
 *  @param values Existing vector whose lifetime exceeds the returned proxy.
 *  @return Borrowed const reference; ownership remains with the caller.
 */
template <typename T>
const sc_core::sc_vector<T>& const_vector(const sc_core::sc_vector<T>& values)
{
  return values;
}

/** @brief Assert the public sc_vector_base typedef relationships at compile time. */
inline bool base_aliases_match()
{
  static_assert(std::is_same<sc_core::sc_vector_base::size_type, std::size_t>::value,
                "sc_vector_base::size_type must match vector storage");
  static_assert(std::is_same<sc_core::sc_vector_base::difference_type, std::ptrdiff_t>::value,
                "sc_vector_base::difference_type must match vector storage");
  static_assert(
    std::is_same<sc_core::sc_vector_base::storage_type, std::vector<sc_core::sc_vector_base::handle_type>>::value,
    "sc_vector_base::storage_type must contain vector handles");
  return true;
}

/** @brief Bind three independent element-port vectors to signal vectors.
 *  @details This native fixture is an independent reference for Python's
 *  range, container, and assembly binding overloads. Ports and signals are
 *  owned by this module and its element vectors; no Python callbacks are used.
 */
class BindFixture final : public sc_core::sc_module
{
 public:
  sc_core::sc_vector<sc_core::sc_in<bool>> direct_ports;
  sc_core::sc_vector<sc_core::sc_in<bool>> ranged_ports;
  sc_core::sc_vector<sc_core::sc_in<bool>> continued_ports;
  sc_core::sc_vector<sc_core::sc_in<bool>> call_ports;
  sc_core::sc_vector<sc_core::sc_in<bool>> call_ranged_ports;
  sc_core::sc_vector<sc_core::sc_in<bool>> call_continued_ports;
  sc_core::sc_vector<sc_core::sc_in<bool>> assembled_ports;
  sc_core::sc_vector<sc_core::sc_in<bool>> assembly_bind_ports;
  sc_core::sc_vector<sc_core::sc_in<bool>> assembly_call_ports_from_channel;
  sc_core::sc_vector<sc_core::sc_signal<bool>> direct_signals;
  sc_core::sc_vector<sc_core::sc_signal<bool>> ranged_signals;
  sc_core::sc_vector<sc_core::sc_signal<bool>> continued_signals;
  sc_core::sc_vector<sc_core::sc_signal<bool>> call_signals;
  sc_core::sc_vector<sc_core::sc_signal<bool>> call_ranged_signals;
  sc_core::sc_vector<sc_core::sc_signal<bool>> call_continued_signals;
  sc_core::sc_vector<sc_core::sc_signal<bool>> assembled_signals;
  sc_core::sc_vector<sc_core::sc_signal<bool>> assembly_ranged_signals;
  sc_core::sc_vector<sc_core::sc_in<bool>> assembly_call_ports;
  sc_core::sc_vector<sc_core::sc_signal<bool>> assembly_call_signals;
  sc_core::sc_vector<PortElement> elements;
  sc_core::sc_vector<ChannelElement> channel_elements;

  /** @brief Construct and bind the three ports through native API overloads.
   *  @param name Unique SystemC module name.
   */
  explicit BindFixture(sc_core::sc_module_name name)
    : sc_module(name)
    , direct_ports("direct_ports", 3)
    , ranged_ports("ranged_ports", 3)
    , continued_ports("continued_ports", 3)
    , call_ports("call_ports", 3)
    , call_ranged_ports("call_ranged_ports", 3)
    , call_continued_ports("call_continued_ports", 3)
    , assembled_ports("assembled_ports", 3)
    , assembly_bind_ports("assembly_bind_ports", 3)
    , assembly_call_ports_from_channel("assembly_call_ports_from_channel", 3)
    , direct_signals("direct_signals", 3)
    , ranged_signals("ranged_signals", 3)
    , continued_signals("continued_signals", 3)
    , call_signals("call_signals", 3)
    , call_ranged_signals("call_ranged_signals", 3)
    , call_continued_signals("call_continued_signals", 3)
    , assembled_signals("assembled_signals", 3)
    , assembly_ranged_signals("assembly_ranged_signals", 3)
    , assembly_call_ports("assembly_call_ports", 3)
    , assembly_call_signals("assembly_call_signals", 3)
    , elements("elements", 3, PortElementCreator{})
    , channel_elements("channel_elements", 3, ChannelElementCreator{})
  {
  }

  /** @brief Bind vectors through container, iterator, and assembly overloads.
   *  @details Every call runs synchronously during elaboration and retains no
   *  callback state.
   */
  void bind_all()
  {
    direct_ports.bind(direct_signals);
    ranged_ports.bind(ranged_signals.begin(), ranged_signals.end());
    continued_ports.bind(continued_signals.begin(), continued_signals.end(), continued_ports.begin());
    call_ports(call_signals);
    call_ranged_ports(call_ranged_signals.begin(), call_ranged_signals.end());
    call_continued_ports(call_continued_signals.begin(), call_continued_signals.end(), call_continued_ports.begin());
    elements.assemble(&PortElement::direct).bind(assembled_ports);
    assembled_ports.bind(assembled_signals);
    elements.assemble(&PortElement::ranged).bind(assembly_ranged_signals.begin(), assembly_ranged_signals.end());
    elements.assemble (&PortElement::assembled)(assembly_call_ports);
    assembly_call_ports.bind(assembly_call_signals);
    assembly_bind_ports.bind(channel_elements.assemble(&ChannelElement::channel));
    assembly_call_ports_from_channel(channel_elements.assemble(&ChannelElement::channel));

    for (unsigned index = 0; index < 3; ++index)
    {
      direct_signals[index].write(index % 2 == 1);
      ranged_signals[index].write(index == 2);
      continued_signals[index].write(index == 1);
      call_signals[index].write(index != 2);
      call_ranged_signals[index].write(index == 0);
      call_continued_signals[index].write(index == 2);
      assembled_signals[index].write(index != 1);
      assembly_ranged_signals[index].write(index == 1);
      assembly_call_signals[index].write(index == 0);
      channel_elements[index].channel.write(index != 1);
    }
  }
};

/** @brief Return the fixture's three port values after channel updates. */
inline int bound_values(const BindFixture& fixture, unsigned index)
{
  return static_cast<int>(fixture.elements[index].direct.read()) * 100 +
         static_cast<int>(fixture.elements[index].ranged.read()) * 10 +
         static_cast<int>(fixture.elements[index].assembled.read());
}

/** @brief Read a three-bit vector of input ports after signal updates. */
inline int port_bits(const sc_core::sc_vector<sc_core::sc_in<bool>>& ports)
{
  int result = 0;
  for (unsigned index = 0; index < ports.size(); ++index) result = result * 2 + static_cast<int>(ports[index].read());
  return result;
}

/** @brief Read the fixture's eleven independent binding paths as stable bit strings. */
inline std::vector<int> binding_values(const BindFixture& fixture)
{
  return {
    port_bits(fixture.direct_ports),
    port_bits(fixture.ranged_ports),
    port_bits(fixture.continued_ports),
    port_bits(fixture.call_ports),
    port_bits(fixture.call_ranged_ports),
    port_bits(fixture.call_continued_ports),
    port_bits(fixture.assembly_bind_ports),
    port_bits(fixture.assembly_call_ports_from_channel),
    static_cast<int>(fixture.elements[0].direct.read()) * 4 + static_cast<int>(fixture.elements[1].direct.read()) * 2 +
      static_cast<int>(fixture.elements[2].direct.read()),
    static_cast<int>(fixture.elements[0].ranged.read()) * 4 + static_cast<int>(fixture.elements[1].ranged.read()) * 2 +
      static_cast<int>(fixture.elements[2].ranged.read()),
    static_cast<int>(fixture.elements[0].assembled.read()) * 4 +
      static_cast<int>(fixture.elements[1].assembled.read()) * 2 +
      static_cast<int>(fixture.elements[2].assembled.read())};
}

}  // namespace sc_core_vector_container_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_VECTOR_CONTAINER_PROBE_H
