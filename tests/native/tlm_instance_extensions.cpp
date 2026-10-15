/** @file Independent native instance-private extension lifecycle reference. */
#include <tlm_utils/instance_specific_extensions.h>

#include <iostream>
#include <stdexcept>
#include <systemc>
#include <tlm>

/** @brief Borrowed extension storage owned by the reference stack. */
struct PrivateValue : tlm_utils::instance_specific_extension<PrivateValue>
{
  int value = 0;
};

/** @brief Verify semantics even when NDEBUG disables native assertions. */
void verify(bool condition)
{
  if (!condition) throw std::runtime_error("private extension reference failed");
}

/** @brief Exercise native accessors without Python callbacks or GIL requirements.
 * @details Extensions remain alive until explicitly cleared. Per-accessor
 * references are never accessed after the final clear releases their container.
 */
int sc_main(int, char**)
{
  tlm::tlm_generic_payload first, second, copied;
  first.resize_extensions();
  second.resize_extensions();
  copied.resize_extensions();
  tlm_utils::instance_specific_extension_accessor a, b;
  PrivateValue x, y, replacement, copy_value;
  x.value = 17;
  y.value = 23;
  replacement.value = 31;
  copy_value.value = 41;
  auto& av = a(first);
  auto& bv = b(first);
  auto& other = a(second);
  verify(av.set_extension(&x) == nullptr);
  verify(bv.set_extension(&y) == nullptr);
  verify(av.get_extension(PrivateValue::priv_id) == &x);
  verify(bv.get_extension(PrivateValue::priv_id) == &y);
  verify(other.get_extension(PrivateValue::priv_id) == nullptr);
  first.set_address(73);
  copied.deep_copy_from(first);
  verify(copied.get_address() == 73);
  verify(copied.get_extension(tlm_utils::instance_specific_extension_carrier::ID) == nullptr);
  auto& copy_access = a(copied);
  verify(copy_access.get_extension(PrivateValue::priv_id) == nullptr);
  verify(copy_access.set_extension(&copy_value) == nullptr);
  verify(av.get_extension(PrivateValue::priv_id) == &x);
  copy_access.clear_extension(PrivateValue::priv_id);
  verify(copied.get_extension(tlm_utils::instance_specific_extension_carrier::ID) == nullptr);
  verify(av.set_extension(&replacement) == &x && x.value == 17);
  verify(av.get_extension(PrivateValue::priv_id) == &replacement);
  av.clear_extension(PrivateValue::priv_id);
  verify(av.get_extension(PrivateValue::priv_id) == nullptr);
  verify(bv.get_extension(PrivateValue::priv_id) == &y);
  bv.clear_extension(PrivateValue::priv_id);
  verify(first.get_extension(tlm_utils::instance_specific_extension_carrier::ID) == nullptr);
  auto& fresh = a(first);
  verify(fresh.get_extension(PrivateValue::priv_id) == nullptr);
  verify(fresh.set_extension(&x) == nullptr);
  fresh.clear_extension(PrivateValue::priv_id);
  verify(first.get_extension(tlm_utils::instance_specific_extension_carrier::ID) == nullptr);
  std::cout << "PRIVATE_EXTENSIONS 17 23 31 41 73 detached\n";
  return 0;
}
