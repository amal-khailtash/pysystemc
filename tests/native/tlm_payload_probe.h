/** @file Native storage and lifetime probes for generic payload operations. */
#pragma once

#include <array>
#include <cstdint>
#include <systemc>
#include <tlm>

namespace payload_probe
{
/** @brief Extension with independently observable clone, copy and free behavior.
 * @details Payloads own installed instances unless explicitly cleared. All
 * operations run on the calling thread without Python callbacks or GIL entry.
 */
template <int Width>
struct Extension : tlm::tlm_extension<Extension<Width>>
{
  sc_dt::sc_biguint<Width> value;
  inline static int live = 0;
  inline static int clones = 0;
  inline static int copies = 0;
  inline static int frees = 0;

  /** @brief Construct an owned extension with an unrestricted native width. */
  explicit Extension(unsigned low = 0) : value(low)
  {
    value[Width - 1] = true;
    ++live;
  }
  /** @brief Count destruction regardless of the native release path. */
  ~Extension() override
  {
    --live;
  }
  /** @brief Return a newly allocated copy owned by the receiving payload. */
  tlm::tlm_extension_base* clone() const override
  {
    auto* result = new Extension;
    result->value = value;
    ++clones;
    return result;
  }
  /** @brief Copy from a borrowed extension of the exact registered type. */
  void copy_from(const tlm::tlm_extension_base& other) override
  {
    value = static_cast<const Extension&>(other).value;
    ++copies;
  }
  /** @brief Release owned native storage and invalidate all borrowed views. */
  void free() override
  {
    ++frees;
    delete this;
  }
};

/** @brief Pool-like manager records release and resets cached auto extensions.
 * @details It borrows payloads instead of deleting them. Retain this manager
 * through the last payload release; free() performs no Python callback.
 */
struct Manager : tlm::tlm_mm_interface
{
  int releases = 0;
  tlm::tlm_generic_payload* last = nullptr;
  /** @brief Observe zero reference count and reset the borrowed payload. */
  void free(tlm::tlm_generic_payload* payload) override
  {
    ++releases;
    last = payload;
    payload->reset();
  }
};

/** @brief Own aligned buffers separately from borrowed payload pointers.
 * @details The manager and buffers outlive both payloads. Payload destruction
 * frees extensions, while data and enable buffers remain fixture-owned.
 */
struct Fixture
{
  Manager manager;
  alignas(std::uint64_t) unsigned char source_data[16] = {};
  alignas(std::uint64_t) unsigned char copy_data[16] = {};
  alignas(std::uint64_t) unsigned char source_enable[8] = {};
  alignas(std::uint64_t) unsigned char copy_enable[8] = {};
  tlm::tlm_generic_payload source;
  tlm::tlm_generic_payload copy;

  /** @brief Fill independent owned buffers with recognizable values. */
  Fixture()
  {
    for (unsigned i = 0; i < 16; ++i)
    {
      source_data[i] = static_cast<unsigned char>(i + 1);
      copy_data[i] = 0xA0;
    }
    for (unsigned i = 0; i < 8; ++i)
    {
      source_enable[i] = i % 2 ? 0 : 0xFF;
      copy_enable[i] = 0;
    }
  }
};
}  // namespace payload_probe
