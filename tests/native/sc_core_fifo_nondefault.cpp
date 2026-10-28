/** @file Native oracle for non-default-constructible FIFO value reads. */
#include "sc_core_fifo_nondefault.h"

#include <iostream>
#include <systemc>

int sc_main(int, char**)
{
  sc_core_fifo_nondefault::Reader reader;
  auto value = reader.read();
  sc_core_fifo_nondefault::Value output(-1);
  reader.read(output);
  std::cout << "FIFO_NONDEFAULT_REFERENCE " << value.value << " " << output.value << " " << reader.value_read_calls()
            << " " << reader.output_read_calls() << std::endl;
  return 0;
}
