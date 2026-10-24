/** @file sc_core_public_streams.cpp
 * @brief Native reference for SystemC status, time, channel and port stream operators.
 * @details SystemC owns all modules and channels; stream operators borrow their
 * arguments and mutate only the output stream. No Python callbacks or GIL
 * operations occur, and native ios failures are caught by the reference.
 */
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "sc_core_public_streams_probe.h"

namespace
{

/** @brief Stream values and return a flattened copy of the result. */
template <class Stream, class... Values>
std::string render(Stream& stream, const Values&... values)
{
  (static_cast<void>(stream << values), ...);
  return sc_core_public_streams_probe::flatten(stream.str());
}

/** @brief Emit one native/Python comparison row. */
void record(const char* label, const std::string& value)
{
  std::cout << "STREAM\t" << label << '\t' << value << '\n';
}

}  // namespace

/** @brief Compare native free ostream operators with their Python adapters. */
int sc_main(int, char**)
{
  using sc_core_public_streams_probe::Payload;
  using sc_core_public_streams_probe::Ports;

  std::ostringstream status_stream;
  record("status", render(status_stream, "status=", sc_core::SC_RUNNING, "|", sc_core::SC_STOPPED, "|", 41));

  const sc_core::sc_time duration(12, sc_core::SC_NS);
  std::ostringstream time_stream;
  record("time", render(time_stream, "time=", duration, "|", 41));

  constexpr const char* wide_sample = "87112285931760246646623899502532662132737";
  const sc_dt::sc_biguint<137> wide_value(wide_sample);
  sc_core::sc_signal<bool> bool_signal("bool_signal", true);
  sc_core::sc_signal<bool> bool_inout_signal("bool_inout_signal", false);
  sc_core::sc_signal<bool> bool_output_signal("bool_output_signal", true);
  sc_core::sc_signal<sc_dt::sc_biguint<137>> wide_signal("wide_signal", wide_value);
  sc_core::sc_buffer<Payload> payload_buffer("payload_buffer", Payload(23));

  Ports ports("stream_ports");
  ports.bool_input(bool_signal);
  ports.bool_inout(bool_inout_signal);
  ports.bool_output(bool_output_signal);
  ports.wide_input(wide_signal);
  ports.wide_inout(wide_signal);
  ports.payload_input(payload_buffer);
  ports.payload_inout(payload_buffer);

  sc_core::sc_start(sc_core::SC_ZERO_TIME);

  std::ostringstream channel_stream;
  record("channels", render(channel_stream, "channels=", bool_signal, "|", wide_signal, "|", payload_buffer, "|", 41));

  std::ostringstream port_stream;
  record("ports",
         render(port_stream, "ports=", ports.bool_input, "|", ports.bool_inout, "|", ports.bool_output, "|",
                ports.wide_input, "|", ports.wide_inout, "|", ports.payload_input, "|", ports.payload_inout, "|", 41));

  sc_core_public_streams_probe::LateOstream late_stream;
  record("derived", render(late_stream, "derived=", sc_core::SC_END_OF_INITIALIZATION, "|", wide_signal, "|", 137));

  if (bool_signal.read() != true || bool_inout_signal.read() != false || bool_output_signal.read() != true ||
      wide_signal.read() != wide_value || payload_buffer.read().value != 23 || ports.bool_input.read() != true ||
      ports.bool_inout.read() != false || ports.bool_output.read() != true || ports.wide_input.read() != wide_value ||
      ports.wide_inout.read() != wide_value || ports.payload_input.read().value != 23 ||
      ports.payload_inout.read().value != 23)
    throw std::runtime_error("stream insertion changed or misread a channel value");

  std::ostringstream failed;
  try
  {
    sc_core_public_streams_probe::fail_stream(failed);
  }
  catch (const std::ios_base::failure&)
  {
  }
  try
  {
    failed << bool_signal;
    throw std::runtime_error("failed stream accepted signal insertion");
  }
  catch (const std::ios_base::failure& error)
  {
    const std::string message(error.what());
    const std::size_t separator = message.rfind(": ");
    record("error", separator == std::string::npos ? message : message.substr(separator + 2));
  }
  if (bool_signal.read() != true || payload_buffer.read().value != 23)
    throw std::runtime_error("failed stream insertion changed channel data");

  sc_core::sc_stop();
  return 0;
}
