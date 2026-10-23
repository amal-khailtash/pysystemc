/** @file sc_core_main_public.cpp
 *  @brief Native reference for direct, wrapped and elaboration entry paths.
 */
#include <string>
#include <systemc>
#include <vector>

/** @brief Own callback records from a timed worker and event-sensitive method. */
struct MainModel : sc_core::sc_module
{
  SC_HAS_PROCESS(MainModel);
  sc_core::sc_event tick{"tick"};
  std::vector<std::string> records;

  /** @brief Register the method and worker thread within the module scope. */
  explicit MainModel(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
    SC_METHOD(observe);
    sensitive << tick;
    dont_initialize();
    SC_THREAD(worker);
  }

  /** @brief Record the event method callback. */
  void observe()
  {
    records.emplace_back("event:" + sc_core::sc_time_stamp().to_string());
  }

  /** @brief Wait, trigger the method and wait again. */
  void worker()
  {
    wait(1, sc_core::SC_NS);
    records.emplace_back("thread:" + sc_core::sc_time_stamp().to_string());
    tick.notify(sc_core::SC_ZERO_TIME);
    wait(1, sc_core::SC_NS);
    records.emplace_back("thread:" + sc_core::sc_time_stamp().to_string());
  }
};

/** @brief Validate copied native arguments and return a nonzero status. */
int sc_main(int argc, char** argv)
{
  using namespace sc_core;
  if (argc != 4 ||
      std::string(argv[1]) != "direct" && std::string(argv[1]) != "global" && std::string(argv[1]) != "elab" &&
        std::string(argv[1]) != "public_elab" ||
      std::string(argv[2]) != "café" || std::string(argv[3]) != "")
    return 2;
  const std::string mode(argv[1]);
  argv[2][0] = 'X';
  if (std::string(sc_argv()[2]) != "café") return 4;
  MainModel model("main_model");
  sc_start(3, SC_NS);
  if (model.records.size() != 3 || model.records[0] != "thread:1 ns" || model.records[1] != "event:1 ns" ||
      model.records[2] != "thread:2 ns")
    return 3;
  std::cout << "MAIN_PUBLIC_REFERENCE " << mode << ' ' << mode << "|café|<empty> " << model.records[0] << ';'
            << model.records[1] << ';' << model.records[2] << ' ' << sc_time_stamp() << '\n';
  return 7;
}
