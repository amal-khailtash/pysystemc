/**
 * @file trace_printf.cpp
 * @brief Restore native formatted trace comments omitted by cppyy varargs reflection.
 */
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

namespace pysystemc_public
{
/** @brief Borrow narrow string data for a single formatting call with GIL held. */
inline const char* trace_printf_arg(const std::string& value)
{
  return value.c_str();
}
/** @brief Borrow wide string data for a single formatting call with GIL held. */
inline const wchar_t* trace_printf_arg(const std::wstring& value)
{
  return value.c_str();
}
/** @brief Preserve caller-selected native arithmetic or pointer printf argument types. */
template <typename T>
const T& trace_printf_arg(const T& value)
{
  return value;
}

/**
 * @brief Format a trace comment with native printf conversions and dynamic storage.
 * @param file Borrowed open native trace, or nullptr for native no-op behavior.
 * @param format Borrowed C printf format; argument types must match its conversions.
 * @param args Borrowed native argument values, retained by the Python caller.
 * @details The GIL remains held; no process callbacks or scheduler operations occur.
 * Buffer storage is owned for this call and released on every exit. Unlike the
 * native fixed 4096-byte buffer, formatting uses the exact required buffer size.
 * @throws std::runtime_error If native formatting fails.
 */
template <typename... Args>
void trace_printf(sc_core::sc_trace_file* file, const std::string& format, Args... args)
{
  int size = std::snprintf(nullptr, 0, format.c_str(), trace_printf_arg(args)...);
  if (size < 0) throw std::runtime_error("native trace comment formatting failed");
  std::vector<char> buffer(static_cast<std::size_t>(size) + 1);
  int written = std::snprintf(buffer.data(), buffer.size(), format.c_str(), trace_printf_arg(args)...);
  if (written != size) throw std::runtime_error("native trace comment formatting changed length");
  sc_core::sc_write_comment(file, std::string(buffer.data(), static_cast<std::size_t>(size)));
}
}  // namespace pysystemc_public
