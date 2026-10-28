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
#include <type_traits>
#include <vector>

namespace pysystemc_public
{
/**
 * @brief Borrow writable storage for one printf count conversion.
 * @details Python retains the buffer or native pointer for the synchronous call.
 * The GIL stays held; the wrapper neither owns storage nor retains callbacks.
 * @throws std::invalid_argument If storage is null or not suitably aligned.
 */
template <typename Counter>
struct trace_count_argument
{
  Counter* pointer;
  Counter measurement = 0;

  /** @brief Adapt a borrowed buffer without cppyy's signed-char pointer conversion. */
  explicit trace_count_argument(void* storage) : pointer(static_cast<Counter*>(storage))
  {
    if (!storage || reinterpret_cast<std::uintptr_t>(storage) % alignof(Counter))
      throw std::invalid_argument("printf count storage must be nonnull and aligned");
  }

  /** @brief Borrow an address extracted from validated Python buffer storage.
   * @details Caller retains the buffer; the GIL stays held and no ownership transfers.
   */
  static trace_count_argument from_address(std::uintptr_t address)
  {
    return trace_count_argument(reinterpret_cast<void*>(address));
  }

  /** @brief Return the exact signed buffer element code, resolving native typedef aliases. */
  static const char* buffer_format()
  {
    if constexpr (std::is_same_v<Counter, signed char>) return "b";
    if constexpr (std::is_same_v<Counter, short>) return "h";
    if constexpr (std::is_same_v<Counter, int>) return "i";
    if constexpr (std::is_same_v<Counter, long>) return "l";
    return "q";
  }

  /** @brief Return the required native alignment for borrowed counter storage. */
  static std::size_t alignment()
  {
    return alignof(Counter);
  }
};

/** @brief Pass the exact native count-pointer type to printf, without copying storage. */
template <typename Counter>
Counter* trace_printf_arg(const trace_count_argument<Counter>& value)
{
  return value.pointer;
}

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

/** @brief Measure count conversions using private scratch storage, without caller writes. */
template <typename Counter>
Counter* trace_printf_measure_arg(trace_count_argument<Counter>& value)
{
  return &value.measurement;
}

/** @brief Preserve ordinary argument formatting during the size-only pass. */
template <typename T>
auto trace_printf_measure_arg(const T& value) -> decltype(trace_printf_arg(value))
{
  return trace_printf_arg(value);
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
  int size = std::snprintf(nullptr, 0, format.c_str(), trace_printf_measure_arg(args)...);
  if (size < 0) throw std::runtime_error("native trace comment formatting failed");
  std::vector<char> buffer(static_cast<std::size_t>(size) + 1);
  int written = std::snprintf(buffer.data(), buffer.size(), format.c_str(), trace_printf_arg(args)...);
  if (written != size) throw std::runtime_error("native trace comment formatting changed length");
  sc_core::sc_write_comment(file, std::string(buffer.data(), static_cast<std::size_t>(size)));
}
}  // namespace pysystemc_public
