/** @file Native reference execution for exact SystemC comparison overloads.
 * @details Each selected call borrows both operands from a live native fixture,
 *          copies the bool result immediately, and tears the fixture down before
 *          the next case. No callbacks or Python state are retained.
 */
#include <iostream>
#include <string>

#include "sc_dt_comparisons_probe.h"

int sc_main(int argc, char **argv)
{
  if (argc != 2) return 2;
  const std::string operation(argv[1]);
  const char *ops[] = {"lt", "le", "gt", "ge", "eq", "ne"};
  bool found = false;
  unsigned probe_count = 0;
  for (const char *op : ops)
  {
    if (operation != "all" && operation != op) continue;
    found = true;
    const unsigned count = sc_dt_comparisons_probe::native_case_count(op);
    probe_count += count;
    for (unsigned index = 0; index < count; ++index)
    {
      sc_dt_comparisons_probe::Sources values;
      const bool result = sc_dt_comparisons_probe::native_case(op, index, values);
      std::cout << "CMP\t" << op << '\t' << index << '\t' << int(result) << '\n';
    }
    if (std::string(op) == "eq")
    {
      std::cout << "EXTRA\tprecision\t" << int(sc_dt_comparisons_probe::precision_greater()) << '\t'
                << int(sc_dt_comparisons_probe::precision_equal()) << '\t'
                << int(sc_dt_comparisons_probe::precision_not_equal()) << '\n';
      std::cout << "EXTRA\tvector\t" << int(sc_dt_comparisons_probe::xz_equal()) << '\t'
                << int(sc_dt_comparisons_probe::xz_different()) << '\t' << int(sc_dt_comparisons_probe::width_equal())
                << '\n';
      std::cout << "EXTRA\tlifetime\t" << int(sc_dt_comparisons_probe::concat_lifetime_result()) << '\n';
      std::cout << "EXTRA\tinvalid\t" << sc_dt_comparisons_probe::native_invalid_bit_error() << '\n';
    }
  }
  if (!found) return 2;
  std::cout << "CMP\tprobe_count\t" << probe_count << "\nSC_DT_COMPARISONS_PUBLIC_OK\n";
  return 0;
}
