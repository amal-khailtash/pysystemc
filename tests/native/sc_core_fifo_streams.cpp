/** @file Independent native FIFO insertion and stream failure reference. */
#include <fstream>
#include <iostream>
#include <sstream>
#include <systemc>
namespace fifo_stream_test
{
/** Derived native string stream reference. */
struct Derived : std::ostringstream
{
};
/** Arbitrary native-valid sample type. */
struct Sample
{
  int value;
  Sample(int value = 0) : value(value)
  {
  }
};
/** Print samples without modifying their storage. */
std::ostream& operator<<(std::ostream& stream, const Sample& sample)
{
  return stream << "sample(" << sample.value << ")";
}
/** Flatten sample lines for reference comparison. */
std::string flatten(std::string value)
{
  for (char& c : value)
    if (c == '\n') c = '|';
  return value;
}
}  // namespace fifo_stream_test
int sc_main(int, char**)
{
  sc_core::sc_fifo<int> fifo("fifo", 2);
  fifo.nb_write(17);
  fifo.nb_write(23);
  sc_core::sc_fifo<sc_dt::sc_biguint<137>> wide("wide", 1);
  wide.nb_write(sc_dt::sc_biguint<137>("87112285931760246646623899502532662132737"));
  sc_core::sc_fifo<fifo_stream_test::Sample> sample("sample", 1);
  sample.nb_write(fifo_stream_test::Sample(9));
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  auto print = [&](const char* label, auto& stream)
  {
    stream << "begin:" << fifo << "end:" << 41;
    std::cout << "FIFO_STREAM\t" << label << '\t' << fifo_stream_test::flatten(stream.str()) << '\n';
  };
  std::ostringstream stream;
  print("string", stream);
  std::stringstream combined;
  print("combined", combined);
  fifo_stream_test::Derived derived;
  print("derived", derived);
  std::ofstream file("fifo.txt");
  file << fifo;
  file.close();
  std::cout << "FIFO_STREAM\tfile\t17|23|\n";
  std::ostringstream generic;
  generic << wide << sample;
  std::cout << "FIFO_STREAM\tgeneric\t" << fifo_stream_test::flatten(generic.str()) << '\n';
  std::ostringstream failed;
  failed.exceptions(std::ios::badbit);
  try
  {
    failed.setstate(std::ios::badbit);
  }
  catch (const std::ios_base::failure&)
  {
  }
  try
  {
    failed << fifo;
  }
  catch (const std::ios_base::failure& error)
  {
    std::cout << "FIFO_STREAM_ERROR\tiostream error\n";
  }
  if (fifo.num_available() != 2) throw std::runtime_error("insertion consumed queue");
  sc_core::sc_stop();
  return 0;
}
