/** @file tlm_extensions_probe.h
 *  @brief Native probes for TLM extension virtuals and private accessor APIs.
 *  @details The probes own their counter state for the lifetime of the
 *  process. Returned extension pointers are owned by the caller until
 *  ``free`` is called. The probes do not start SystemC processes or release
 *  the GIL; Python-facing calls are synchronous and must retain the callback
 *  owner until the native interface is no longer used.
 */
#ifndef PYSYSTEMC_TLM_EXTENSIONS_PROBE_H
#define PYSYSTEMC_TLM_EXTENSIONS_PROBE_H

#include <tlm_core/tlm_2/tlm_generic_payload/tlm_endian_conv.h>
#include <tlm_utils/instance_specific_extensions.h>

#include <array>
#include <tlm>

namespace tlm_extensions_probe
{
/** @brief Arbitrary-width extension state with observable native lifecycle. */
struct WideValue : tlm::tlm_extension<WideValue>
{
  /** @brief Construct one value and record native allocation. */
  explicit WideValue(unsigned long long seed = 0) : words{}
  {
    ++created;
    for (unsigned index = 0; index < 137; ++index) words[index] = seed + index * 17ULL;
  }

  /** @brief Clone the full 137-word state into caller-owned native storage. */
  tlm::tlm_extension_base* clone() const override
  {
    auto* copy = new WideValue;
    for (unsigned index = 0; index < 137; ++index) copy->words[index] = words[index];
    ++cloned;
    return copy;
  }

  /** @brief Copy state after validating the exact dynamic extension type. */
  void copy_from(const tlm::tlm_extension_base& source) override
  {
    const auto* typed = dynamic_cast<const WideValue*>(&source);
    if (!typed) throw std::bad_cast();
    for (unsigned index = 0; index < 137; ++index) words[index] = typed->words[index];
    ++copied;
  }

  /** @brief Delete this caller-owned extension and record the release. */
  void free() override
  {
    ++freed;
    delete this;
  }

  /** @brief Register the custom type through the protected base API. */
  static unsigned register_again()
  {
    return register_extension(typeid(WideValue));
  }

  /** @brief Return the statically registered ID from the public template. */
  static unsigned registered_id()
  {
    return tlm::tlm_extension<WideValue>::ID;
  }

  unsigned long long words[137];
  inline static unsigned created = 0;
  inline static unsigned cloned = 0;
  inline static unsigned copied = 0;
  inline static unsigned freed = 0;
};

/** @brief Concrete base-derived extension using tlm_extension_base::free. */
struct BaseDefaultFree : tlm::tlm_extension_base
{
  /** @brief Record one public derived construction. */
  BaseDefaultFree()
  {
    ++created;
  }

  /** @brief Record destruction through the protected virtual base destructor. */
  ~BaseDefaultFree() override
  {
    ++destroyed;
  }

  /** @brief Satisfy the pure virtual clone contract. */
  tlm::tlm_extension_base* clone() const override
  {
    return new BaseDefaultFree;
  }

  /** @brief Satisfy the pure virtual copy contract. */
  void copy_from(const tlm::tlm_extension_base&) override
  {
  }

  inline static unsigned created = 0;
  inline static unsigned destroyed = 0;
};

/** @brief Native memory manager recording calls at zero payload references. */
struct CountingManager : tlm::tlm_mm_interface
{
  /** @brief Initialize externally inspected callback counters. */
  CountingManager() : calls(0), last_ref_count(-1), last_address(0)
  {
  }

  /** @brief Record the borrowed payload and leave its storage with its owner. */
  void free(tlm::tlm_generic_payload* payload) override
  {
    ++calls;
    last_ref_count = payload->get_ref_count();
    last_address = payload->get_address();
  }

  unsigned calls;
  int last_ref_count;
  unsigned long long last_address;
};

/** @brief Minimal typed private extension exercising priv_id registration. */
struct PrivateWide : tlm_utils::instance_specific_extension<PrivateWide>
{
  /** @brief Initialize custom private payload state. */
  explicit PrivateWide(unsigned long long initial = 0) : value(initial)
  {
  }
  unsigned long long value;
};

/** @brief A second private payload type for distinct typed/manual slots. */
struct PrivateOther : tlm_utils::instance_specific_extension<PrivateOther>
{
  /** @brief Initialize the second private extension value. */
  explicit PrivateOther(unsigned long long initial = 0) : value(initial)
  {
  }
  unsigned long long value;
};

/** @brief Expose protected private-ID registration for stable-ID checks. */
struct PrivateRegistry : tlm_utils::ispex_base
{
  /** @brief Register an already-used private type a second time. */
  static unsigned register_again()
  {
    return register_private_extension(typeid(PrivateWide));
  }
};
}  // namespace tlm_extensions_probe

#endif  // PYSYSTEMC_TLM_EXTENSIONS_PROBE_H
