/** @file plain_char.cpp
 * @brief Independent native plain char signal/FIFO endpoint reference.
 * @details Channels own their samples. sc_main performs all accesses without
 * Python callbacks or GIL requirements; no borrowed storage escapes.
 */
#include <iostream>
#include <limits>
#include <systemc>

/** @brief Compare native signal references and FIFO caller-output overloads. */
int sc_main(int, char**)
{
  sc_core::sc_signal<char> signal("signal");
  sc_core::sc_fifo<char> fifo("fifo", 1);
  char values[] = {std::numeric_limits<char>::lowest(), std::numeric_limits<char>::max(), 0, 7};
  for (char value : values)
  {
    signal.write(value);
    fifo.write(value);
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    char blocking = 0;
    fifo.read(blocking);
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    fifo.write(value);
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    char nonblocking = 0;
    if (!fifo.nb_read(nonblocking) || blocking != value || nonblocking != value || signal.read() != value ||
        signal.get_data_ref() != value)
      throw std::runtime_error("plain char mismatch");
    std::cout << "PLAIN_CHAR " << int(value) << " " << int(signal.read()) << " " << int(signal.get_data_ref()) << " "
              << int(blocking) << " " << int(nonblocking) << "\n";
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
  }
  return 0;
}
