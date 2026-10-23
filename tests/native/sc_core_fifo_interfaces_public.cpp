/** @file sc_core_fifo_interfaces_public.cpp
 * @brief Compare FIFO interface dispatch, event timing and native type traits.
 * FIFO events are borrowed by the observer and remain owned by the FIFO. The
 * derived interface probes own their events and test protected construction
 * and virtual destruction through public interface pointers.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>
#include <vector>

using namespace sc_core;

/** @brief Concrete input implementation for constructor/destructor probes. */
struct InputProbe : sc_fifo_in_if<int>
{
  sc_event written;
  static int destructions;

  /** @brief Invoke the protected base constructor through a derived class. */
  InputProbe() : sc_fifo_in_if<int>()
  {
  }
  /** @brief Count destruction through an interface pointer. */
  ~InputProbe() override
  {
    ++destructions;
  }
  bool nb_read(int&) override
  {
    return false;
  }
  const sc_event& data_written_event() const override
  {
    return written;
  }
  void read(int&) override
  {
  }
  int read() override
  {
    return 0;
  }
  int num_available() const override
  {
    return 0;
  }
};

int InputProbe::destructions = 0;

/** @brief Concrete output implementation for constructor/destructor probes. */
struct OutputProbe : sc_fifo_out_if<int>
{
  sc_event read_event;
  static int destructions;

  /** @brief Invoke the protected base constructor through a derived class. */
  OutputProbe() : sc_fifo_out_if<int>()
  {
  }
  /** @brief Count destruction through an interface pointer. */
  ~OutputProbe() override
  {
    ++destructions;
  }
  int num_free() const override
  {
    return 0;
  }
  bool nb_write(const int&) override
  {
    return false;
  }
  const sc_event& data_read_event() const override
  {
    return read_event;
  }
  void write(const int&) override
  {
  }
};

int OutputProbe::destructions = 0;

/** @brief Concrete blocking input used for base-pointer destruction checks. */
struct BlockingInputProbe : sc_fifo_blocking_in_if<int>
{
  static int destructions;
  ~BlockingInputProbe() override
  {
    ++destructions;
  }
  void read(int&) override
  {
  }
  int read() override
  {
    return 0;
  }
};

int BlockingInputProbe::destructions = 0;

/** @brief Concrete blocking output used for base-pointer destruction checks. */
struct BlockingOutputProbe : sc_fifo_blocking_out_if<int>
{
  static int destructions;
  ~BlockingOutputProbe() override
  {
    ++destructions;
  }
  void write(const int&) override
  {
  }
};

int BlockingOutputProbe::destructions = 0;

/** @brief Concrete nonblocking input used for base-pointer destruction checks. */
struct NonblockingInputProbe : sc_fifo_nonblocking_in_if<int>
{
  sc_event written;
  static int destructions;
  ~NonblockingInputProbe() override
  {
    ++destructions;
  }
  bool nb_read(int&) override
  {
    return false;
  }
  const sc_event& data_written_event() const override
  {
    return written;
  }
};

int NonblockingInputProbe::destructions = 0;

/** @brief Concrete nonblocking output used for base-pointer destruction checks. */
struct NonblockingOutputProbe : sc_fifo_nonblocking_out_if<int>
{
  sc_event read_event;
  static int destructions;
  ~NonblockingOutputProbe() override
  {
    ++destructions;
  }
  bool nb_write(const int&) override
  {
    return false;
  }
  const sc_event& data_read_event() const override
  {
    return read_event;
  }
};

int NonblockingOutputProbe::destructions = 0;

static_assert(std::is_base_of<sc_fifo_nonblocking_in_if<int>, sc_fifo_in_if<int>>::value);
static_assert(std::is_base_of<sc_fifo_blocking_in_if<int>, sc_fifo_in_if<int>>::value);
static_assert(std::is_base_of<sc_fifo_nonblocking_out_if<int>, sc_fifo_out_if<int>>::value);
static_assert(std::is_base_of<sc_fifo_blocking_out_if<int>, sc_fifo_out_if<int>>::value);
static_assert(std::is_base_of<sc_interface, sc_fifo_blocking_in_if<int>>::value);
static_assert(std::is_base_of<sc_interface, sc_fifo_nonblocking_in_if<int>>::value);
static_assert(std::is_base_of<sc_interface, sc_fifo_blocking_out_if<int>>::value);
static_assert(std::is_base_of<sc_interface, sc_fifo_nonblocking_out_if<int>>::value);
static_assert(std::is_abstract<sc_fifo_in_if<int>>::value);
static_assert(std::is_abstract<sc_fifo_out_if<int>>::value);
static_assert(std::is_abstract<sc_fifo_blocking_in_if<int>>::value);
static_assert(std::is_abstract<sc_fifo_blocking_out_if<int>>::value);
static_assert(std::is_abstract<sc_fifo_nonblocking_in_if<int>>::value);
static_assert(std::is_abstract<sc_fifo_nonblocking_out_if<int>>::value);
static_assert(std::has_virtual_destructor<sc_fifo_in_if<int>>::value);
static_assert(std::has_virtual_destructor<sc_fifo_out_if<int>>::value);
static_assert(std::has_virtual_destructor<sc_fifo_blocking_in_if<int>>::value);
static_assert(std::has_virtual_destructor<sc_fifo_blocking_out_if<int>>::value);
static_assert(std::has_virtual_destructor<sc_fifo_nonblocking_in_if<int>>::value);
static_assert(std::has_virtual_destructor<sc_fifo_nonblocking_out_if<int>>::value);
static_assert(!std::is_copy_constructible<sc_fifo_in_if<int>>::value);
static_assert(!std::is_copy_assignable<sc_fifo_in_if<int>>::value);
static_assert(!std::is_copy_constructible<sc_fifo_out_if<int>>::value);
static_assert(!std::is_copy_assignable<sc_fifo_out_if<int>>::value);
using ReadRef = void (sc_fifo_blocking_in_if<int>::*)(int&);
using ReadValue = int (sc_fifo_blocking_in_if<int>::*)();
using NbRead = bool (sc_fifo_nonblocking_in_if<int>::*)(int&);
using NbWrite = bool (sc_fifo_nonblocking_out_if<int>::*)(const int&);
using Write = void (sc_fifo_blocking_out_if<int>::*)(const int&);
using ReadEvent = const sc_event& (sc_fifo_nonblocking_in_if<int>::*)() const;
using WrittenEvent = const sc_event& (sc_fifo_nonblocking_out_if<int>::*)() const;
static_assert(std::is_same<decltype(static_cast<ReadRef>(&sc_fifo_blocking_in_if<int>::read)), ReadRef>::value);
static_assert(std::is_same<decltype(static_cast<ReadValue>(&sc_fifo_blocking_in_if<int>::read)), ReadValue>::value);
static_assert(std::is_same<decltype(&sc_fifo_nonblocking_in_if<int>::nb_read), NbRead>::value);
static_assert(std::is_same<decltype(&sc_fifo_nonblocking_out_if<int>::nb_write), NbWrite>::value);
static_assert(std::is_same<decltype(&sc_fifo_blocking_out_if<int>::write), Write>::value);
static_assert(std::is_same<decltype(&sc_fifo_nonblocking_in_if<int>::data_written_event), ReadEvent>::value);
static_assert(std::is_same<decltype(&sc_fifo_nonblocking_out_if<int>::data_read_event), WrittenEvent>::value);
static_assert(std::is_default_constructible<InputProbe>::value);
static_assert(std::is_default_constructible<OutputProbe>::value);
using WideType = sc_dt::sc_biguint<137>;
static_assert(std::is_abstract<sc_fifo_in_if<WideType>>::value);
static_assert(std::is_abstract<sc_fifo_out_if<WideType>>::value);
static_assert(std::is_abstract<sc_fifo_blocking_in_if<WideType>>::value);
static_assert(std::is_abstract<sc_fifo_blocking_out_if<WideType>>::value);
static_assert(std::is_abstract<sc_fifo_nonblocking_in_if<WideType>>::value);
static_assert(std::is_abstract<sc_fifo_nonblocking_out_if<WideType>>::value);

/** @brief Record FIFO activity with events borrowed for the module lifetime. */
struct Observer : sc_module
{
  SC_HAS_PROCESS(Observer);
  std::vector<std::pair<char, std::pair<sc_dt::uint64, sc_dt::uint64>>> records;

  /** @brief Register separate observers for read and write activity. */
  Observer(sc_module_name name, const sc_event& written, const sc_event& read) : sc_module(name)
  {
    SC_METHOD(on_write);
    sensitive << written;
    dont_initialize();
    SC_METHOD(on_read);
    sensitive << read;
    dont_initialize();
  }

  /** @brief Record write activity at its native time and delta. */
  void on_write()
  {
    records.push_back({'W', {sc_time_stamp().value(), sc_delta_count()}});
  }
  /** @brief Record read activity at its native time and delta. */
  void on_read()
  {
    records.push_back({'R', {sc_time_stamp().value(), sc_delta_count()}});
  }
};

/** @brief Compare public interface behavior against the Python model. */
int sc_main(int, char**)
{
  sc_fifo<int> fifo("fifo", 2);
  sc_fifo_in_if<int>& combined_in = fifo;
  sc_fifo_out_if<int>& combined_out = fifo;
  sc_fifo_blocking_in_if<int>& blocking_in = fifo;
  sc_fifo_blocking_out_if<int>& blocking_out = fifo;
  sc_fifo_nonblocking_in_if<int>& nonblocking_in = fifo;
  sc_fifo_nonblocking_out_if<int>& nonblocking_out = fifo;
  Observer observer("observer", nonblocking_in.data_written_event(), nonblocking_out.data_read_event());

  auto* input_probe = new InputProbe;
  sc_fifo_in_if<int>* input_base = input_probe;
  delete input_base;
  auto* output_probe = new OutputProbe;
  sc_fifo_out_if<int>* output_base = output_probe;
  delete output_base;
  if (InputProbe::destructions != 1 || OutputProbe::destructions != 1) return 2;

  auto* temporary = new sc_fifo<int>("fifo_lifetime", 1);
  delete temporary;
  if (sc_find_object("fifo_lifetime")) return 3;
  std::cout << "FIFO_IFACE_LIFETIME 1\n";

  auto* blocking_input_probe = new BlockingInputProbe;
  sc_fifo_blocking_in_if<int>* blocking_input_base = blocking_input_probe;
  delete blocking_input_base;
  auto* blocking_output_probe = new BlockingOutputProbe;
  sc_fifo_blocking_out_if<int>* blocking_output_base = blocking_output_probe;
  delete blocking_output_base;
  auto* nonblocking_input_probe = new NonblockingInputProbe;
  sc_fifo_nonblocking_in_if<int>* nonblocking_input_base = nonblocking_input_probe;
  delete nonblocking_input_base;
  auto* nonblocking_output_probe = new NonblockingOutputProbe;
  sc_fifo_nonblocking_out_if<int>* nonblocking_output_base = nonblocking_output_probe;
  delete nonblocking_output_base;
  if (BlockingInputProbe::destructions != 1 || BlockingOutputProbe::destructions != 1 ||
      NonblockingInputProbe::destructions != 1 || NonblockingOutputProbe::destructions != 1)
    return 4;
  std::cout << "FIFO_IFACE_DESTRUCTION 1 1 1 1 1 1\n";

  sc_fifo<WideType> wide_fifo("wide_fifo", 3);
  sc_fifo_in_if<WideType>& wide_combined_in = wide_fifo;
  sc_fifo_out_if<WideType>& wide_combined_out = wide_fifo;
  sc_fifo_blocking_in_if<WideType>& wide_blocking_in = wide_fifo;
  sc_fifo_blocking_out_if<WideType>& wide_blocking_out = wide_fifo;
  sc_fifo_nonblocking_in_if<WideType>& wide_nonblocking_in = wide_fifo;
  sc_fifo_nonblocking_out_if<WideType>& wide_nonblocking_out = wide_fifo;
  WideType wide_output = 73;
  bool wide_empty_read = wide_nonblocking_in.nb_read(wide_output);
  WideType wide_first("174224571863520493293247799005065324265");
  WideType wide_second("174224571863520493293247799005065324267");
  WideType wide_third("174224571863520493293247799005065324271");
  bool wide_write = wide_nonblocking_out.nb_write(wide_first);
  wide_blocking_out.write(wide_second);
  wide_combined_out.write(wide_third);
  int wide_available_before_update = wide_combined_in.num_available();
  int wide_free_before_update = wide_combined_out.num_free();
  if (wide_empty_read || wide_output.to_string() != "73" || !wide_write || wide_available_before_update != 0 ||
      wide_free_before_update != 0)
    return 5;

  int output = 91;
  bool empty_read = nonblocking_in.nb_read(output);
  std::cout << "FIFO_IFACE_VALUES " << combined_in.num_available() << ' ' << combined_out.num_free() << ' '
            << empty_read << ' ' << output << ' ';
  bool first_write = nonblocking_out.nb_write(17);
  blocking_out.write(23);
  bool rejected_write = combined_out.nb_write(29);
  int available_before_update = combined_in.num_available();
  int free_before_update = combined_out.num_free();
  sc_start(1, SC_NS);
  int readable = combined_in.num_available();
  int first_read = -1;
  blocking_in.read(first_read);
  int second_read = combined_in.read();
  int after_reads = combined_in.num_available();
  int free_before_read_update = combined_out.num_free();
  int wide_readable = wide_combined_in.num_available();
  if (wide_readable != 3 || !wide_nonblocking_in.nb_read(wide_output)) return 6;
  std::string wide_read_1 = wide_output.to_string();
  wide_blocking_in.read(wide_output);
  std::string wide_read_2 = wide_output.to_string();
  std::string wide_read_3 = wide_combined_in.read().to_string();
  int wide_after_reads = wide_combined_in.num_available();
  int wide_free_before_read_update = wide_combined_out.num_free();
  sc_start(1, SC_NS);
  int free_after_update = combined_out.num_free();
  int wide_free_after_update = wide_combined_out.num_free();
  std::cout << first_write << ' ' << rejected_write << ' ' << available_before_update << ' ' << free_before_update
            << ' ' << readable << ' ' << first_read << ' ' << second_read << ' ' << after_reads << ' '
            << free_before_read_update << ' ' << free_after_update << '\n';
  for (const auto& record : observer.records)
    std::cout << "FIFO_IFACE_EVENT " << record.first << ' ' << record.second.first << ' ' << record.second.second
              << '\n';
  if (observer.records.size() != 2 || observer.records[0].first != 'W' || observer.records[1].first != 'R') return 1;

  std::cout << "FIFO_IFACE_WIDE " << wide_empty_read << ' ' << wide_write << ' ' << wide_available_before_update << ' '
            << wide_free_before_update << ' ' << wide_readable << ' ' << wide_read_1 << ' ' << wide_read_2 << ' '
            << wide_read_3 << ' ' << wide_after_reads << ' ' << wide_free_before_read_update << ' '
            << wide_free_after_update << '\n';
  if (wide_read_1 != wide_first.to_string() || wide_read_2 != wide_second.to_string() ||
      wide_read_3 != wide_third.to_string() || wide_after_reads != 0 || wide_free_before_read_update != 0 ||
      wide_free_after_update != 3)
    return 7;

  sc_stop();
  return 0;
}
