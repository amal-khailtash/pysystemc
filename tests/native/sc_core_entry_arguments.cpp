/** @file sc_core_entry_arguments.cpp
 * @brief Native original argv versus copied entry buffer reference.
 * @details Argument arrays are borrowed only during entry, with no callbacks.
 */
#include <cstring>
#include <systemc>
/** @brief Verify argc, UTF-8 arguments and mutation isolation. */
int sc_main(int argc, char** argv)
{
  const auto* original = sc_core::sc_argv();
  if (argc != 4 || sc_core::sc_argc() != argc || std::strcmp(original[1], "café") ||
      std::strcmp(original[2], "--flag") || std::strcmp(original[3], ""))
    return 2;
  argv[1][0] = 'X';
  if (std::strcmp(original[1], "café")) return 3;
  std::cout << "ENTRY_ARGUMENT_REFERENCE " << argc << " " << original[1] << " " << original[2] << " " << original[3]
            << "\n";
  return 0;
}
