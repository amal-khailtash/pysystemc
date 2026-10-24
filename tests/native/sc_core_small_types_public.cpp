/** @file sc_core_small_types_public.cpp @brief Native enum and clock/queue alias reference. */
#include <iostream>
#include <vector>

#include "sc_core_small_types_probe.h"
using namespace sc_core;
/** @brief Native channel alias model retaining ports and channels through simulation. */
struct AliasModel : sc_channel
{
  sc_clock clock;
  sc_in_clk input;
  sc_signal<bool> io_signal, out_signal;
  sc_inout_clk io;
  sc_out_clk output;
  sc_event_queue queue;
  sc_event_queue_port queue_port;
  std::vector<sc_dt::uint64> edges, events;
  SC_HAS_PROCESS(AliasModel);
  AliasModel(sc_module_name name)
    : sc_channel(name)
    , clock("clock", sc_time(2, SC_NS))
    , input("input")
    , io_signal("io_signal")
    , out_signal("out_signal")
    , io("io")
    , output("output")
    , queue("queue")
    , queue_port("queue_port")
  {
    input(clock);
    io(io_signal);
    output(out_signal);
    queue_port(queue);
    SC_METHOD(edge);
    sensitive << input.pos();
    dont_initialize();
    SC_METHOD(queued);
    sensitive << queue_port->default_event();
    dont_initialize();
    SC_THREAD(writer);
  }
  /** @brief Record positive clock edge ticks. */ void edge()
  {
    edges.push_back(sc_time_stamp().value());
  }
  /** @brief Record queued notification ticks. */ void queued()
  {
    events.push_back(sc_time_stamp().value());
  }
  /** @brief Write aliases and schedule two timed queue notifications. */ void writer()
  {
    queue_port->notify(1, SC_NS);
    queue_port->notify(3, SC_NS);
    wait(1, SC_NS);
    io.write(true);
    output.write(true);
    wait(2, SC_NS);
    io.write(false);
    output.write(false);
  }
};
/** @brief Validate enums through their exact native type and run alias modeling. */
int sc_main(int, char**)
{
  std::cout << "SMALL_TYPE sc_curr_proc_kind " << small_types_probe::enum_value<sc_curr_proc_kind>(SC_NO_PROC_) << ","
            << small_types_probe::enum_value<sc_curr_proc_kind>(SC_METHOD_PROC_) << ","
            << small_types_probe::enum_value<sc_curr_proc_kind>(SC_THREAD_PROC_) << ","
            << small_types_probe::enum_value<sc_curr_proc_kind>(SC_CTHREAD_PROC_) << "\n";
  std::cout << "SMALL_TYPE sc_descendant_inclusion_info "
            << small_types_probe::enum_value<sc_descendant_inclusion_info>(SC_NO_DESCENDANTS) << ","
            << small_types_probe::enum_value<sc_descendant_inclusion_info>(SC_INCLUDE_DESCENDANTS) << ","
            << small_types_probe::enum_value<sc_descendant_inclusion_info>(SC_INVALID_DESCENDANTS) << "\n";
  std::cout << "SMALL_TYPE sc_port_policy " << small_types_probe::enum_value<sc_port_policy>(SC_ONE_OR_MORE_BOUND)
            << "," << small_types_probe::enum_value<sc_port_policy>(SC_ZERO_OR_MORE_BOUND) << ","
            << small_types_probe::enum_value<sc_port_policy>(SC_ALL_BOUND) << "\n";
  std::cout << "SMALL_TYPE sc_severity " << small_types_probe::enum_value<sc_severity>(SC_INFO) << ","
            << small_types_probe::enum_value<sc_severity>(SC_WARNING) << ","
            << small_types_probe::enum_value<sc_severity>(SC_ERROR) << ","
            << small_types_probe::enum_value<sc_severity>(SC_FATAL) << ","
            << small_types_probe::enum_value<sc_severity>(SC_MAX_SEVERITY) << "\n";
  std::cout << "SMALL_TYPE sc_starvation_policy "
            << small_types_probe::enum_value<sc_starvation_policy>(SC_EXIT_ON_STARVATION) << ","
            << small_types_probe::enum_value<sc_starvation_policy>(SC_RUN_TO_TIME) << "\n";
  std::cout << "SMALL_TYPE sc_stop_mode " << small_types_probe::enum_value<sc_stop_mode>(SC_STOP_FINISH_DELTA) << ","
            << small_types_probe::enum_value<sc_stop_mode>(SC_STOP_IMMEDIATE) << "\n";
  std::cout << "SMALL_TYPE sc_time_unit " << small_types_probe::enum_value<sc_time_unit>(SC_FS) << ","
            << small_types_probe::enum_value<sc_time_unit>(SC_PS) << ","
            << small_types_probe::enum_value<sc_time_unit>(SC_NS) << ","
            << small_types_probe::enum_value<sc_time_unit>(SC_US) << ","
            << small_types_probe::enum_value<sc_time_unit>(SC_MS) << ","
            << small_types_probe::enum_value<sc_time_unit>(SC_SEC) << "\n";
  std::cout << "SMALL_TYPE sc_verbosity " << small_types_probe::enum_value<sc_verbosity>(SC_NONE) << ","
            << small_types_probe::enum_value<sc_verbosity>(SC_LOW) << ","
            << small_types_probe::enum_value<sc_verbosity>(SC_MEDIUM) << ","
            << small_types_probe::enum_value<sc_verbosity>(SC_HIGH) << ","
            << small_types_probe::enum_value<sc_verbosity>(SC_FULL) << ","
            << small_types_probe::enum_value<sc_verbosity>(SC_DEBUG) << "\n";
  std::cout << "SMALL_TYPE sc_writer_policy " << small_types_probe::enum_value<sc_writer_policy>(SC_ONE_WRITER) << ","
            << small_types_probe::enum_value<sc_writer_policy>(SC_MANY_WRITERS) << ","
            << small_types_probe::enum_value<sc_writer_policy>(SC_UNCHECKED_WRITERS) << "\n";
  std::cout << "SMALL_TYPE sc_actions " << small_types_probe::actions(sc_actions(SC_DISPLAY | SC_CACHE_REPORT)) << "\n";
  AliasModel model("aliases");
  sc_assert(std::string(small_types_probe::behavior_name(model)) == "aliases");
  sc_assert(std::string(small_types_probe::channel_name(model)) == "aliases");
  sc_start(5, SC_NS);
  sc_assert((model.edges == std::vector<sc_dt::uint64>{0, 2000, 4000}));
  sc_assert((model.events == std::vector<sc_dt::uint64>{1000, 3000}));
  sc_assert(!model.io.read() && !model.output.read());
  std::cout << "SMALL_ALIAS aliases edges=0,2000,4000 queue=1000,3000 io=0 out=0 end=" << sc_time_stamp().value()
            << "\n";
  return 0;
}
