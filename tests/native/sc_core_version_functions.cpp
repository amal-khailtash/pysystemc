/** @file sc_core_version_functions.cpp
 * @brief Compare copied native static strings; no callbacks or ownership transfer.
 */
#include <iomanip>
#include <systemc>
/** @brief Encode multiline strings as comparable hexadecimal records. */
void record(const char* name, const char* value)
{
  std::cout << "VERSION_FUNCTION_REFERENCE " << name << " ";
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value); *p; ++p)
    std::cout << std::hex << std::setw(2) << std::setfill('0') << unsigned(*p);
  std::cout << "\n";
}
/** @brief Emit exact values of all public native version string functions. */
int sc_main(int, char**)
{
  record("sc_copyright", sc_core::sc_copyright());
  record("sc_release", sc_core::sc_release());
  record("sc_version", sc_core::sc_version());
  return 0;
}
