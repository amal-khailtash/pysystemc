/** @file sc_core_fifo_ports_public.cpp
 *  @brief Native reference for the complete sc_fifo_in/sc_fifo_out port families.
 */
#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <type_traits>
#include <vector>

namespace
{
using wide_type = sc_dt::sc_biguint<137>;
using in_type = sc_core::sc_fifo_in<wide_type>;
using out_type = sc_core::sc_fifo_out<wide_type>;
using in_if_type = sc_core::sc_fifo_in_if<wide_type>;
using out_if_type = sc_core::sc_fifo_out_if<wide_type>;
using generic_in_port = sc_core::sc_port<in_if_type, 0, sc_core::SC_ONE_OR_MORE_BOUND>;
using generic_out_port = sc_core::sc_port<out_if_type, 0, sc_core::SC_ONE_OR_MORE_BOUND>;
std::vector<int> report_ids;
std::vector<std::string> report_types;

void capture_report(const sc_core::sc_report& report, const sc_core::sc_actions&)
{
  report_ids.push_back(report.get_id());
  report_types.emplace_back(report.get_msg_type());
}

static_assert(std::is_same<in_type::data_type, wide_type>::value, "input data alias");
static_assert(std::is_same<in_type::if_type, in_if_type>::value, "input interface alias");
static_assert(std::is_same<in_type::in_if_type, in_if_type>::value, "input direction alias");
static_assert(std::is_same<in_type::base_type, generic_in_port>::value, "input base alias");
static_assert(std::is_same<in_type::in_port_type, sc_core::sc_port_b<in_if_type>>::value, "input base port alias");
static_assert(std::is_same<in_type::this_type, in_type>::value, "input self alias");
static_assert(std::is_same<out_type::data_type, wide_type>::value, "output data alias");
static_assert(std::is_same<out_type::if_type, out_if_type>::value, "output interface alias");
static_assert(std::is_same<out_type::out_if_type, out_if_type>::value, "output direction alias");
static_assert(std::is_same<out_type::base_type, generic_out_port>::value, "output base alias");
static_assert(std::is_same<out_type::out_port_type, sc_core::sc_port_b<out_if_type>>::value, "output base port alias");
static_assert(std::is_same<out_type::this_type, out_type>::value, "output self alias");
static_assert(!std::is_copy_constructible<in_type>::value, "input copy constructor disabled");
static_assert(!std::is_copy_assignable<in_type>::value, "input copy assignment disabled");
static_assert(!std::is_copy_constructible<out_type>::value, "output copy constructor disabled");
static_assert(!std::is_copy_assignable<out_type>::value, "output copy assignment disabled");
static_assert(std::is_destructible<in_type>::value, "input destructor is public");
static_assert(std::is_destructible<out_type>::value, "output destructor is public");
static_assert(!std::is_constructible<in_type, const in_if_type&>::value, "input port does not bind a const interface");
static_assert(!std::is_constructible<in_type, const char*, const in_if_type&>::value,
              "named input port does not bind a const interface");
static_assert(!std::is_constructible<out_type, const out_if_type&>::value,
              "output port does not bind a const interface");
static_assert(!std::is_constructible<out_type, const char*, const out_if_type&>::value,
              "named output port does not bind a const interface");

/** @brief Probe protected port lifecycle and binding state inherited from sc_port_base. */
class in_probe : public in_type
{
 public:
  explicit in_probe(const char* name) : in_type(name)
  {
  }
  bool before_called = false;
  bool binding_state_allocated() const
  {
    return m_bind_info != nullptr;
  }

 protected:
  void before_end_of_elaboration() override
  {
    in_type::before_end_of_elaboration();
    before_called = true;
  }
};

/** @brief Probe protected port lifecycle and binding state inherited from sc_port_base. */
class out_probe : public out_type
{
 public:
  explicit out_probe(const char* name) : out_type(name)
  {
  }
  bool before_called = false;
  bool binding_state_allocated() const
  {
    return m_bind_info != nullptr;
  }

 protected:
  void before_end_of_elaboration() override
  {
    out_type::before_end_of_elaboration();
    before_called = true;
  }
};

std::string time_text()
{
  std::ostringstream stream;
  stream << sc_core::sc_time_stamp();
  return stream.str();
}

/** @brief Own constructor matrices and execute native FIFO-port reference operations. */
struct model : sc_core::sc_module
{
  // Parent vectors precede child vectors so children are destroyed first.
  std::vector<std::unique_ptr<sc_core::sc_fifo<wide_type>>> channels;
  std::vector<std::unique_ptr<generic_in_port>> generic_in_parents;
  std::vector<std::unique_ptr<generic_out_port>> generic_out_parents;
  std::vector<std::unique_ptr<in_type>> concrete_in_parents;
  std::vector<std::unique_ptr<out_type>> concrete_out_parents;
  std::vector<std::unique_ptr<in_type>> inputs;
  std::vector<std::unique_ptr<out_type>> outputs;

  sc_core::sc_fifo<wide_type> blocking_fifo;
  sc_core::sc_fifo<wide_type> probe_fifo;
  sc_core::sc_fifo<wide_type> lifetime_fifo;
  in_type blocking_in;
  out_type blocking_out;
  std::unique_ptr<in_type> lifetime_in;
  std::unique_ptr<out_type> lifetime_out;
  in_probe probe_in;
  out_probe probe_out;
  sc_core::sc_signal<bool> activity;
  sc_core::sc_event start_event;
  std::vector<std::string> written_events;
  std::vector<std::string> read_events;
  std::vector<std::string> process_records;

  SC_HAS_PROCESS(model);
  explicit model(sc_core::sc_module_name name)
    : sc_module(name)
    , blocking_fifo("blocking_fifo", 1)
    , probe_fifo("probe_fifo", 1)
    , lifetime_fifo("lifetime_fifo", 1)
    , blocking_in("blocking_in")
    , blocking_out("blocking_out")
    , probe_in("probe_in")
    , probe_out("probe_out")
    , activity("activity")
  {
    blocking_in.bind(blocking_fifo);
    blocking_out(blocking_fifo);
    lifetime_in.reset(new in_type("lifetime_in"));
    lifetime_in->bind(lifetime_fifo);
    lifetime_in->data_written();
    lifetime_out.reset(new out_type("lifetime_out"));
    (*lifetime_out)(lifetime_fifo);
    lifetime_out->data_read();
    make_matrix();
    SC_METHOD(on_written);
    sensitive << blocking_in.data_written();
    dont_initialize();
    SC_METHOD(on_read);
    sensitive << blocking_out.data_read();
    dont_initialize();
    SC_THREAD(producer);
    SC_THREAD(consumer);
  }

  void make_matrix()
  {
    for (int index = 0; index < 8; ++index)
    {
      const std::string input_label = "matrix_in_" + std::to_string(index);
      const std::string output_label = "matrix_out_" + std::to_string(index);
      channels.emplace_back(new sc_core::sc_fifo<wide_type>(("matrix_" + std::to_string(index) + "_fifo").c_str(), 1));
      auto* input_fifo = channels.back().get();
      auto* output_fifo = input_fifo;

      switch (index)
      {
      case 0:
      {
        generic_in_parents.emplace_back(new generic_in_port((input_label + "_parent").c_str()));
        generic_in_parents.back()->bind(*input_fifo);
        inputs.emplace_back(new in_type());
        (*inputs.back())(*generic_in_parents.back());
        generic_out_parents.emplace_back(new generic_out_port((output_label + "_parent").c_str()));
        generic_out_parents.back()->bind(*output_fifo);
        outputs.emplace_back(new out_type());
        (*outputs.back())(*generic_out_parents.back());
        break;
      }
      case 1:
        inputs.emplace_back(new in_type(input_label.c_str()));
        (*inputs.back())(*input_fifo);
        outputs.emplace_back(new out_type(output_label.c_str()));
        outputs.back()->bind(*output_fifo);
        break;
      case 2:
        inputs.emplace_back(new in_type(*input_fifo));
        outputs.emplace_back(new out_type(*output_fifo));
        break;
      case 3:
        inputs.emplace_back(new in_type(input_label.c_str(), *input_fifo));
        outputs.emplace_back(new out_type(output_label.c_str(), *output_fifo));
        break;
      case 4:
      case 5:
        generic_in_parents.emplace_back(new generic_in_port((input_label + "_parent").c_str()));
        generic_in_parents.back()->bind(*input_fifo);
        inputs.emplace_back(index == 4 ? new in_type(*generic_in_parents.back())
                                       : new in_type(input_label.c_str(), *generic_in_parents.back()));
        generic_out_parents.emplace_back(new generic_out_port((output_label + "_parent").c_str()));
        generic_out_parents.back()->bind(*output_fifo);
        outputs.emplace_back(index == 4 ? new out_type(*generic_out_parents.back())
                                        : new out_type(output_label.c_str(), *generic_out_parents.back()));
        break;
      case 6:
      case 7:
        concrete_in_parents.emplace_back(new in_type((input_label + "_parent").c_str()));
        concrete_in_parents.back()->bind(*input_fifo);
        inputs.emplace_back(index == 6 ? new in_type(*concrete_in_parents.back())
                                       : new in_type(input_label.c_str(), *concrete_in_parents.back()));
        concrete_out_parents.emplace_back(new out_type((output_label + "_parent").c_str()));
        concrete_out_parents.back()->bind(*output_fifo);
        outputs.emplace_back(index == 6 ? new out_type(*concrete_out_parents.back())
                                        : new out_type(output_label.c_str(), *concrete_out_parents.back()));
        break;
      }
    }
    probe_in.bind(probe_fifo);
    probe_out.bind(probe_fifo);
  }

  void on_written()
  {
    written_events.push_back(time_text());
  }
  void on_read()
  {
    read_events.push_back(time_text());
  }

  void producer()
  {
    wait(start_event);
    wait(1, sc_core::SC_NS);
    wide_type first = 17;
    first[136] = true;
    blocking_out.write(first);
    activity.write(true);
    process_records.push_back("write:first:" + time_text());
    wait(1, sc_core::SC_NS);
    blocking_out.write(wide_type(29));
    activity.write(false);
    process_records.push_back("write:second:" + time_text());
  }

  void consumer()
  {
    wait(start_event);
    wait(1, sc_core::SC_NS);
    wide_type value = 73;
    if (blocking_in.nb_read(value) || value.to_uint() != 73)
      throw std::runtime_error("empty nonblocking read changed output");
    blocking_in.read(value);
    if (!value[136] || value.to_uint() != 17) throw std::runtime_error("blocking reference read");
    process_records.push_back("read:first:" + time_text());
    const wide_type result = blocking_in.read();
    if (result.to_uint() != 29 || result[136]) throw std::runtime_error("blocking value read");
    process_records.push_back("read:second:" + time_text());
  }
};

void verify_port(sc_core::sc_port_base& port, const char* expected_kind)
{
  if (std::string(port.kind()) != expected_kind || port.bind_count() != 1)
    throw std::runtime_error("port kind/bind count");
  std::ostringstream printed;
  std::ostringstream dumped;
  port.print(printed);
  port.dump(dumped);
  if (printed.str().find(port.name()) == std::string::npos || dumped.str().find(port.name()) == std::string::npos)
    throw std::runtime_error("port print/dump");
}
}  // namespace

/** @brief Check constructors, binding, port convenience calls and process behavior. */
int sc_main(int argc, char** argv)
{
  if (argc > 1 && std::string(argv[1]) == "errors")
  {
    sc_core::sc_module_name error_name("error_top");
    struct error_model : sc_core::sc_module
    {
      in_type input;
      out_type output;
      explicit error_model(sc_core::sc_module_name name) : sc_module(name), input("error_in"), output("error_out")
      {
      }
    } errors(error_name);
    const sc_core::sc_report_handler_proc previous_handler = sc_core::sc_report_handler::set_handler(&capture_report);
    const sc_core::sc_actions previous_actions =
      sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_DO_NOTHING);
    const sc_core::sc_event& unresolved_input = errors.input.data_written().find_event();
    const sc_core::sc_event& unresolved_output = errors.output.data_read().find_event();
    sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous_actions);
    sc_core::sc_report_handler::set_handler(previous_handler);
    if (&unresolved_input != &sc_core::sc_event::none || &unresolved_output != &sc_core::sc_event::none ||
        report_ids != std::vector<int>{118, 118} ||
        report_types != std::vector<std::string>{"find event failed", "find event failed"})
      throw std::runtime_error("unbound FIFO port finder report");
    std::cout << "FIFO_PORT_ERRORS\t1\t1\n";
    return 0;
  }
  model top("top");
  sc_core::sc_trace_file* trace = sc_core::sc_create_vcd_trace_file("fifo_ports_public");
  trace->set_time_unit(1, sc_core::SC_NS);
  sc_core::sc_trace(trace, top.activity, "activity");
  if (!top.probe_in.binding_state_allocated() || !top.probe_out.binding_state_allocated())
    throw std::runtime_error("protected binding state before elaboration");
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  if (!top.probe_in.before_called || !top.probe_out.before_called || top.probe_in.binding_state_allocated() ||
      top.probe_out.binding_state_allocated())
    throw std::runtime_error("protected port probe");

  std::vector<int> matrix_values;
  for (int index = 0; index < 8; ++index)
  {
    in_type& input = *top.inputs[index];
    out_type& output = *top.outputs[index];
    verify_port(input, "sc_fifo_in");
    verify_port(output, "sc_fifo_out");
    if (input.size() != 1 || output.size() != 1 || input.num_available() != 0 || output.num_free() != 1)
      throw std::runtime_error("input/output metadata");
    if (&input.data_written() != &input.data_written() ||
        &input.data_written().find_event() != &input.data_written_event() ||
        &output.data_read() != &output.data_read() || &output.data_read().find_event() != &output.data_read_event())
      throw std::runtime_error("event finder lookup/cache");
    if (!output.nb_write(wide_type(index + 17)) || output.nb_write(wide_type(99)))
      throw std::runtime_error("nonblocking output port");
  }
  sc_core::sc_start(1, sc_core::SC_NS);
  for (int index = 0; index < 8; ++index)
  {
    in_type& input = *top.inputs[index];
    out_type& output = *top.outputs[index];
    if (input.num_available() != 1) throw std::runtime_error("input availability");
    wide_type value = 0;
    if (!input.nb_read(value) || value.to_uint() != static_cast<unsigned int>(index + 17))
      throw std::runtime_error("nonblocking input port");
    if (input.nb_read(value) || value.to_uint() != static_cast<unsigned int>(index + 17))
      throw std::runtime_error("failed nonblocking input changed output");
    matrix_values.push_back(static_cast<int>(value.to_uint()));
    output.trace(trace);
    input.trace(trace);
  }
  sc_core::sc_start(1, sc_core::SC_NS);
  for (int index = 0; index < 8; ++index)
  {
    if (top.inputs[index]->num_available() != 0 || top.outputs[index]->num_free() != 1)
      throw std::runtime_error("final FIFO port counts");
  }

  if (&top.blocking_in.data_written() != &top.blocking_in.data_written() ||
      &top.blocking_out.data_read() != &top.blocking_out.data_read())
    throw std::runtime_error("cached finder identity");
  top.start_event.notify(sc_core::SC_ZERO_TIME);
  sc_core::sc_start(4, sc_core::SC_NS);
  std::sort(top.written_events.begin(), top.written_events.end());
  std::sort(top.read_events.begin(), top.read_events.end());
  std::sort(top.process_records.begin(), top.process_records.end());
  if (top.written_events != std::vector<std::string>{"3 ns", "4 ns"} ||
      top.read_events != std::vector<std::string>{"3 ns", "4 ns"} ||
      top.process_records !=
        std::vector<std::string>{"read:first:3 ns", "read:second:4 ns", "write:first:3 ns", "write:second:4 ns"})
    throw std::runtime_error("blocking FIFO port timing");
  if (top.blocking_in.num_available() != 0 || top.blocking_out.num_free() != 1)
    throw std::runtime_error("final blocking FIFO port counts");
  sc_core::sc_stop();
  sc_core::sc_close_vcd_trace_file(trace);
  const std::string lifetime_in_name = top.lifetime_in->name();
  const std::string lifetime_out_name = top.lifetime_out->name();
  top.lifetime_in.reset();
  top.lifetime_out.reset();
  if (sc_core::sc_find_object(lifetime_in_name.c_str()) != nullptr ||
      sc_core::sc_find_object(lifetime_out_name.c_str()) != nullptr)
    throw std::runtime_error("FIFO port destruction left a registered object");

  std::cout << "FIFO_PORT_MATRIX\t";
  for (std::size_t index = 0; index < matrix_values.size(); ++index)
  {
    if (index != 0) std::cout << ',';
    std::cout << matrix_values[index];
  }
  std::cout << "\nFIFO_PORT_PROTECTED\t1\t1\nFIFO_PORT_LIFETIME\t1\t1\nFIFO_PORT_REFERENCE\t";
  for (std::size_t index = 0; index < top.written_events.size(); ++index)
  {
    if (index != 0) std::cout << '\t';
    std::cout << top.written_events[index];
  }
  std::cout << '\n';
  return 0;
}
