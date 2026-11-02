/** @file effective_defaults.cpp
 * @brief Native assertions for selected omitted versus explicit defaults.
 * @details Each invocation owns one kernel. Stack channels and modules outlive
 * simulation callbacks. Native-only code uses no Python references or GIL;
 * SystemC reports propagate to the runner and assertion failures return nonzero.
 */
#include <iostream>
#include <memory>
#include <string>
#include <systemc>
#include <utility>
#include <vector>

namespace
{
/** @brief Run one empty-kernel overload with an omitted or explicit policy.
 * @param shape Time, integer, or double overload selection.
 * @param policy Omitted, run-to-time, or starvation exit selection.
 * @return Zero if the final timestamp matches the native policy contract.
 */
int run_start(const std::string& shape, const std::string& policy)
{
  using namespace sc_core;
  const auto selected = policy == "exit" ? SC_EXIT_ON_STARVATION : SC_RUN_TO_TIME;
  if (shape == "time")
  {
    if (policy == "omitted")
      sc_start(sc_time(7, SC_NS));
    else
      sc_start(sc_time(7, SC_NS), selected);
  }
  else if (shape == "int")
  {
    if (policy == "omitted")
      sc_start(7, SC_NS);
    else
      sc_start(7, SC_NS, selected);
  }
  else if (shape == "double")
  {
    if (policy == "omitted")
      sc_start(7.0, SC_NS);
    else
      sc_start(7.0, SC_NS, selected);
  }
  else
    return 2;
  if (sc_time_stamp() != sc_time(policy == "exit" ? 0 : 7, SC_NS)) return 3;
  std::cout << "DEFAULT_START\t" << sc_time_stamp().value() << '\n';
  return 0;
}

/** @brief Compare owned default and explicit FIFOs through nonblocking updates.
 * @return Zero if both capacities, ordered values and recovered space match.
 * @details No callbacks or coroutine stacks are used; channels outlive sc_start.
 */
int run_fifo()
{
  using namespace sc_core;
  sc_fifo<int> omitted;
  sc_fifo<int> explicit_size(16);
  for (auto* channel : {&omitted, &explicit_size})
  {
    if (channel->num_free() != 16 || channel->num_available() != 0) return 4;
    for (int value = 0; value < 16; ++value)
      if (!channel->nb_write(value)) return 5;
    if (channel->num_free() != 0 || channel->nb_write(99)) return 6;
  }
  sc_start(SC_ZERO_TIME);
  for (auto* channel : {&omitted, &explicit_size})
  {
    if (channel->num_available() != 16) return 7;
    int output = -1;
    int checksum = 0;
    for (int value = 0; value < 16; ++value)
    {
      if (!channel->nb_read(output) || output != value) return 8;
      checksum += output;
    }
    if (channel->nb_read(output) || output != 15 || checksum != 120 || channel->num_available() != 0) return 9;
  }
  sc_start(SC_ZERO_TIME);
  for (auto* channel : {&omitted, &explicit_size})
    if (channel->num_free() != 16 || !channel->nb_write(42)) return 10;
  std::cout << "DEFAULT_FIFO\t16\t120\t16\n";
  return 0;
}

/** @brief Observe two borrowed queues whose owners survive every native callback.
 * @details No Python references or GIL requirements; methods run on the kernel
 * thread and do not throw. The caller stops simulation before queue destruction.
 */
struct observer : sc_core::sc_module
{
  std::vector<std::pair<sc_dt::uint64, sc_dt::uint64>> generated_records;
  std::vector<std::pair<sc_dt::uint64, sc_dt::uint64>> explicit_records;
  SC_HAS_PROCESS(observer);
  /** @brief Register methods against borrowed default events.
   * @param name Module name consumed during construction.
   * @param generated Default-constructed queue owned by the caller.
   * @param explicit_queue Explicitly named queue owned by the caller.
   */
  observer(sc_core::sc_module_name name, sc_core::sc_event_queue& generated, sc_core::sc_event_queue& explicit_queue)
    : sc_module(name)
  {
    SC_METHOD(observe_generated);
    sensitive << generated.default_event();
    dont_initialize();
    SC_METHOD(observe_explicit);
    sensitive << explicit_queue.default_event();
    dont_initialize();
  }
  /** @brief Save the generated queue's delivery timestamp and delta. */
  void observe_generated()
  {
    generated_records.emplace_back(sc_core::sc_time_stamp().value(), sc_core::sc_delta_count());
  }
  /** @brief Save the explicitly named queue's delivery timestamp and delta. */
  void observe_explicit()
  {
    explicit_records.emplace_back(sc_core::sc_time_stamp().value(), sc_core::sc_delta_count());
  }
};

/** @brief Compare generated-name defaults, notifications and stopped teardown.
 * @return Zero if names, callback records and removed kernel objects match.
 * @details Unique pointers own both queues until after sc_stop; observer lives
 * through teardown and no callback executes after its borrowed events disappear.
 */
int run_queue()
{
  using namespace sc_core;
  auto generated = std::make_unique<sc_event_queue>();
  auto explicit_queue = std::make_unique<sc_event_queue>(sc_module_name(sc_gen_unique_name("event_queue")));
  const std::string generated_name = generated->name();
  const std::string explicit_name = explicit_queue->name();
  if (generated_name != "event_queue_0" || explicit_name != "event_queue_1") return 11;
  observer model("observer", *generated, *explicit_queue);
  for (auto* queue : {generated.get(), explicit_queue.get()})
  {
    queue->notify(sc_time(1, SC_NS));
    queue->notify(sc_time(3, SC_NS));
  }
  sc_start(4, SC_NS);
  if (model.generated_records != model.explicit_records || model.generated_records.size() != 2 ||
      model.generated_records[0].first != 1000 || model.generated_records[1].first != 3000 ||
      model.generated_records[0].second >= model.generated_records[1].second)
    return 12;
  sc_stop();
  generated.reset();
  explicit_queue.reset();
  if (sc_find_object(generated_name.c_str()) || sc_find_object(explicit_name.c_str())) return 13;
  std::cout << "DEFAULT_QUEUE\t1000\t3000\t2\t1\n";
  return 0;
}
}  // namespace

/** @brief Select one independent bounded default comparison.
 * @param argc Argument count including executable name.
 * @param argv Runner-owned scenario and policy strings.
 * @return Zero on asserted equivalence, nonzero on invalid setup or mismatch.
 */
int sc_main(int argc, char** argv)
{
  if (argc != 3) return 2;
  const std::string scenario(argv[1]);
  if (scenario == "fifo") return run_fifo();
  if (scenario == "queue") return run_queue();
  return run_start(scenario, argv[2]);
}
