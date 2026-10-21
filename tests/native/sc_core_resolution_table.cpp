/** @file sc_core_resolution_table.cpp
 * @brief Independent native logic-resolution table and configuration reads.
 * @details Read library-owned const table without mutation or ownership transfer.
 */
#include <systemc>
/** @brief Emit every native cell and the initial public boolean globals. */
int sc_main(int, char**)
{
  for (int left = 0; left < 4; ++left)
    for (int right = 0; right < 4; ++right)
      std::cout << "RESOLUTION_CELL " << left << " " << right << " "
                << int(sc_core::sc_logic_resolution_tbl[left][right]) << "\n";
  std::cout << "CORE_GLOBAL_READS " << int(sc_core::sc_allow_process_control_corners) << " "
            << int(sc_core::sc_enable_name_checking) << "\n";
  return 0;
}
