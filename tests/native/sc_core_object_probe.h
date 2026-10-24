/** @file sc_core_object_probe.h
 *  @brief Native-derived access probes for sc_object's protected test surface.
 *  @details Probe objects own their own attributes and hierarchy registration.
 *  Child/event links borrow their targets, and the caller keeps those targets
 *  alive while the links exist. The wrappers are synchronous, do not release
 *  the GIL, and retain no Python callbacks.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_OBJECT_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_OBJECT_PROBE_H

#include <sstream>
#include <systemc>

namespace sc_core_object_probe
{

/** @brief Concrete sc_object exposing native protected constructors/helpers. */
class ObjectProbe : public sc_core::sc_object
{
 public:
  /** @brief Construct with the native unique-name constructor. */
  ObjectProbe() : sc_object()
  {
  }
  /** @brief Construct a named object using native hierarchy registration. */
  explicit ObjectProbe(const char* name) : sc_object(name)
  {
  }
  /** @brief Copy only the native object's name seed and hierarchy context. */
  ObjectProbe(const ObjectProbe& other) : sc_object(other)
  {
  }
  /** @brief Destroy through the native virtual base destructor. */
  ~ObjectProbe() override = default;

  /** @brief Add a borrowed child-object pointer through the protected base API. */
  void add_child(sc_core::sc_object* child)
  {
    add_child_object(child);
  }
  /** @brief Remove a borrowed child-object pointer through the protected base API. */
  bool remove_child(sc_core::sc_object* child)
  {
    return remove_child_object(child);
  }
  /** @brief Add a borrowed child-event pointer through the protected base API. */
  void add_event(sc_core::sc_event* event)
  {
    add_child_event(event);
  }
  /** @brief Remove a borrowed child-event pointer through the protected base API. */
  bool remove_event(sc_core::sc_event* event)
  {
    return remove_child_event(event);
  }
  /** @brief Register a phase mask through the protected base API. */
  phase_cb_mask register_mask(phase_cb_mask mask)
  {
    return register_simulation_phase_callback(mask);
  }
  /** @brief Unregister a phase mask through the protected base API. */
  phase_cb_mask unregister_mask(phase_cb_mask mask)
  {
    return unregister_simulation_phase_callback(mask);
  }
  /** @brief Invoke the protected no-op assignment operator and return its base reference. */
  sc_core::sc_object& assign_from(const sc_core::sc_object& other)
  {
    return sc_object::operator=(other);
  }
};

/** @brief A module whose members give sc_object real parent/child relationships. */
class ObjectTreeProbe : public sc_core::sc_module
{
 public:
  ObjectProbe first;
  ObjectProbe second;
  sc_core::sc_event event;

  /** @brief Construct named children beneath a native module hierarchy scope. */
  explicit ObjectTreeProbe(sc_core::sc_module_name name)
    : sc_module(name), first("first"), second("second"), event("event")
  {
  }
};

/** @brief Invoke the const get_attribute overload and return its borrowed pointer. */
inline const sc_core::sc_attr_base* const_attribute(const sc_core::sc_object& object, const std::string& name)
{
  return object.get_attribute(name);
}

/** @brief Invoke the const attr_cltn overload and return its borrowed collection. */
inline const sc_core::sc_attr_cltn& const_attributes(const sc_core::sc_object& object)
{
  return object.attr_cltn();
}

/** @brief Copy formatted native output from sc_object::print. */
inline std::string printed(const sc_core::sc_object& object)
{
  std::ostringstream stream;
  object.print(stream);
  return stream.str();
}

/** @brief Copy formatted native output from sc_object::dump. */
inline std::string dumped(const sc_core::sc_object& object)
{
  std::ostringstream stream;
  object.dump(stream);
  return stream.str();
}

/** @brief Compare object identity without transferring ownership. */
inline bool same_object(const sc_core::sc_object* left, const sc_core::sc_object* right)
{
  return left == right;
}

/** @brief Compare event identity without transferring ownership. */
inline bool same_event(const sc_core::sc_event* left, const sc_core::sc_event* right)
{
  return left == right;
}

}  // namespace sc_core_object_probe

#endif
