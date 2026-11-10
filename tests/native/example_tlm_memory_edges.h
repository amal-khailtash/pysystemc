/** @file example_tlm_memory_edges.h
 * @brief Native reference probes for the upstream TLM example memory and DMI target.
 * @details Calls the unmodified SystemC example implementations directly. All
 * payload data storage is aligned and remains at least four bytes long because
 * upstream report::print reads a native unsigned int regardless of payload length.
 */
#pragma once

#include <tlm_utils/simple_initiator_socket.h>

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "lt_dmi_target.h"
#include "memory.h"
#include "reporting.h"

namespace example_tlm_memory_edges
{
/** @brief Render a byte range without reading outside retained native storage. */
inline std::string bytes(const unsigned char* data, std::size_t count)
{
  std::ostringstream output;
  output << std::hex << std::uppercase << std::setfill('0');
  for (std::size_t index = 0; index < count; ++index)
  {
    if (index) output << ',';
    output << std::setw(2) << static_cast<unsigned>(data[index]);
  }
  return output.str();
}

/** @brief Exercise actual memory::operation with safe payload backing storage. */
class MemoryProbe
{
 public:
  MemoryProbe() : memory_(9, sc_core::sc_time(3, sc_core::SC_NS), sc_core::sc_time(7, sc_core::SC_NS), 16, 4)
  {
  }

  /** @brief Run valid, malformed-attribute, range, length and command cases. */
  void run()
  {
    run_case("write4", tlm::TLM_WRITE_COMMAND, 0, 4, 4, false, {0x11, 0x22, 0x33, 0x44});
    run_case("read4", tlm::TLM_READ_COMMAND, 0, 4, 4, false, {0xA0, 0xA1, 0xA2, 0xA3});
    run_case("byteenable_write", tlm::TLM_WRITE_COMMAND, 4, 4, 4, true, {0x51, 0x62, 0x73, 0x84});
    run_case("stream_mismatch_read", tlm::TLM_READ_COMMAND, 4, 4, 3, false, {0xB0, 0xB1, 0xB2, 0xB3});
    run_case("length1_read", tlm::TLM_READ_COMMAND, 0, 1, 1, false, {0xC0, 0xC1, 0xC2, 0xC3});
    run_case("length5_write", tlm::TLM_WRITE_COMMAND, 8, 5, 5, false, {0x91, 0x82, 0x73, 0x64, 0x55, 0xC5, 0xC6, 0xC7});
    run_case("length0_read", tlm::TLM_READ_COMMAND, 15, 0, 0, false, {0xD0, 0xD1, 0xD2, 0xD3});
    run_case("exact_end", tlm::TLM_READ_COMMAND, 12, 4, 4, false, {0xE0, 0xE1, 0xE2, 0xE3});
    run_case("cross_end", tlm::TLM_WRITE_COMMAND, 13, 4, 4, false, {0x31, 0x32, 0x33, 0x34});
    run_case("out_of_range", tlm::TLM_READ_COMMAND, 16, 4, 4, false, {0xF0, 0xF1, 0xF2, 0xF3});
    run_case("bad_address_byteenable", tlm::TLM_READ_COMMAND, 16, 4, 4, true, {0xE0, 0xE1, 0xE2, 0xE3});
    run_case("ignore_first", tlm::TLM_IGNORE_COMMAND, 0, 4, 4, false, {0xA5, 0xA6, 0xA7, 0xA8});
    run_case("ignore_second", tlm::TLM_IGNORE_COMMAND, 0, 4, 4, false, {0xB5, 0xB6, 0xB7, 0xB8});
    run_case("bad_address_ignore", tlm::TLM_IGNORE_COMMAND, 16, 4, 4, false, {0xC5, 0xC6, 0xC7, 0xC8});
  }

 private:
  /** @brief Apply one transaction and print response, bytes, memory and annotation. */
  void run_case(const char* label, tlm::tlm_command command, sc_dt::uint64 address, unsigned length, unsigned stream,
                bool byte_enable, std::array<unsigned char, 8> initial)
  {
    alignas(unsigned int) std::array<unsigned char, 8> data = initial;
    unsigned char enables[2] = {0xFF, 0x00};
    tlm::tlm_generic_payload payload;
    payload.set_command(command);
    payload.set_address(address);
    payload.set_data_ptr(data.data());
    payload.set_data_length(length);
    payload.set_streaming_width(stream);
    payload.set_byte_enable_ptr(byte_enable ? enables : nullptr);
    payload.set_byte_enable_length(byte_enable ? 2 : 0);
    payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
    sc_core::sc_time delay(2, sc_core::SC_NS);
    memory_.operation(payload, delay);
    std::cout << "MEM|" << label << '|' << payload.get_response_string() << '|' << delay << '|' << bytes(data.data(), 8)
              << '|' << bytes(memory_.get_mem_ptr(), 16) << '\n';
  }

  memory memory_;
};

/** @brief Exercise the actual lt_dmi_target callback through a native socket. */
class DmiProbe : public sc_core::sc_module
{
 public:
  tlm_utils::simple_initiator_socket<DmiProbe> initiator;
  lt_dmi_target target;

  SC_HAS_PROCESS(DmiProbe);
  /** @brief Bind native sockets and schedule calls while DMI is disabled. */
  explicit DmiProbe(sc_core::sc_module_name name)
    : sc_core::sc_module(name)
    , initiator("initiator")
    , target("target", 9, "target_socket", 4096, 4, sc_core::sc_time(20, sc_core::SC_NS),
             sc_core::sc_time(3, sc_core::SC_NS), sc_core::sc_time(7, sc_core::SC_NS))
  {
    initiator.bind(target.m_memory_socket);
    initiator.register_invalidate_direct_mem_ptr(this, &DmiProbe::invalidate);
    SC_THREAD(check_disabled);
  }

  /** @brief Check source inclusive-endpoint DMI behavior before simulation. */
  void check_enabled()
  {
    for (sc_dt::uint64 address : {sc_dt::uint64(0), sc_dt::uint64(4096), sc_dt::uint64(4097)})
      check_one("enabled", address);
  }

 private:
  /** @brief Record the source invalidation callback without retaining its range. */
  void invalidate(sc_dt::uint64 first, sc_dt::uint64 last)
  {
    std::cout << "INVALIDATE|" << first << '|' << last << '\n';
  }

  /** @brief Query one source DMI address and report any granted descriptor. */
  void check_one(const char* phase, sc_dt::uint64 address)
  {
    tlm::tlm_generic_payload payload;
    payload.set_address(address);
    tlm::tlm_dmi result;
    const bool granted = initiator->get_direct_mem_ptr(payload, result);
    std::cout << "DMI|" << phase << '|' << address << '|' << granted;
    if (granted)
      std::cout << '|' << result.get_start_address() << '|' << result.get_end_address() << '|'
                << (result.get_dmi_ptr() != nullptr) << '|' << result.get_read_latency() << '|'
                << result.get_write_latency();
    std::cout << '\n';
  }

  /** @brief Wait until the source initialization method disables DMI, then query. */
  void check_disabled()
  {
    wait(sc_core::SC_ZERO_TIME);
    for (sc_dt::uint64 address : {sc_dt::uint64(0), sc_dt::uint64(4096), sc_dt::uint64(4097)})
      check_one("disabled", address);
    sc_core::sc_stop();
  }
};
}  // namespace example_tlm_memory_edges
