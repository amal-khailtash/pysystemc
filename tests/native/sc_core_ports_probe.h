/** @file sc_core_ports_probe.h
 *  @brief Test-only native interfaces and protected sc_port_b adapters.
 *  @details Channels own their native event state. Ports and binding records
 *  borrow channels and parent ports, so tests retain them until child ports
 *  are destroyed. The helpers run synchronously, retain no Python callbacks,
 *  and do not release the GIL.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_PORTS_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_PORTS_PROBE_H

#include <systemc>

namespace sc_core_ports_probe
{

/** @brief Small arbitrary interface for generic port template coverage. */
class ValueIf : public virtual sc_core::sc_interface
{
 public:
  /** @brief Read the channel's stable integer value. */
  virtual int value() const = 0;
  /** @brief Return the channel's borrowed event. */
  virtual const sc_core::sc_event& changed_event() const = 0;
};

/** @brief Named module channel implementing an interface outside builtin families. */
class ValueChannel : public sc_core::sc_module, public ValueIf
{
 public:
  sc_core::sc_event changed;

  /** @brief Construct a channel with the requested stable value. */
  ValueChannel(sc_core::sc_module_name name, int value) : sc_module(name), changed("changed"), value_(value)
  {
  }
  /** @brief Return the channel's value through the custom interface. */
  int value() const override
  {
    return value_;
  }
  /** @brief Return the same borrowed event as sc_interface's default event. */
  const sc_core::sc_event& changed_event() const override
  {
    return changed;
  }
  /** @brief Return the interface default event. */
  const sc_core::sc_event& default_event() const override
  {
    return changed;
  }

 private:
  int value_;
};

/** @brief Hold native channels for generic and inventoried port specializations. */
class PortHost : public sc_core::sc_module
{
 public:
  ValueChannel left;
  ValueChannel right;
  sc_core::sc_signal<bool> bool_signal;
  sc_core::sc_signal<sc_dt::sc_logic> logic_signal;
  sc_core::sc_event_queue event_queue;

  /** @brief Construct source channels under a native module hierarchy. */
  explicit PortHost(sc_core::sc_module_name name)
    : sc_module(name)
    , left("left", 13)
    , right("right", 29)
    , bool_signal("bool_signal")
    , logic_signal("logic_signal")
    , event_queue("event_queue")
  {
  }
};

/** @brief Expose protected sc_port_b constructors for one native interface type. */
template <class IF>
class PortBProbe : public sc_core::sc_port_b<IF>
{
 public:
  /** @brief Construct through the protected native unnamed base constructor. */
  explicit PortBProbe(int max_size, sc_core::sc_port_policy policy = sc_core::SC_ONE_OR_MORE_BOUND)
    : sc_core::sc_port_b<IF>(max_size, policy)
  {
  }
  /** @brief Construct through the protected native named base constructor. */
  PortBProbe(const char* name, int max_size, sc_core::sc_port_policy policy = sc_core::SC_ONE_OR_MORE_BOUND)
    : sc_core::sc_port_b<IF>(name, max_size, policy)
  {
  }
  /** @brief Destroy through the public derived destructor and protected base destructor. */
  ~PortBProbe() override = default;

  /** @brief Select the const indexed interface overload explicitly. */
  const IF* const_interface_at(int index) const
  {
    return sc_core::sc_port_b<IF>::get_interface(index);
  }
  /** @brief Select the const indexed subscript overload explicitly. */
  const IF* const_subscript(int index) const
  {
    return static_cast<const sc_core::sc_port_b<IF>&>(*this)[index];
  }
  /** @brief Select the mutable arrow overload explicitly. */
  IF* interface_arrow()
  {
    return sc_core::sc_port_b<IF>::operator->();
  }
  /** @brief Select the const arrow overload explicitly. */
  const IF* const_interface_arrow() const
  {
    return sc_core::sc_port_b<IF>::operator->();
  }
  /** @brief Select the zero-argument mutable base-interface overload explicitly. */
  sc_core::sc_interface* base_interface()
  {
    return sc_core::sc_port_b<IF>::get_interface();
  }
  /** @brief Select the zero-argument const base-interface overload explicitly. */
  const sc_core::sc_interface* const_base_interface() const
  {
    return sc_core::sc_port_b<IF>::get_interface();
  }
  /** @brief Exercise protected interface vbind with a dynamically checked base ref. */
  int probe_vbind_interface(sc_core::sc_interface& interface)
  {
    return this->vbind(interface);
  }
  /** @brief Exercise protected parent-port vbind with a dynamically checked base ref. */
  int probe_vbind_port(sc_core::sc_port_base& parent)
  {
    return this->vbind(parent);
  }
  /** @brief Exercise protected thread make_sensitive without scheduling a process. */
  void probe_thread_sensitivity()
  {
    this->make_sensitive(static_cast<sc_core::sc_thread_handle>(nullptr));
  }
  /** @brief Exercise protected method make_sensitive without scheduling a process. */
  void probe_method_sensitivity()
  {
    this->make_sensitive(static_cast<sc_core::sc_method_handle>(nullptr));
  }
};

/** @brief Compare interface pointers without transferring ownership. */
template <class IF>
inline bool same_interface(const IF* left, const IF* right)
{
  return left == right;
}

/** @brief Read a custom interface through a port's native arrow operator. */
template <class IF>
inline int read_port_value(sc_core::sc_port_b<IF>& port)
{
  return port.operator->()->value();
}

/** @brief Read a custom interface through the const native arrow operator. */
template <class IF>
inline int read_const_port_value(const sc_core::sc_port_b<IF>& port)
{
  return port.operator->()->value();
}

/** @brief Compare an interface pointer against a live concrete channel. */
template <class IF, class OBJECT>
inline bool interface_matches(const sc_core::sc_interface* interface, const OBJECT& object)
{
  return interface == dynamic_cast<const sc_core::sc_interface*>(dynamic_cast<const IF*>(&object));
}

/** @brief Compare references after cppyy maps an interface pointer to a proxy reference. */
template <class IF, class OBJECT>
inline bool interface_matches_ref(const IF& interface, const OBJECT& object)
{
  return &interface == dynamic_cast<const IF*>(&object);
}

/** @brief Select indexed const get_interface for any public port specialization. */
template <class PORT>
inline auto const_interface_at(const PORT& port, int index) -> decltype(port.get_interface(index))
{
  return port.get_interface(index);
}

/** @brief Select const operator[] for any public port specialization. */
template <class PORT>
inline auto const_subscript(const PORT& port, int index) -> decltype(port[index])
{
  return port[index];
}

/** @brief Select const interface arrow for any public port specialization. */
template <class PORT>
inline auto const_arrow(const PORT& port) -> decltype(port.operator->())
{
  return port.operator->();
}

/** @brief Compare a native sc_type_index with its expected interface type. */
template <class IF>
inline bool has_interface_type(const sc_core::sc_type_index& type)
{
  return type == typeid(IF);
}

/** @brief Select const generic-port access through the native subscript overload. */
template <class IF, int N, sc_core::sc_port_policy P>
inline const IF* const_port_subscript(const sc_core::sc_port<IF, N, P>& port, int index)
{
  return port[index];
}

}  // namespace sc_core_ports_probe

#endif
