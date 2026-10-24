/** @file tlm_namespace_public.cpp
 * @brief Native reference for TLM namespace values and registry semantics.
 * @details Constants are static native storage; arrays own pointer slots but
 * borrow pointees. No Python callbacks, scheduler entry or GIL operations occur.
 */
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include "tlm_namespace_probe.h"
/** @brief Encode native values without allowing embedded newlines in records. */
template <class Value>
std::string text(const Value& value)
{
  std::ostringstream stream;
  stream << value;
  std::string result;
  for (char c : stream.str())
  {
    if (c == '\n')
      result += "\\n";
    else if (c == '\r')
      result += "\\r";
    else if (c == '\\')
      result += "\\\\";
    else
      result += c;
  }
  return result;
}
/** @brief Emit native values, host predicates and registry deltas. */
int sc_main(int, char**)
{
  std::cout << "TLM_NAMESPACE|BEGIN_REQ|" << text(tlm::BEGIN_REQ) << "\n";
  std::cout << "TLM_NAMESPACE|BEGIN_RESP|" << text(tlm::BEGIN_RESP) << "\n";
  std::cout << "TLM_NAMESPACE|END_REQ|" << text(tlm::END_REQ) << "\n";
  std::cout << "TLM_NAMESPACE|END_RESP|" << text(tlm::END_RESP) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_ACCEPTED|" << text(tlm::TLM_ACCEPTED) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_ADDRESS_ERROR_RESPONSE|" << text(tlm::TLM_ADDRESS_ERROR_RESPONSE) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_BIG_ENDIAN|" << text(tlm::TLM_BIG_ENDIAN) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_BURST_ERROR_RESPONSE|" << text(tlm::TLM_BURST_ERROR_RESPONSE) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_BYTE_ENABLE_ERROR_RESPONSE|" << text(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_COMMAND_ERROR_RESPONSE|" << text(tlm::TLM_COMMAND_ERROR_RESPONSE) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_COMPLETED|" << text(tlm::TLM_COMPLETED) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_FULL_PAYLOAD|" << text(tlm::TLM_FULL_PAYLOAD) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_FULL_PAYLOAD_ACCEPTED|" << text(tlm::TLM_FULL_PAYLOAD_ACCEPTED) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_GENERIC_ERROR_RESPONSE|" << text(tlm::TLM_GENERIC_ERROR_RESPONSE) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_IGNORE_COMMAND|" << text(tlm::TLM_IGNORE_COMMAND) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_INCOMPLETE_RESPONSE|" << text(tlm::TLM_INCOMPLETE_RESPONSE) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_INITIATOR_SOCKET|" << text(tlm::TLM_INITIATOR_SOCKET) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_LITTLE_ENDIAN|" << text(tlm::TLM_LITTLE_ENDIAN) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_MIN_PAYLOAD|" << text(tlm::TLM_MIN_PAYLOAD) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_MULTI_INITIATOR_SOCKET|" << text(tlm::TLM_MULTI_INITIATOR_SOCKET) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_MULTI_SOCKET|" << text(tlm::TLM_MULTI_SOCKET) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_MULTI_TARGET_SOCKET|" << text(tlm::TLM_MULTI_TARGET_SOCKET) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_OK_RESPONSE|" << text(tlm::TLM_OK_RESPONSE) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_READ_COMMAND|" << text(tlm::TLM_READ_COMMAND) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_TARGET_SOCKET|" << text(tlm::TLM_TARGET_SOCKET) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_UNKNOWN_ENDIAN|" << text(tlm::TLM_UNKNOWN_ENDIAN) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_UNKNOWN_SOCKET|" << text(tlm::TLM_UNKNOWN_SOCKET) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_UPDATED|" << text(tlm::TLM_UPDATED) << "\n";
  std::cout << "TLM_NAMESPACE|TLM_WRITE_COMMAND|" << text(tlm::TLM_WRITE_COMMAND) << "\n";
  std::cout << "TLM_NAMESPACE|UNINITIALIZED_PHASE|" << text(tlm::UNINITIALIZED_PHASE) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_copyright_string|" << text(tlm::tlm_copyright_string) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_is_prerelease|" << text(tlm::tlm_is_prerelease) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_version_major|" << text(tlm::tlm_version_major) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_version_minor|" << text(tlm::tlm_version_minor) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_version_originator|" << text(tlm::tlm_version_originator) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_version_patch|" << text(tlm::tlm_version_patch) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_version_prerelease|" << text(tlm::tlm_version_prerelease) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_version_release_date|" << text(tlm::tlm_version_release_date) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_version_string|" << text(tlm::tlm_version_string) << "\n";
  std::cout << "TLM_NAMESPACE|tlm_version_string_2|" << text(tlm::tlm_version_string_2) << "\n";
  static_assert(std::is_same_v<decltype(tlm::TLM_IGNORE_COMMAND), tlm::tlm_command>);
  std::cout << "TLM_ENUM|tlm_command|" << static_cast<int>(tlm::TLM_IGNORE_COMMAND) << "|1\n";
  static_assert(std::is_same_v<decltype(tlm::TLM_BIG_ENDIAN), tlm::tlm_endianness>);
  std::cout << "TLM_ENUM|tlm_endianness|" << static_cast<int>(tlm::TLM_BIG_ENDIAN) << "|1\n";
  static_assert(std::is_same_v<decltype(tlm::TLM_FULL_PAYLOAD), tlm::tlm_gp_option>);
  std::cout << "TLM_ENUM|tlm_gp_option|" << static_cast<int>(tlm::TLM_FULL_PAYLOAD) << "|1\n";
  static_assert(std::is_same_v<decltype(tlm::BEGIN_REQ), tlm::tlm_phase_enum>);
  std::cout << "TLM_ENUM|tlm_phase_enum|" << static_cast<int>(tlm::BEGIN_REQ) << "|1\n";
  static_assert(std::is_same_v<decltype(tlm::TLM_ADDRESS_ERROR_RESPONSE), tlm::tlm_response_status>);
  std::cout << "TLM_ENUM|tlm_response_status|" << static_cast<int>(tlm::TLM_ADDRESS_ERROR_RESPONSE) << "|1\n";
  static_assert(std::is_same_v<decltype(tlm::TLM_INITIATOR_SOCKET), tlm::tlm_socket_category>);
  std::cout << "TLM_ENUM|tlm_socket_category|" << static_cast<int>(tlm::TLM_INITIATOR_SOCKET) << "|1\n";
  static_assert(std::is_same_v<decltype(tlm::TLM_ACCEPTED), tlm::tlm_sync_enum>);
  std::cout << "TLM_ENUM|tlm_sync_enum|" << static_cast<int>(tlm::TLM_ACCEPTED) << "|1\n";
  std::cout << "TLM_FUNCTION|get_host_endianness|" << text(tlm::get_host_endianness()) << "\n";
  std::cout << "TLM_FUNCTION|host_has_little_endianness|" << text(tlm::host_has_little_endianness()) << "\n";
  std::cout << "TLM_FUNCTION|tlm_copyright|" << text(tlm::tlm_copyright()) << "\n";
  std::cout << "TLM_FUNCTION|tlm_release|" << text(tlm::tlm_release()) << "\n";
  std::cout << "TLM_FUNCTION|tlm_version|" << text(tlm::tlm_version()) << "\n";
  for (auto endian : {tlm::TLM_UNKNOWN_ENDIAN, tlm::TLM_LITTLE_ENDIAN, tlm::TLM_BIG_ENDIAN})
    std::cout << "TLM_HOST|" << static_cast<int>(endian) << "|" << tlm::has_host_endianness(endian) << "\n";
  auto before = tlm::max_num_extensions();
  auto id = tlm_namespace_probe::Registry::register_type();
  auto after = tlm::max_num_extensions();
  if (id != before || after != before + 1 || tlm_namespace_probe::Registry::register_type() != id ||
      tlm::max_num_extensions() != after)
    return 1;
  std::cout << "TLM_REGISTRY|1|1|1\n";
  tlm::tlm_array<tlm::tlm_extension_base*> array(2);
  array.expand(5);
  array.expand(3);
  if (array.size() != 5 || tlm_namespace_probe::null_slots(array) != 5) return 2;
  std::cout << "TLM_ARRAY|" << array.kind() << "|" << array.size() << "|" << tlm_namespace_probe::null_slots(array)
            << "\n";
  return 0;
}
