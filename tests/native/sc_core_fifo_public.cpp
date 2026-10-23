/** @file sc_core_fifo_public.cpp
 * @brief Independent native reference for public sc_fifo overloads.
 * @details The channels, trace file and full-width values remain owned through
 * simulation and serialization. No Python callbacks or GIL state are retained.
 */
#include <algorithm>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <systemc>
#include <typeinfo>

/** @brief Count an owned FIFO proxy's native destruction without storing callbacks. */
namespace fifo_lifecycle
{
inline int destroyed = 0;
struct Tracked : sc_core::sc_fifo<int>
{
  Tracked() : sc_core::sc_fifo<int>("lifecycle_fifo", 1)
  {
  }
  ~Tracked() override
  {
    ++destroyed;
  }
};
}  // namespace fifo_lifecycle

/** @brief Compare blocking and nonblocking FIFO behavior for scalar and wide T. */
struct fifo_public_model : sc_core::sc_module
{
  sc_core::sc_fifo<int> default_fifo;
  sc_core::sc_fifo<int> int_fifo;
  sc_core::sc_fifo<sc_dt::sc_biguint<137>> wide_fifo;
  sc_core::sc_fifo<int> assignment_fifo;
  sc_core::sc_fifo<double> float_fifo;
  sc_core::sc_fifo<bool> bool_fifo;
  sc_core::sc_fifo<sc_dt::sc_biguint<137>> wide_conversion_fifo;
  sc_core::sc_fifo<int> text_fifo;
  std::unique_ptr<fifo_lifecycle::Tracked> lifecycle_fifo;
  sc_dt::sc_biguint<137> sample = 0;
  sc_core::sc_trace_file* trace_file;
  int int_value = 0;
  std::string wide_first;
  std::string wide_second;
  std::string int_producer_time;
  std::string int_consumer_time;
  std::string wide_producer_time;
  std::string wide_consumer_time;
  int assigned_scalar = 0;
  std::string converted_wide;
  double converted_float = 0.0;
  bool converted_bool = false;

  SC_HAS_PROCESS(fifo_public_model);

  /** @brief Retain channels through every process and traced VCD transition. */
  explicit fifo_public_model(sc_core::sc_module_name name)
    : sc_module(name)
    , int_fifo("int_fifo", 1)
    , wide_fifo("wide_fifo", 2)
    , assignment_fifo("assignment_fifo", 1)
    , float_fifo("float_fifo", 1)
    , bool_fifo("bool_fifo", 1)
    , wide_conversion_fifo("wide_conversion_fifo", 1)
    , text_fifo("text_fifo", 2)
    , trace_file(sc_core::sc_create_vcd_trace_file("fifo_family"))
  {
    trace_file->set_time_unit(1, sc_core::SC_NS);
    sc_core::sc_trace(trace_file, sample, "wide_sample");
    wide_fifo.trace(trace_file);
    lifecycle_fifo = std::make_unique<fifo_lifecycle::Tracked>();
    if (!text_fifo.nb_write(51) || !text_fifo.nb_write(61)) throw std::runtime_error("text FIFO fill");
    SC_THREAD(int_producer);
    SC_THREAD(int_consumer);
    SC_THREAD(wide_producer);
    SC_THREAD(wide_consumer);
    SC_THREAD(assignment_conversion);
  }

  /** @brief Fill a one-slot channel and block until each write can complete. */
  void int_producer()
  {
    if (int_fifo.num_free() != 1 || int_fifo.num_available() != 0) throw std::runtime_error("initial int counts");
    int_fifo.write(17);
    if (int_fifo.num_free() != 0 || int_fifo.num_available() != 0) throw std::runtime_error("delta write counts");
    int_fifo.write(23);
    int_fifo.write(41);
    if (int_fifo.nb_write(99)) throw std::runtime_error("full nonblocking write");
    int_producer_time = sc_core::sc_time_stamp().to_string();
  }

  /** @brief Read into a caller reference, by value, and through nb_read. */
  void int_consumer()
  {
    int output = -1;
    if (int_fifo.nb_read(output) || output != -1) throw std::runtime_error("empty int read");
    sc_core::wait(sc_core::SC_ZERO_TIME);
    if (int_fifo.num_available() != 1) throw std::runtime_error("int availability after update");
    int_fifo.read(output);
    if (output != 17 || int_fifo.num_available() != 0 || int_fifo.num_free() != 0)
      throw std::runtime_error("blocking reference read");
    int_value = int_fifo.read();
    if (int_value != 23) throw std::runtime_error("blocking value read");
    int_fifo.read(output);
    if (output != 41 || int_fifo.nb_read(output) || output != 41) throw std::runtime_error("nonblocking int output");
    sc_core::wait(sc_core::SC_ZERO_TIME);
    if (int_fifo.num_available() != 0 || int_fifo.num_free() != 1) throw std::runtime_error("final int counts");
    int_consumer_time = sc_core::sc_time_stamp().to_string();
  }

  /** @brief Exercise two successful nb writes, a failed full write, and a blocked write. */
  void wide_producer()
  {
    using value_type = sc_dt::sc_biguint<137>;
    const value_type first("174224571863520493293247799005065324265");
    const value_type second("174224571863520493293247799005065324267");
    const value_type third("174224571863520493293247799005065324271");
    if (!wide_fifo.nb_write(first) || !wide_fifo.nb_write(second) || wide_fifo.nb_write(third))
      throw std::runtime_error("wide nonblocking writes");
    sample = first;
    wide_fifo.write(third);
    sc_core::wait(1, sc_core::SC_NS);
    sample = third;
    wide_producer_time = sc_core::sc_time_stamp().to_string();
  }

  /** @brief Read arbitrary-width outputs by reference and value and preserve on failure. */
  void wide_consumer()
  {
    using value_type = sc_dt::sc_biguint<137>;
    value_type output = 9;
    if (wide_fifo.nb_read(output) || output != 9) throw std::runtime_error("empty wide read");
    sc_core::wait(sc_core::SC_ZERO_TIME);
    if (wide_fifo.num_available() != 2 || wide_fifo.num_free() != 0) throw std::runtime_error("wide update counts");
    if (!wide_fifo.nb_read(output)) throw std::runtime_error("wide reference nb_read");
    wide_first = output.to_string();
    wide_second = wide_fifo.read().to_string();
    if (wide_first != "174224571863520493293247799005065324265" ||
        wide_second != "174224571863520493293247799005065324267")
      throw std::runtime_error("wide value mismatch");
    if (wide_fifo.read().to_string() != "174224571863520493293247799005065324271")
      throw std::runtime_error("wide blocking read");
    if (wide_fifo.nb_read(output) || output.to_string() != wide_first)
      throw std::runtime_error("wide output preservation");
    wide_consumer_time = sc_core::sc_time_stamp().to_string();
  }

  /** @brief Exercise native operator= and conversion operators for int and uint137. */
  void assignment_conversion()
  {
    assignment_fifo = 73;
    sc_core::wait(sc_core::SC_ZERO_TIME);
    assigned_scalar = static_cast<int>(assignment_fifo);
    if (assigned_scalar != 73) throw std::runtime_error("int FIFO conversion");
    sc_core::wait(sc_core::SC_ZERO_TIME);
    using value_type = sc_dt::sc_biguint<137>;
    const std::string expected = "174224571863520493293247799005065324269";
    wide_conversion_fifo = value_type(expected.c_str());
    sc_core::wait(sc_core::SC_ZERO_TIME);
    converted_wide = static_cast<value_type>(wide_conversion_fifo).to_string();
    if (converted_wide != expected) throw std::runtime_error("wide FIFO conversion");
    float_fifo = 2.75;
    bool_fifo = true;
    sc_core::wait(sc_core::SC_ZERO_TIME);
    converted_float = static_cast<double>(float_fifo);
    converted_bool = static_cast<bool>(bool_fifo);
    if (converted_float != 2.75 || !converted_bool) throw std::runtime_error("scalar FIFO conversion");
  }

  /** @brief Serialize native print/dump output while the stream remains locally owned. */
  std::pair<std::string, std::string> text() const
  {
    std::ostringstream print_stream;
    std::ostringstream insert_stream;
    std::ostringstream dump_stream;
    text_fifo.print(print_stream);
    insert_stream << text_fifo;
    text_fifo.dump(dump_stream);
    if (insert_stream.str() != print_stream.str()) throw std::runtime_error("FIFO stream insertion mismatch");
    return {print_stream.str(), dump_stream.str()};
  }
};

/** @brief Pass real public FIFO ports to register_port and expose registration reports. */
struct fifo_port_conflict : sc_core::sc_module
{
  sc_core::sc_fifo<int> fifo;
  sc_core::sc_fifo_in<int> first_reader;
  sc_core::sc_fifo_in<int> second_reader;
  sc_core::sc_fifo_out<int> first_writer;
  sc_core::sc_fifo_out<int> second_writer;

  /** @brief Construct stable ports and FIFO storage for direct registration calls. */
  fifo_port_conflict(sc_core::sc_module_name name, bool reader)
    : sc_module(name)
    , fifo("conflict_fifo", 2)
    , first_reader("first_reader")
    , second_reader("second_reader")
    , first_writer("first_writer")
    , second_writer("second_writer")
  {
  }

  /** @brief Call register_port twice using the exact native interface type spelling. */
  void register_twice(bool reader)
  {
    if (reader)
    {
      const char* interface_name = typeid(sc_core::sc_fifo_in_if<int>).name();
      fifo.register_port(first_reader, interface_name);
      fifo.register_port(second_reader, interface_name);
    }
    else
    {
      const char* interface_name = typeid(sc_core::sc_fifo_out_if<int>).name();
      fifo.register_port(first_writer, interface_name);
      fifo.register_port(second_writer, interface_name);
    }
  }
};

/** @brief Compare invalid-size reporting separately from the normal kernel. */
int sc_main(int argc, char** argv)
{
  if (argc > 1)
  {
    if (std::strcmp(argv[1], "duplicate-reader") == 0 || std::strcmp(argv[1], "duplicate-writer") == 0)
    {
      const bool reader = std::strcmp(argv[1], "duplicate-reader") == 0;
      try
      {
        fifo_port_conflict conflict("fifo_port_error", reader);
        conflict.register_twice(reader);
      }
      catch (const sc_core::sc_report& report)
      {
        std::cout << "FIFO_REGISTER_ERROR " << (reader ? "reader" : "writer") << " " << report.get_id() << " "
                  << report.get_msg_type() << "\n";
        return 0;
      }
      throw std::runtime_error("duplicate FIFO port registration was accepted");
    }
    try
    {
      sc_core::sc_fifo<int> invalid("invalid_fifo", 0);
    }
    catch (const sc_core::sc_report& report)
    {
      std::cout << "FIFO_INVALID_SIZE " << report.get_msg_type() << "\n";
      return 0;
    }
    throw std::runtime_error("invalid FIFO size was accepted");
  }

  fifo_public_model model("fifo_public");
  if (model.default_fifo.num_free() != 16 || std::string(model.default_fifo.kind()) != "sc_fifo" ||
      std::string(model.int_fifo.name()) != "fifo_public.int_fifo" ||
      std::string(model.int_fifo.data_read_event().name()).find("read_event") == std::string::npos ||
      std::string(model.int_fifo.data_written_event().name()).find("write_event") == std::string::npos)
    throw std::runtime_error("FIFO metadata mismatch");
  sc_core::sc_start(2, sc_core::SC_NS);
  const auto printed = model.text();
  if (printed.first != "51\n61\n" || printed.second != "name = fifo_public.text_fifo\nvalue[0] = 51\nvalue[1] = 61\n")
    throw std::runtime_error("FIFO print/dump mismatch");
  if (model.int_fifo.num_available() != 0 || model.int_fifo.num_free() != 1 || model.wide_fifo.num_available() != 0 ||
      model.wide_fifo.num_free() != 2)
    throw std::runtime_error("final FIFO counts mismatch");
  std::cout << "FIFO_REFERENCE " << model.default_fifo.num_free() << " " << model.default_fifo.kind() << " "
            << model.int_fifo.name() << " " << model.int_value << " " << model.wide_first << " " << model.wide_second
            << " " << model.int_fifo.num_available() << " " << model.int_fifo.num_free() << " "
            << model.wide_fifo.num_available() << " " << model.wide_fifo.num_free() << "\n";
  std::cout << "FIFO_PROCESSES int_producer=" << model.int_producer_time << " int_consumer=" << model.int_consumer_time
            << " wide_producer=" << model.wide_producer_time << " wide_consumer=" << model.wide_consumer_time << "\n";
  std::cout << "FIFO_ASSIGNMENT_CONVERSION " << model.assigned_scalar << " " << model.converted_wide << "\n";
  std::cout << "FIFO_SCALAR_CONVERSIONS " << model.converted_float << " " << (model.converted_bool ? "True" : "False")
            << "\n";
  std::string printed_line = printed.first;
  std::string dumped_line = printed.second;
  std::replace(printed_line.begin(), printed_line.end(), '\n', '|');
  std::replace(dumped_line.begin(), dumped_line.end(), '\n', '|');
  if (!printed_line.empty()) printed_line.pop_back();
  if (!dumped_line.empty()) dumped_line.pop_back();
  std::cout << "FIFO_TEXT " << printed_line << " " << dumped_line << "\n";
  std::cout << "FIFO_STREAM " << printed_line << "\n";
  std::cout << "FIFO_EVENTS " << model.int_fifo.data_read_event().name() << " "
            << model.int_fifo.data_written_event().name() << "\n";
  sc_core::sc_stop();
  sc_core::sc_close_vcd_trace_file(model.trace_file);
  model.lifecycle_fifo.reset();
  std::cout << "FIFO_DESTRUCTION " << fifo_lifecycle::destroyed << "\n";
  return 0;
}
