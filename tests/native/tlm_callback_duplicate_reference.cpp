/** @file tlm_callback_duplicate_reference.cpp
 *  @brief Native reference for simple and tagged target duplicate callbacks.
 *  @details Registration and interface dispatch use upstream sockets directly.
 *  The module owns both sockets; the temporary payload and DMI result are
 *  caller-owned for each immediate callback.
 */
#include <tlm_utils/simple_target_socket.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tlm>

/** @brief Native owner recording callback selection and tagged DMI ID. */
struct DuplicateReference : sc_core::sc_module
{
  using Payload = tlm::tlm_generic_payload;
  using Protocol = tlm::tlm_base_protocol_types;
  using Simple = tlm_utils::simple_target_socket_optional<DuplicateReference, 32, Protocol>;
  using Tagged = tlm_utils::simple_target_socket_tagged_optional<DuplicateReference, 32, Protocol>;

  Simple simple;
  Tagged tagged;
  std::string simple_callback;
  std::string tagged_callback;
  int tagged_id = -1;

  /** @brief Construct sockets, register duplicates, and dispatch native paths. */
  explicit DuplicateReference(sc_core::sc_module_name name) : sc_module(name), simple("simple"), tagged("tagged")
  {
    simple.register_b_transport(this, &DuplicateReference::simple_first);
    simple.register_b_transport(this, &DuplicateReference::simple_duplicate);
    Payload payload;
    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
    simple.get_base_interface().b_transport(payload, delay);

    tagged.register_get_direct_mem_ptr(this, &DuplicateReference::tagged_first, 11);
    tagged.register_get_direct_mem_ptr(this, &DuplicateReference::tagged_duplicate, 22);
    tlm::tlm_dmi dmi;
    tagged.get_base_interface().get_direct_mem_ptr(payload, dmi);
  }

  /** @brief Record the first simple target callback. */
  void simple_first(Payload&, sc_core::sc_time&)
  {
    simple_callback = "first";
  }

  /** @brief Record an unexpected simple duplicate callback. */
  void simple_duplicate(Payload&, sc_core::sc_time&)
  {
    simple_callback = "duplicate";
  }

  /** @brief Record the initial tagged DMI callback and ID. */
  bool tagged_first(int id, Payload&, tlm::tlm_dmi&)
  {
    tagged_callback = "first";
    tagged_id = id;
    return false;
  }

  /** @brief Record the replacement tagged DMI callback and ID. */
  bool tagged_duplicate(int id, Payload&, tlm::tlm_dmi&)
  {
    tagged_callback = "duplicate";
    tagged_id = id;
    return false;
  }
};

int sc_main(int, char**)
{
  sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);
  DuplicateReference model("duplicate_reference");
  if (model.simple_callback != "first" || model.tagged_callback != "duplicate" || model.tagged_id != 22)
    throw std::runtime_error("upstream duplicate registration policy changed");
  std::cout << "DUPLICATE_REF simple " << model.simple_callback << " -1\n";
  std::cout << "DUPLICATE_REF tagged_dmi " << model.tagged_callback << ' ' << model.tagged_id << '\n';
  return 0;
}
