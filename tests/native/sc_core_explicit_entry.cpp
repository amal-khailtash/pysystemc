/** @file sc_core_explicit_entry.cpp
 * @brief Native explicit argument count and nonzero return reference.
 * @details Native caller owns argv; original pointers remain borrowed during entry.
 */
#include <cstring>
#include <systemc>
/** @brief Compare copied entry arguments and original native argv. */
int sc_main(int argc, char** argv)
{
  if (argc != 4 || sc_core::sc_argc() != argc || std::strcmp(argv[1], "café") || std::strcmp(argv[2], "--flag") ||
      std::strcmp(argv[3], ""))
    return 2;
  argv[1][0] = 'X';
  if (std::strcmp(sc_core::sc_argv()[1], "café")) return 3;
  std::cout << "EXPLICIT_ENTRY_REFERENCE 4 café --flag \n";
  return 7;
}
