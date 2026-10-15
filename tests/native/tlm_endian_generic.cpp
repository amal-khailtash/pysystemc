/** @file Independent unaligned streaming endian reference with repeating enables. */
#include <iostream>
#include <stdexcept>
#include <systemc>
#include <tlm>

/** @brief Print actual byte storage for cross-language semantic comparison. */
void record(const char* label, const unsigned char* data, unsigned int length)
{
  std::cout << "GENERIC " << label;
  for (unsigned int i = 0; i < length; ++i) std::cout << ' ' << static_cast<unsigned int>(data[i]);
  std::cout << '\n';
}

/** @brief Convert borrowed stack buffers; payload context owns temporary buffers.
 * @details No Python callbacks or GIL requirements. Original storage outlives
 * conversion and response copying; unconditional checks work under NDEBUG.
 */
int sc_main(int, char**)
{
  for (auto command : {tlm::TLM_WRITE_COMMAND, tlm::TLM_READ_COMMAND})
  {
    unsigned char data[8] = {99, 99, 99, 99, 99, 99, 99, 99};
    unsigned char enables[2] = {255, 0};
    tlm::tlm_generic_payload payload;
    payload.resize_extensions();
    payload.set_command(command);
    payload.set_address(17);
    payload.set_data_ptr(data);
    payload.set_data_length(8);
    payload.set_streaming_width(4);
    payload.set_byte_enable_ptr(enables);
    payload.set_byte_enable_length(2);
    tlm::tlm_to_hostendian_generic<unsigned short>(&payload, 4);
    if (payload.get_address() != 16 || payload.get_data_length() != 16 || payload.get_streaming_width() != 8)
      throw std::runtime_error("generic endian metadata mismatch");
    record("enables", payload.get_byte_enable_ptr(), 16);
    if (command == tlm::TLM_READ_COMMAND)
    {
      for (unsigned int i = 0; i < 16; ++i) payload.get_data_ptr()[i] = 40 + i;
    }
    else
    {
      for (unsigned int i : {1u, 7u, 9u, 15u})
        if (payload.get_data_ptr()[i] != 99) throw std::runtime_error("enabled write mismatch");
    }
    tlm::tlm_from_hostendian(&payload);
    const unsigned char expected[8] = {41, 99, 47, 99, 49, 99, 55, 99};
    for (unsigned int i = 0; i < 8; ++i)
      if (data[i] != (command == tlm::TLM_READ_COMMAND ? expected[i] : 99))
        throw std::runtime_error("generic endian response mismatch");
    record("response", data, 8);
    record("source-enables", enables, 2);
  }
  return 0;
}
