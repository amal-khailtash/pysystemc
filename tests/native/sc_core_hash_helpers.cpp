/** @file sc_core_hash_helpers.cpp
 * @brief Native hash helper reference; duplicate keys are explicitly freed once.
 * @details Hash pointer values are never dereferenced. String inputs live through calls.
 */
#include <sysc/utils/sc_hash.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <systemc>
/** @brief Verify native integer/address/string hashes and allocated key lifetime. */
int sc_main(int, char**)
{
  using namespace sc_core;
  for (uintptr_t value : {uintptr_t(0), uintptr_t(1), uintptr_t(4), uintptr_t(17), uintptr_t(0x12345678)})
  {
    const void* p = reinterpret_cast<const void*>(value);
    std::cout << "HASH_REFERENCE " << value << " " << default_int_hash_fn(p) << " " << default_ptr_hash_fn(p) << "\n";
  }
  for (const char* key : {"", "alpha", "SystemC", "a.b_9"})
  {
    void* duplicate = sc_strhash_kdup(key);
    if (!duplicate || sc_strhash_cmp(key, duplicate) != 0) return 2;
    sc_strhash_kfree(duplicate);
    std::ostringstream encoded;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(key); *p; ++p)
      encoded << std::hex << std::setw(2) << std::setfill('0') << unsigned(*p);
    std::cout << "STRING_HASH_REFERENCE " << (encoded.str().empty() ? "empty" : encoded.str()) << " "
              << default_str_hash_fn(key) << "\n";
  }
  if (default_str_hash_fn(nullptr) != 0 || sc_strhash_cmp("a", "b") >= 0 || sc_strhash_cmp("same", "same") != 0 ||
      sc_strhash_cmp("z", "a") <= 0)
    return 3;
  std::cout << "HASH_COMPARE_REFERENCE -1 0 1 0\n";
  return 0;
}
