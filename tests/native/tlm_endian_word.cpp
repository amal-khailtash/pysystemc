/** @file Independent native word-converter type and offset reference. */
#include <iostream>
#include <stdexcept>
#include <systemc>
#include <tlm>
#include <vector>

/** @brief Compare native bytes and metadata across every bus offset.
 * @details Original vector storage outlives request/response conversion; payload
 * context owns temporary buffers. No callbacks or Python GIL are involved.
 */
template <class Word>
void check()
{
  for (unsigned int bus : {4u, 8u})
    for (unsigned int offset = 0; offset < bus; ++offset)
      for (auto command : {tlm::TLM_WRITE_COMMAND, tlm::TLM_READ_COMMAND})
      {
        const unsigned int length = sizeof(Word) * 2;
        std::vector<unsigned char> data(length);
        for (unsigned int i = 0; i < length; ++i) data[i] = i + 1;
        tlm::tlm_generic_payload payload;
        payload.resize_extensions();
        payload.set_command(command);
        payload.set_address(32 + offset);
        payload.set_data_ptr(data.data());
        payload.set_data_length(length);
        payload.set_streaming_width(length);
        tlm::tlm_to_hostendian_word<Word>(&payload, bus);
        const auto expanded = payload.get_data_length();
        if (payload.get_address() != 32 || expanded < length) throw std::runtime_error("word metadata mismatch");
        for (unsigned int i = 0; i < length; ++i)
        {
          const auto mapped =
            ((32 + offset + (i / sizeof(Word)) * sizeof(Word) + sizeof(Word) - 1 - i % sizeof(Word)) ^ (bus - 1)) - 32;
          if (mapped >= expanded || payload.get_byte_enable_ptr()[mapped] != 255)
            throw std::runtime_error("word byte mapping mismatch");
          if (command == tlm::TLM_WRITE_COMMAND && payload.get_data_ptr()[mapped] != i + 1)
            throw std::runtime_error("word write mismatch");
        }
        if (command == tlm::TLM_READ_COMMAND)
          for (unsigned int i = 0; i < expanded; ++i) payload.get_data_ptr()[i] = 50 + i;
        std::cout << "WORD " << sizeof(Word) << ' ' << bus << ' ' << offset << ' ' << static_cast<int>(command) << ' '
                  << expanded;
        for (unsigned int i = 0; i < expanded; ++i)
          std::cout << ' ' << static_cast<unsigned int>(payload.get_byte_enable_ptr()[i]);
        tlm::tlm_from_hostendian(&payload);
        for (unsigned int i = 0; i < length; ++i) std::cout << ' ' << static_cast<unsigned int>(data[i]);
        std::cout << '\n';
      }
}

/** @brief Run all 72 native request/response cases. */
int sc_main(int, char**)
{
  check<unsigned char>();
  check<unsigned short>();
  check<unsigned int>();
  return 0;
}
