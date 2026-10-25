/** @file tlm_extensions_public.cpp
 *  @brief Independent native reference for public TLM extension families.
 *  @details One process owns the SystemC singleton and all native callbacks.
 *  Every extension pointer in this reference is released through its native
 *  ``free`` virtual before process exit.
 */
#include <iostream>
#include <stdexcept>

#include "tlm_extensions_probe.h"

namespace
{
void verify(bool condition)
{
  if (!condition) throw std::runtime_error("TLM extension reference failed");
}
}  // namespace

int sc_main(int, char**)
{
  using namespace tlm_extensions_probe;
  const unsigned extension_id = WideValue::registered_id();
  const unsigned endian_id = tlm::tlm_endian_context::ID;
  const unsigned carrier_id = tlm_utils::instance_specific_extension_carrier::ID;
  verify(endian_id < tlm::max_num_extensions());
  verify(carrier_id < tlm::max_num_extensions());
  verify(extension_id == WideValue::register_again());
  verify(extension_id < tlm::max_num_extensions());
  verify(PrivateWide::priv_id == PrivateRegistry::register_again());
  auto* base_default_free = new BaseDefaultFree;
  base_default_free->free();
  verify(BaseDefaultFree::created == 1 && BaseDefaultFree::destroyed == 1);

  auto* source = new WideValue(7000);
  tlm::tlm_generic_payload payload;
  payload.resize_extensions();
  verify(payload.set_extension(source) == nullptr);
  verify(payload.get_extension(extension_id) == source);
  WideValue* typed_payload_result = nullptr;
  payload.get_extension(typed_payload_result);
  verify(typed_payload_result == source);

  auto* duplicate = source->clone();
  verify(duplicate != nullptr);
  auto* typed_duplicate = dynamic_cast<WideValue*>(duplicate);
  verify(typed_duplicate != nullptr);
  verify(typed_duplicate->words[136] == 7000ULL + 136ULL * 17ULL);
  auto* target = new WideValue;
  target->copy_from(*duplicate);
  verify(target->words[136] == typed_duplicate->words[136]);
  duplicate->free();
  target->free();

  tlm_utils::instance_specific_extension_accessor accessor_a;
  tlm_utils::instance_specific_extension_accessor accessor_b;
  tlm_utils::instance_specific_extensions_per_accessor& view_a = accessor_a(payload);
  tlm_utils::instance_specific_extensions_per_accessor& view_b = accessor_b(payload);
  PrivateWide private_wide(801);
  PrivateOther private_other(902);
  verify(view_a.set_extension(&private_wide) == nullptr);
  verify(view_b.set_extension(PrivateOther::priv_id, &private_other) == nullptr);
  PrivateWide* typed_result = nullptr;
  view_a.get_extension(typed_result);
  verify(typed_result == &private_wide);
  verify(view_a.get_extension(PrivateWide::priv_id) == &private_wide);
  verify(view_b.get_extension(PrivateOther::priv_id) == &private_other);
  view_a.resize_extensions();
  view_b.clear_extension(&private_other);
  view_a.clear_extension(PrivateWide::priv_id);
  verify(view_a.get_extension(PrivateWide::priv_id) == nullptr);
  verify(payload.get_extension(extension_id) == source);
  payload.clear_extension(source);
  source->free();

  auto* endian_context = new tlm::tlm_endian_context;
  endian_context->copy_from(*endian_context);
  verify(endian_context->clone() == nullptr);
  endian_context->free();
  tlm_utils::instance_specific_extension_carrier carrier;
  carrier.copy_from(carrier);
  verify(carrier.clone() == nullptr);
  carrier.free();
  using container_type = tlm_utils::instance_specific_extensions_per_accessor::container_type;
  container_type* null_container = nullptr;
  tlm_utils::instance_specific_extensions_per_accessor detached(null_container);
  detached.resize_extensions();

  CountingManager manager;
  tlm::tlm_generic_payload managed(&manager);
  managed.set_address(0x123456789ULL);
  managed.acquire();
  managed.release();
  verify(manager.calls == 1);
  verify(manager.last_ref_count == 0);
  verify(manager.last_address == 0x123456789ULL);

  std::cout << "EXTENSION_IDS stable 137\n";
  std::cout << "EXTENSION_LIFECYCLE " << WideValue::created << ' ' << WideValue::cloned << ' ' << WideValue::copied
            << ' ' << WideValue::freed << '\n';
  std::cout << "BASE_EXTENSION_FREE 1 1\n";
  std::cout << "EXTENSION_SPECIALIZATIONS nullclone pooled\n";
  std::cout << "PRIVATE_ACCESS 801 902 isolated cleared\n";
  std::cout << "MEMORY_MANAGER 1 0 4886718345\n";
  std::cout << "TLM_EXTENSIONS_PUBLIC_OK\n";
  return 0;
}
