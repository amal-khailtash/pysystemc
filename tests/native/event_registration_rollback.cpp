/** @file event_registration_rollback.cpp
 * @brief Successful native wait/next_trigger controls for rollback scenarios.
 * @details This model never injects an exception. Each invocation checks one
 * process kind and OR/AND expression using ordinary SystemC registration, then
 * records OR/AND event winners at 1/2 ns and a timeout winner at 3 ns. The
 * CTHREAD clock is 1 ns; in this schedule its native wait_cycles alignment
 * lands on the same timestamps as the event and timeout winners. The clock,
 * events, lists and process callbacks remain owned through the run.
 */
#include <string>
#include <systemc>
#include <utility>

namespace
{

using namespace sc_core;

/** @brief Run one successful native event-list control case. */
class NativeControl : public sc_module
{
 public:
  SC_HAS_PROCESS(NativeControl);

  /** @brief Construct the requested process and its deterministic stimulus.
   * @param name Module name borrowed by the native hierarchy.
   * @param process_kind One of thread, cthread, or method.
   * @param expression_kind One of OR or AND.
   * @param timed Whether the first successful registration has a timeout.
   * @throws std::invalid_argument For malformed case arguments.
   * @details SystemC owns scheduling; the module retains all source events and
   * the CTHREAD clock through completion. No Python callback or GIL is involved.
   */
  NativeControl(sc_module_name name, std::string process_kind, std::string expression_kind, bool timed)
    : sc_module(name)
    , process_kind_(std::move(process_kind))
    , expression_kind_(std::move(expression_kind))
    , timed_(timed)
  {
    if (process_kind_ != "thread" && process_kind_ != "cthread" && process_kind_ != "method")
      throw std::invalid_argument("unknown process kind");
    if (expression_kind_ != "OR" && expression_kind_ != "AND") throw std::invalid_argument("unknown expression kind");

    clock_port_(clock_);
    if (process_kind_ == "thread")
    {
      SC_THREAD(worker);
    }
    else if (process_kind_ == "cthread")
    {
      SC_CTHREAD(worker, clock_port_.pos());
    }
    else
    {
      SC_METHOD(method_worker);
      sensitive << kick_;
      dont_initialize();
    }
    SC_THREAD(driver);
  }

  /** @brief First successful event winner elapsed from registration. */
  sc_time event_elapsed = SC_ZERO_TIME;
  /** @brief Timed second-wait winner elapsed from its registration. */
  sc_time timeout_elapsed = SC_ZERO_TIME;
  bool valid = false;

 private:
  std::string process_kind_;
  std::string expression_kind_;
  bool timed_;
  sc_clock clock_{"clock", sc_time(1, SC_NS)};
  sc_in<bool> clock_port_{"clock_port"};
  sc_event first_{"first"};
  sc_event second_{"second"};
  sc_event late_first_{"late_first"};
  sc_event late_second_{"late_second"};
  sc_event previous_{"previous"};
  sc_event kick_{"kick"};
  sc_event ready_{"ready"};
  sc_event done_{"done"};
  sc_time first_start_ = SC_ZERO_TIME;
  sc_time timeout_start_ = SC_ZERO_TIME;
  int method_phase_ = 0;

  /** @brief Perform native expression waits and check both result intervals. */
  void worker()
  {
    ready_.notify(SC_ZERO_TIME);
    first_start_ = sc_time_stamp();
    wait_for_first();
    event_elapsed = sc_time_stamp() - first_start_;
    timeout_start_ = sc_time_stamp();
    wait_for_timeout();
    timeout_elapsed = sc_time_stamp() - timeout_start_;
    valid = expected_event_elapsed() == event_elapsed && sc_time(3, SC_NS) == timeout_elapsed;
    done_.notify(SC_ZERO_TIME);
  }

  /** @brief Perform ordinary native OR/AND event wait registration. */
  void wait_for_first()
  {
    if (expression_kind_ == "OR")
    {
      if (timed_)
        wait(sc_time(5, SC_NS), first_ | second_);
      else
        wait(first_ | second_);
    }
    else
    {
      if (timed_)
        wait(sc_time(5, SC_NS), first_ & second_);
      else
        wait(first_ & second_);
    }
  }

  /** @brief Perform a native timed AND/OR wait whose timeout wins. */
  void wait_for_timeout()
  {
    if (expression_kind_ == "OR")
      wait(sc_time(3, SC_NS), late_first_ | late_second_);
    else
      wait(sc_time(3, SC_NS), late_first_ & late_second_);
  }

  /** @brief Return the expected event-winner delay for the selected operator. */
  sc_time expected_event_elapsed() const
  {
    return expression_kind_ == "OR" ? sc_time(1, SC_NS) : sc_time(2, SC_NS);
  }

  /** @brief Register two ordinary native dynamic triggers from one method. */
  void method_worker()
  {
    if (method_phase_ == 0)
    {
      method_phase_ = 1;
      next_trigger(previous_);
      ready_.notify(SC_ZERO_TIME);
      return;
    }
    if (method_phase_ == 1)
    {
      method_phase_ = 2;
      first_start_ = sc_time_stamp();
      if (expression_kind_ == "OR")
      {
        if (timed_)
          next_trigger(sc_time(5, SC_NS), first_ | second_);
        else
          next_trigger(first_ | second_);
      }
      else
      {
        if (timed_)
          next_trigger(sc_time(5, SC_NS), first_ & second_);
        else
          next_trigger(first_ & second_);
      }
      ready_.notify(SC_ZERO_TIME);
      return;
    }
    if (method_phase_ == 2)
    {
      event_elapsed = sc_time_stamp() - first_start_;
      timeout_start_ = sc_time_stamp();
      method_phase_ = 3;
      if (expression_kind_ == "OR")
        next_trigger(sc_time(3, SC_NS), late_first_ | late_second_);
      else
        next_trigger(sc_time(3, SC_NS), late_first_ & late_second_);
      return;
    }

    timeout_elapsed = sc_time_stamp() - timeout_start_;
    valid = expected_event_elapsed() == event_elapsed && sc_time(3, SC_NS) == timeout_elapsed;
    done_.notify(SC_ZERO_TIME);
  }

  /** @brief Drive event winners first and late events after the timeout. */
  void driver()
  {
    if (process_kind_ == "method")
    {
      kick_.notify(SC_ZERO_TIME);
      wait(ready_);
      previous_.notify(sc_time(1, SC_NS));
      wait(ready_);
      first_.notify(sc_time(1, SC_NS));
      second_.notify(sc_time(2, SC_NS));
      late_first_.notify(sc_time(8, SC_NS));
      late_second_.notify(sc_time(9, SC_NS));
    }
    else
    {
      wait(ready_);
      first_.notify(sc_time(1, SC_NS));
      second_.notify(sc_time(2, SC_NS));
      late_first_.notify(sc_time(8, SC_NS));
      late_second_.notify(sc_time(9, SC_NS));
    }
    wait(done_);
    sc_stop();
  }
};

}  // namespace

/** @brief Run and print one independent native reference case. */
int sc_main(int argc, char** argv)
{
  if (argc != 4) return 2;
  const std::string process_kind = argv[1];
  const std::string expression_kind = argv[2];
  const std::string timed_arg = argv[3];
  if (timed_arg != "timed" && timed_arg != "untimed") return 2;

  NativeControl model("event_rollback_native", process_kind, expression_kind, timed_arg == "timed");
  sc_start();
  if (!model.valid)
  {
    std::cerr << "EVENT_REGISTRATION_ROLLBACK_NATIVE_INVALID " << process_kind << " " << expression_kind << " "
              << timed_arg << " event=" << model.event_elapsed.to_string()
              << " timeout=" << model.timeout_elapsed.to_string() << "\n";
    return 1;
  }
  std::cout << "EVENT_REGISTRATION_ROLLBACK_NATIVE " << process_kind << " " << expression_kind << " " << timed_arg
            << " event=" << model.event_elapsed.to_string() << " timeout=" << model.timeout_elapsed.to_string() << "\n";
  return 0;
}
