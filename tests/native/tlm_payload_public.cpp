/** @file Independent native generic-payload field and ownership reference. */
#include <iostream>
#include <stdexcept>
#include <string>

#include "tlm_payload_probe.h"

/** @brief Verify native behavior even when SystemC assertions are disabled. */
static void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

int sc_main(int, char**)
{
  using E = payload_probe::Extension<137>;
  {
    tlm::tlm_generic_payload empty;
    verify(!empty.has_mm() && empty.get_ref_count() == 0, "default ownership");
    verify(empty.get_address() == 0 && empty.get_data_length() == 0 && empty.get_byte_enable_length() == 0,
           "default lengths");
    verify(!empty.get_data_ptr() && !empty.get_byte_enable_ptr(), "default pointers");
    verify(empty.get_command() == tlm::TLM_IGNORE_COMMAND && !empty.is_read() && !empty.is_write(), "default command");
    verify(!empty.is_dmi_allowed() && empty.get_streaming_width() == 0, "default streaming");
    verify(empty.get_response_status() == tlm::TLM_INCOMPLETE_RESPONSE && empty.is_response_error() &&
             !empty.is_response_ok() && empty.get_gp_option() == tlm::TLM_MIN_PAYLOAD,
           "default response");

    payload_probe::Fixture fixture;
    auto& source = fixture.source;
    auto& copy = fixture.copy;
    source.set_data_ptr(fixture.source_data);
    source.set_byte_enable_ptr(fixture.source_enable);
    copy.set_data_ptr(fixture.copy_data);
    copy.set_byte_enable_ptr(fixture.copy_enable);
    source.set_address(0xFEDCBA9876543210ULL);
    source.set_data_length(16);
    source.set_byte_enable_length(8);
    source.set_streaming_width(7);
    source.set_write();
    verify(source.is_write() && !source.is_read(), "write predicate");
    source.set_command(tlm::TLM_IGNORE_COMMAND);
    verify(!source.is_read() && !source.is_write(), "ignore predicates");
    source.set_read();
    source.set_dmi_allowed(true);
    source.set_gp_option(tlm::TLM_FULL_PAYLOAD);
    verify(source.get_data_ptr() == fixture.source_data && source.get_byte_enable_ptr() == fixture.source_enable,
           "borrowed pointers");
    const auto overflow = static_cast<unsigned int>(std::uint64_t(1) << 40);
    source.set_data_length(overflow);
    verify(source.get_data_length() == 0, "unsigned length conversion");
    std::cout << "PAYLOAD_UINT32|length|" << source.get_data_length() << '\n';
    source.set_data_length(16);
    source.set_byte_enable_length(overflow);
    verify(source.get_byte_enable_length() == 0, "unsigned enable conversion");
    std::cout << "PAYLOAD_UINT32|enable|" << source.get_byte_enable_length() << '\n';
    source.set_byte_enable_length(8);
    source.set_streaming_width(overflow);
    verify(source.get_streaming_width() == 0, "unsigned stream conversion");
    std::cout << "PAYLOAD_UINT32|stream|" << source.get_streaming_width() << '\n';
    source.set_streaming_width(7);
    const tlm::tlm_response_status statuses[] = {tlm::TLM_OK_RESPONSE,
                                                 tlm::TLM_INCOMPLETE_RESPONSE,
                                                 tlm::TLM_GENERIC_ERROR_RESPONSE,
                                                 tlm::TLM_ADDRESS_ERROR_RESPONSE,
                                                 tlm::TLM_COMMAND_ERROR_RESPONSE,
                                                 tlm::TLM_BURST_ERROR_RESPONSE,
                                                 tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE};
    const char* spellings[] = {"TLM_OK_RESPONSE",
                               "TLM_INCOMPLETE_RESPONSE",
                               "TLM_GENERIC_ERROR_RESPONSE",
                               "TLM_ADDRESS_ERROR_RESPONSE",
                               "TLM_COMMAND_ERROR_RESPONSE",
                               "TLM_BURST_ERROR_RESPONSE",
                               "TLM_BYTE_ENABLE_ERROR_RESPONSE"};
    for (unsigned i = 0; i < 7; ++i)
    {
      source.set_response_status(statuses[i]);
      verify(source.get_response_status() == statuses[i] && source.get_response_string() == spellings[i], "response");
      verify(source.is_response_ok() == (i == 0) && source.is_response_error() == (i != 0), "response predicates");
      std::cout << "PAYLOAD_STATUS|" << static_cast<int>(statuses[i]) << '|' << spellings[i] << '\n';
    }
    source.set_response_status(tlm::TLM_OK_RESPONSE);

    auto* extension = new E(7);
    source.resize_extensions();
    verify(!source.set_extension<E>(extension), "initial extension");
    verify(source.get_extension(E::ID) == extension && source.get_extension<E>() == extension, "typed extension");
    E* output = nullptr;
    source.get_extension(output);
    verify(output == extension, "output extension");
    copy.deep_copy_from(source);
    auto* cloned = copy.get_extension<E>();
    verify(cloned && cloned != extension && cloned->value[136] && cloned->value.to_uint64() == 7 && E::clones == 1,
           "extension clone");
    verify(copy.get_address() == source.get_address() && copy.get_data_length() == 16 &&
             copy.get_byte_enable_length() == 8 && copy.get_streaming_width() == 7 && copy.is_read() &&
             copy.is_dmi_allowed() && copy.get_gp_option() == tlm::TLM_FULL_PAYLOAD,
           "deep fields");
    for (unsigned i = 0; i < 16; ++i) verify(fixture.copy_data[i] == i + 1, "deep bytes");
    for (unsigned i = 0; i < 8; ++i) verify(fixture.copy_enable[i] == (i % 2 ? 0 : 255), "deep enables");
    fixture.copy_data[0] = 77;
    verify(fixture.source_data[0] == 1, "independent buffers");
    copy.deep_copy_from(source);
    verify(E::clones == 1 && E::copies == 1, "existing extension copy");
    cloned->value = E(9).value;
    source.update_extensions_from(copy);
    verify(extension->value.to_uint64() == 9 && extension->value[136] && E::copies == 2, "extension update");

    for (unsigned enable_length : {8U, 4U, 3U})
    {
      source.set_byte_enable_length(enable_length);
      for (unsigned i = 0; i < 16; ++i)
      {
        fixture.source_data[i] = static_cast<unsigned char>(i + 1);
        fixture.copy_data[i] = static_cast<unsigned char>(100 + i);
      }
      copy.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
      copy.set_dmi_allowed(false);
      source.update_original_from(copy);
      verify(source.get_response_status() == tlm::TLM_ADDRESS_ERROR_RESPONSE && !source.is_dmi_allowed(),
             "response update");
      std::cout << "PAYLOAD_MASK|" << enable_length << '|';
      for (unsigned i = 0; i < 16; ++i)
      {
        const unsigned expected = (i % enable_length) % 2 ? i + 1 : 100 + i;
        verify(fixture.source_data[i] == expected, "masked read");
        if (i) std::cout << ',';
        std::cout << static_cast<unsigned>(fixture.source_data[i]);
      }
      std::cout << '\n';
      source.update_original_from(copy, false);
      for (unsigned i = 0; i < 16; ++i) verify(fixture.source_data[i] == 100 + i, "unmasked read");
    }
    source.set_write();
    fixture.source_data[0] = 88;
    source.update_original_from(copy, false);
    verify(fixture.source_data[0] == 88, "write data remains original");
    source.clear_extension(extension);
    verify(!source.get_extension<E>() && extension->value.to_uint64() == 9, "clear borrows");
    verify(!source.set_extension(E::ID, extension), "indexed extension");
    source.clear_extension<E>();
    extension->free();
    copy.free_all_extensions();
    verify(!copy.get_extension<E>() && E::live == 0 && E::frees == 2, "free all");
    for (bool pointer : {true, false})
    {
      auto* value = new E(31);
      source.set_extension(value);
      if (pointer)
        source.release_extension(value);
      else
        source.release_extension<E>();
      verify(!source.get_extension<E>(), "immediate release");
    }
    auto* previous = new E(51);
    auto* replacement = new E(52);
    verify(!source.set_extension(previous) && source.set_extension(replacement) == previous, "replacement returns old");
    previous->free();
    source.release_extension<E>();
    verify(E::live == 0 && E::frees == 6, "replacement release");
    source.set_mm(&fixture.manager);
    tlm::tlm_generic_payload managed(&fixture.manager);
    verify(source.has_mm() && managed.has_mm() && managed.get_ref_count() == 0, "manager constructor");
    for (bool indexed : {false, true})
    {
      auto* value = new E(41);
      verify(!(indexed ? source.set_auto_extension(E::ID, value) : source.set_auto_extension(value)), "auto extension");
      source.set_gp_option(tlm::TLM_FULL_PAYLOAD_ACCEPTED);
      source.acquire();
      source.acquire();
      verify(source.get_ref_count() == 2, "acquire");
      source.release();
      verify(source.get_ref_count() == 1, "retained release");
      source.release();
      verify(source.get_ref_count() == 0 && fixture.manager.last == &source && !source.get_extension<E>() &&
               source.get_gp_option() == tlm::TLM_MIN_PAYLOAD,
             "pooled release");
    }
    verify(fixture.manager.releases == 2 && E::frees == 8, "manager calls");
    for (bool pointer : {true, false})
    {
      auto* value = new E(61);
      source.set_extension(value);
      if (pointer)
        source.release_extension(value);
      else
        source.release_extension<E>();
      verify(source.get_extension<E>() == value && E::live == 1, "scheduled release");
      source.reset();
      verify(!source.get_extension<E>() && E::live == 0, "scheduled reset");
    }
    source.set_mm(nullptr);
    source.set_gp_option(tlm::TLM_FULL_PAYLOAD);
    source.reset();
    verify(!source.has_mm() && source.get_gp_option() == tlm::TLM_MIN_PAYLOAD &&
             source.get_address() == 0xFEDCBA9876543210ULL && source.get_data_length() == 16,
           "reset retains fields");
    auto* context = new tlm::tlm_endian_context;
    verify(!source.set_extension(context) && source.get_extension<tlm::tlm_endian_context>() == context,
           "endian specialization");
    source.clear_extension(context);
    context->free();
    source.set_data_ptr(nullptr);
    source.set_byte_enable_ptr(nullptr);
    empty.deep_copy_from(source);
    verify(empty.get_address() == source.get_address() && empty.get_data_length() == 16 && !empty.get_data_ptr() &&
             !empty.get_byte_enable_ptr(),
           "null deep copy");
    empty.update_original_from(copy);
    verify(empty.get_response_status() == copy.get_response_status(), "null response update");
    empty.set_extension(new E(71));
    verify(E::live == 1, "destructor ownership");
  }
  verify(E::live == 0 && E::frees == 11, "payload destructor frees extension");
  std::cout << "PAYLOAD_LIFETIME|1|11|2\nTLM_PAYLOAD_PUBLIC_OK\n";
  return 0;
}
