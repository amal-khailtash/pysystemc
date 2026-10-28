/** @file sc_core_port_base_probe.h
 *  @brief Native-derived probes for sc_port_base construction and helpers.
 *  @details Ports own their binding state and borrow interfaces and parent
 *  ports. Tests keep those objects alive until all binding probes are
 *  destroyed. The wrappers execute synchronously with the GIL held and retain
 *  no Python callback or interface owner.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_PORT_BASE_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_PORT_BASE_PROBE_H

#include <sstream>
#include <systemc>
#include <vector>

namespace sc_core_port_base_probe
{

/** @brief Minimal native interface used for exact port binding checks. */
class ProbeInterface : public sc_core::sc_interface
{
 public:
  /** @brief Public virtual destructor for the concrete probe interface. */
  ~ProbeInterface() override = default;
};

/** @brief Concrete port exposing the protected sc_port_base declaration surface. */
class PortProbe : public sc_core::sc_port_base
{
 public:
  /** @brief Construct through the protected unnamed native constructor. */
  explicit PortProbe(int max_size, sc_core::sc_port_policy policy = sc_core::SC_ONE_OR_MORE_BOUND)
    : sc_port_base(max_size, policy), hooks_()
  {
  }

  /** @brief Construct through the protected named native constructor. */
  PortProbe(const char* name, int max_size, sc_core::sc_port_policy policy = sc_core::SC_ONE_OR_MORE_BOUND)
    : sc_port_base(name, max_size, policy), hooks_()
  {
  }

  /** @brief Destroy native binding state through the virtual base destructor. */
  ~PortProbe() override = default;

  /** @brief Return the first bound interface through the non-const overload. */
  sc_core::sc_interface* get_interface() override
  {
    return interfaces_.empty() ? nullptr : interfaces_.front();
  }
  /** @brief Return the first bound interface through the const overload. */
  const sc_core::sc_interface* get_interface() const override
  {
    return interfaces_.empty() ? nullptr : interfaces_.front();
  }
  /** @brief Return the exact interface RTTI type for this probe. */
  sc_core::sc_type_index get_interface_type() const override
  {
    return typeid(ProbeInterface);
  }

  /** @brief Bind through the protected interface-reference overload. */
  void bind_interface(sc_core::sc_interface& interface)
  {
    bind(interface);
  }
  /** @brief Bind through the protected parent-port-reference overload. */
  void bind_parent(sc_core::sc_port_base& parent)
  {
    bind(parent);
  }
  /** @brief Reach the protected thread sensitivity overload without scheduling it. */
  void probe_thread_sensitivity()
  {
    make_sensitive(static_cast<sc_core::sc_thread_handle>(nullptr));
  }
  /** @brief Reach the protected method sensitivity overload without scheduling it. */
  void probe_method_sensitivity()
  {
    make_sensitive(static_cast<sc_core::sc_method_handle>(nullptr));
  }
  /** @brief Read whether native binding storage is live. */
  bool has_binding_state() const
  {
    return m_bind_info != nullptr;
  }
  /** @brief Invoke all four protected lifecycle hooks through virtual dispatch. */
  std::string run_lifecycle_hooks()
  {
    hooks_.clear();
    before_end_of_elaboration();
    end_of_elaboration();
    start_of_simulation();
    end_of_simulation();
    return hooks_;
  }
  /** @brief Invoke protected report_error and copy the native diagnostic. */
  std::string report_error_text() const
  {
    try
    {
      report_error(sc_core::SC_ID_BIND_IF_TO_PORT_, "port probe");
    }
    catch (const sc_core::sc_report& report)
    {
      return report.what();
    }
    return "no report";
  }

 protected:
  /** @brief Record native before-elaboration dispatch for parity checks. */
  void before_end_of_elaboration() override
  {
    hooks_ += 'B';
  }
  /** @brief Record native end-elaboration dispatch for parity checks. */
  void end_of_elaboration() override
  {
    hooks_ += 'E';
  }
  /** @brief Record native start-simulation dispatch for parity checks. */
  void start_of_simulation() override
  {
    hooks_ += 'S';
  }
  /** @brief Record native end-simulation dispatch for parity checks. */
  void end_of_simulation() override
  {
    hooks_ += 'F';
  }
  /** @brief Record interfaces added by native binding completion. */
  void add_interface(sc_core::sc_interface* interface) override
  {
    interfaces_.push_back(interface);
  }
  /** @brief Return the number of interfaces recorded by this port. */
  int interface_count() const override
  {
    return static_cast<int>(interfaces_.size());
  }
  /** @brief Accept interface and parent binding during native positional binding. */
  int vbind(sc_core::sc_interface& interface) override
  {
    bind(interface);
    return 0;
  }
  /** @brief Accept parent-port binding during native positional binding. */
  int vbind(sc_core::sc_port_base& parent) override
  {
    bind(parent);
    return 0;
  }

 private:
  std::vector<sc_core::sc_interface*> interfaces_;
  std::string hooks_;
};

/** @brief Own valid ports beneath a module hierarchy, as required by SystemC. */
class PortTreeProbe : public sc_core::sc_module
{
 public:
  PortProbe unnamed;
  PortProbe default_policy;
  PortProbe named;
  PortProbe named_default_policy;
  PortProbe parent;
  PortProbe child;
  ProbeInterface interface;

  /** @brief Construct all ports while the native module hierarchy is active. */
  explicit PortTreeProbe(sc_core::sc_module_name name)
    : sc_module(name)
    , unnamed(1, sc_core::SC_ZERO_OR_MORE_BOUND)
    , default_policy(1)
    , named("named_port", 2, sc_core::SC_ZERO_OR_MORE_BOUND)
    , named_default_policy("named_default_policy", 1)
    , parent("parent_port", 1, sc_core::SC_ZERO_OR_MORE_BOUND)
    , child("child_port", 1, sc_core::SC_ZERO_OR_MORE_BOUND)
  {
  }
};

/** @brief Select the const get_interface overload explicitly. */
inline const sc_core::sc_interface* const_interface(const sc_core::sc_port_base& port)
{
  return port.get_interface();
}

/** @brief Select the interface-reference binding overload explicitly. */
inline void bind_interface(sc_core::sc_port_base& port, sc_core::sc_interface& interface)
{
  static_cast<PortProbe&>(port).bind_interface(interface);
}

/** @brief Select the parent-port-reference binding overload explicitly. */
inline void bind_parent(sc_core::sc_port_base& port, sc_core::sc_port_base& parent)
{
  static_cast<PortProbe&>(port).bind_parent(parent);
}

/** @brief Compare interface identity without transferring ownership. */
inline bool same_interface(const sc_core::sc_interface* left, const sc_core::sc_interface* right)
{
  return left == right;
}

/** @brief Compare port identity without transferring ownership. */
inline bool same_port(const sc_core::sc_port_base* left, const sc_core::sc_port_base* right)
{
  return left == right;
}

}  // namespace sc_core_port_base_probe

#endif
