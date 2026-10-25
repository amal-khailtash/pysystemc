/** @file tlm_endian_public.cpp
 * @brief Independent native reference for endian buffer conversion families.
 * @details Data/enable vectors outlive both conversions; payloads own their
 * contexts and borrow original buffers. Calls do not suspend or require GIL.
 */
#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <systemc>
#include <tlm>
#include <vector>

#include "tlm_endian_probe.h"

/** @brief Encode owned byte observations without signed-char formatting. */
static std::string bytes(const std::vector<unsigned char>& values)
{
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    if (i) out << ',';
    out << unsigned(values[i]);
  }
  return out.str();
}
/** @brief Exercise each native converter for a freely instantiated word type. */
template <class Word>
void check_word()
{
  using Convert = void (*)(tlm::tlm_generic_payload*, unsigned int);
  const std::array<Convert, 4> to = {tlm::tlm_to_hostendian_generic<Word>, tlm::tlm_to_hostendian_word<Word>,
                                     tlm::tlm_to_hostendian_aligned<Word>, tlm::tlm_to_hostendian_single<Word>};
  const std::array<Convert, 4> from = {tlm::tlm_from_hostendian_generic<Word>, tlm::tlm_from_hostendian_word<Word>,
                                       tlm::tlm_from_hostendian_aligned<Word>, tlm::tlm_from_hostendian_single<Word>};
  const std::array<const char*, 4> names = {"generic", "word", "aligned", "single"};
  constexpr unsigned size = sizeof(Word);
  for (unsigned algorithm = 0; algorithm < 4; ++algorithm)
    for (unsigned bus : {4u, 8u, 32u})
    {
      if (size > bus) continue;
      for (auto command : {tlm::TLM_WRITE_COMMAND, tlm::TLM_READ_COMMAND})
        for (bool masked : {false, true})
          for (bool dispatcher : {false, true})
          {
            const sc_dt::uint64 base = 128;
            auto address = base;
            if (algorithm == 3 && size * 2 <= bus) address += size;
            if (algorithm <= 1) ++address;
            unsigned length = algorithm == 3 ? size : bus * 2;
            unsigned stream = algorithm == 0 ? bus : length;
            unsigned enable_length = algorithm == 0 ? size * 2 : length;
            std::vector<unsigned char> data(length), enables(enable_length);
            for (unsigned i = 0; i < length; ++i) data[i] = i + 3;
            for (unsigned i = 0; i < enable_length; ++i) enables[i] = (i / size % 2 == 0 && algorithm != 3) ? 255 : 0;
            const auto original_data = data;
            const auto original_enables = enables;
            tlm::tlm_generic_payload payload;
            payload.resize_extensions();
            payload.set_command(command);
            payload.set_address(address);
            payload.set_data_ptr(data.data());
            payload.set_data_length(length);
            payload.set_streaming_width(stream);
            if (masked)
            {
              payload.set_byte_enable_ptr(enables.data());
              payload.set_byte_enable_length(enable_length);
            }
            to[algorithm](&payload, bus);
            const auto expanded = payload.get_data_length();
            auto* converted = payload.get_data_ptr();
            auto* flags = payload.get_byte_enable_ptr();
            std::vector<unsigned char> converted_flags;
            if (flags) converted_flags.assign(flags, flags + payload.get_byte_enable_length());
            std::vector<unsigned char> observed;
            for (unsigned i = 0; i < expanded; ++i)
            {
              if (flags && flags[i % converted_flags.size()] != 255) continue;
              if (command == tlm::TLM_READ_COMMAND) converted[i] = (80 + i) % 256;
              observed.push_back(converted[i]);
            }
            const bool same_pointer = converted == data.data();
            if (algorithm == 3 &&
                (payload.get_address() != (address ^ (bus - size)) || !same_pointer || expanded != size))
              throw std::runtime_error("single-word metadata mismatch");
            if (algorithm == 2 && size == bus && !same_pointer) throw std::runtime_error("equal-width pointer changed");
            auto converted_address = payload.get_address() + (sc_dt::uint64(1) << 63);
            auto converted_stream = payload.get_streaming_width();
            if (dispatcher)
              tlm::tlm_from_hostendian(&payload);
            else
              from[algorithm](&payload, bus);
            if (enables != original_enables || (command == tlm::TLM_WRITE_COMMAND && data != original_data))
              throw std::runtime_error("original request storage mutated");
            std::cout << "ENDIAN_PUBLIC|" << names[algorithm] << '|' << size << '|' << bus << '|' << int(command) << '|'
                      << masked << '|' << dispatcher << '|' << converted_address << '|' << expanded << '|'
                      << converted_stream << '|' << same_pointer << '|' << bytes(converted_flags) << '|'
                      << bytes(observed) << '|' << bytes(data) << '\n';
          }
    }
}
/** @brief Compare context reuse against native conversion in bus-relative windows.
 * @details Native 2.3.4 masks generic request addresses at unsigned-int width.
 * Use equivalent low addresses here and report their original high windows;
 * original buffer response bytes are independent of the absolute bus anchor.
 */
static void check_reuse()
{
  tlm::tlm_generic_payload payload;
  payload.resize_extensions();
  const tlm::tlm_endian_context* first_context = nullptr;
  for (sc_dt::uint64 address : {(sc_dt::uint64(1) << 32) - 1, (sc_dt::uint64(1) << 32) + 1,
                                (sc_dt::uint64(1) << 63) + 131, ~sc_dt::uint64(0) - 64})
  {
    std::vector<unsigned char> data(16, 11), enables{255, 0};
    payload.set_command(tlm::TLM_READ_COMMAND);
    payload.set_address(address & 7);
    payload.set_data_ptr(data.data());
    payload.set_data_length(16);
    payload.set_streaming_width(8);
    payload.set_byte_enable_ptr(enables.data());
    payload.set_byte_enable_length(2);
    tlm::tlm_to_hostendian_generic<unsigned short>(&payload, 8);
    auto* context = payload.get_extension<tlm::tlm_endian_context>();
    if (!first_context) first_context = context;
    if (context != first_context) throw std::runtime_error("endian context replaced during reuse");
    for (unsigned index = 0; index < payload.get_data_length(); ++index)
      payload.get_data_ptr()[index] = (100 + index) % 256;
    tlm::tlm_from_hostendian(&payload);
    if (enables != std::vector<unsigned char>{255, 0}) throw std::runtime_error("original enables mutated");
    std::cout << "ENDIAN_REUSE|" << address << '|' << bytes(data) << '\n';
  }
}

/** @brief Run all converter/type cases under one independent native kernel. */
int sc_main(int, char**)
{
  check_word<unsigned char>();
  check_word<unsigned short>();
  check_word<unsigned int>();
  check_word<unsigned long long>();
  check_word<EndianWord16>();
  check_reuse();
  std::cout << "ENDIAN_PUBLIC_OK 384\n";
  return 0;
}
