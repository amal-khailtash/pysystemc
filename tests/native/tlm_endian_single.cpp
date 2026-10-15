/** @file Independent single-word endian conversion reference. */
#include <iostream>
#include <stdexcept>
#include <systemc>
#include <tlm>

/** @brief Check native address lane conversion and response dispatch.
 * @details Payload owns its context, has no data buffers, and stays alive through
 * both conversions. No Python callbacks or GIL requirements apply.
 */
template <class Word>
void check()
{
  for (unsigned int bus : {4u, 8u, 16u})
  {
    for (sc_dt::uint64 address : {0u, 16u, 32u})
    {
      tlm::tlm_generic_payload payload;
      payload.resize_extensions();
      payload.set_address(address);
      payload.set_command(tlm::TLM_READ_COMMAND);
      payload.set_data_length(sizeof(Word));
      payload.set_streaming_width(sizeof(Word));
      tlm::tlm_to_hostendian_single<Word>(&payload, bus);
      const auto expected = address + bus - sizeof(Word);
      if (payload.get_address() != expected) throw std::runtime_error("endian address mismatch");
      payload.set_response_status(tlm::TLM_OK_RESPONSE);
      tlm::tlm_from_hostendian(&payload);
      if (payload.get_address() != expected || !payload.is_response_ok())
        throw std::runtime_error("endian response mismatch");
      tlm::tlm_from_hostendian_single<Word>(&payload, bus);
      if (payload.get_address() != expected || !payload.is_response_ok())
        throw std::runtime_error("explicit endian response mismatch");
      std::cout << "ENDIAN " << sizeof(Word) << ' ' << bus << ' ' << address << ' ' << expected << '\n';
    }
  }
}

/** @brief Execute independent native template combinations. */
int sc_main(int, char**)
{
  check<unsigned char>();
  check<unsigned short>();
  check<unsigned int>();
  return 0;
}
