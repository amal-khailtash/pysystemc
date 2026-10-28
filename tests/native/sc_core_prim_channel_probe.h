/** @file sc_core_prim_channel_probe.h
 * @brief Native derived probes for protected primitive-channel overloads.
 * @details The probes own no Python references and make no Python C API calls.
 * Protected scheduler calls are invoked only from native SystemC processes.
 */
#pragma once

#include <stdexcept>
#include <string>
#include <systemc>
#include <thread>
#include <vector>

namespace prim_channel_public_probe
{
/** @brief Expose protected constructors, lifecycle hooks and scheduler helpers.
 * @details SystemC owns registration while the kernel is active. Records are
 * borrowed from the enclosing test model and must outlive this channel.
 */
class Channel : public sc_core::sc_prim_channel
{
 public:
  /** @brief Construct a named test channel borrowing its output records. */
  explicit Channel(std::vector<std::string>& records) : sc_prim_channel("prim_probe"), records_(records)
  {
  }
  /** @brief Construct an unnamed channel through the protected native constructor. */
  Channel() : sc_prim_channel(), records_(empty_records_)
  {
  }
  /** @brief Track destruction through the native virtual base. */
  ~Channel() override
  {
    ++destroyed;
  }

  static int destroyed;

  /** @brief Invoke one protected wait overload in the active process. */
  void wait_form(unsigned form, const sc_core::sc_event& event, const sc_core::sc_event_or_list& or_list,
                 const sc_core::sc_event_and_list& and_list)
  {
    using namespace sc_core;
    switch (form)
    {
    case 0:
      wait();
      break;
    case 1:
      wait(event);
      break;
    case 2:
      wait(or_list);
      break;
    case 3:
      wait(and_list);
      break;
    case 4:
      wait(sc_time(1, SC_NS));
      break;
    case 5:
      wait(1.0, SC_NS);
      break;
    case 6:
      wait(sc_time(10, SC_NS), event);
      break;
    case 7:
      wait(10.0, SC_NS, event);
      break;
    case 8:
      wait(sc_time(10, SC_NS), or_list);
      break;
    case 9:
      wait(10.0, SC_NS, or_list);
      break;
    case 10:
      wait(sc_time(10, SC_NS), and_list);
      break;
    case 11:
      wait(10.0, SC_NS, and_list);
      break;
    case 12:
      wait(2);
      break;
    default:
      throw std::out_of_range("invalid primitive-channel wait form");
    }
  }

  /** @brief Invoke one protected next_trigger overload in an SC_METHOD. */
  void trigger_form(unsigned form, const sc_core::sc_event& event, const sc_core::sc_event_or_list& or_list,
                    const sc_core::sc_event_and_list& and_list)
  {
    using namespace sc_core;
    switch (form)
    {
    case 0:
      next_trigger();
      break;
    case 1:
      next_trigger(event);
      break;
    case 2:
      next_trigger(or_list);
      break;
    case 3:
      next_trigger(and_list);
      break;
    case 4:
      next_trigger(sc_time(1, SC_NS));
      break;
    case 5:
      next_trigger(1.0, SC_NS);
      break;
    case 6:
      next_trigger(sc_time(10, SC_NS), event);
      break;
    case 7:
      next_trigger(10.0, SC_NS, event);
      break;
    case 8:
      next_trigger(sc_time(10, SC_NS), or_list);
      break;
    case 9:
      next_trigger(10.0, SC_NS, or_list);
      break;
    case 10:
      next_trigger(sc_time(10, SC_NS), and_list);
      break;
    case 11:
      next_trigger(10.0, SC_NS, and_list);
      break;
    default:
      throw std::out_of_range("invalid primitive-channel next_trigger form");
    }
  }

  /** @brief Query timeout status from the active process. */
  bool protected_timed_out()
  {
    return timed_out();
  }

 protected:
  /** @brief Record before-end-of-elaboration ordering. */
  void before_end_of_elaboration() override
  {
    records_.push_back("B");
  }
  /** @brief Record end-of-elaboration ordering. */
  void end_of_elaboration() override
  {
    records_.push_back("E");
  }
  /** @brief Record start-of-simulation ordering. */
  void start_of_simulation() override
  {
    records_.push_back("S");
  }
  /** @brief Record end-of-simulation ordering. */
  void end_of_simulation() override
  {
    records_.push_back("F");
  }
  /** @brief Count updates and capture the public queue-state query. */
  void update() override
  {
    ++updates;
    update_requested_during_update = update_requested();
  }

 public:
  unsigned updates = 0;
  bool update_requested_during_update = false;

 private:
  std::vector<std::string>& records_;
  inline static std::vector<std::string> empty_records_{};
};

inline int Channel::destroyed = 0;

/** @brief Request a native channel update from a real host thread.
 * @details The thread only calls the native async API. It owns no Python state
 * and is joined before channel destruction.
 */
class AsyncChannel : public sc_core::sc_prim_channel
{
 public:
  /** @brief Attach a named channel and retain its event and lifecycle records. */
  AsyncChannel(sc_core::sc_event& ready, std::vector<std::string>& records)
    : sc_prim_channel("async_prim_probe"), ready_(ready), records_(records)
  {
    attached = async_attach_suspending();
  }
  /** @brief Join outstanding host work before unregistering this channel. */
  ~AsyncChannel() override
  {
    if (worker_.joinable()) worker_.join();
    ++destroyed;
  }
  static int destroyed;
  bool attached = false;
  bool detached = false;
  unsigned updates = 0;
  std::thread worker_;

 protected:
  /** @brief Count one async update and notify the native observer. */
  void update() override
  {
    ++updates;
    ready_.notify(sc_core::SC_ZERO_TIME);
  }
  /** @brief Record before-end-of-elaboration ordering. */
  void before_end_of_elaboration() override
  {
    records_.push_back("B");
  }
  /** @brief Record end-of-elaboration ordering. */
  void end_of_elaboration() override
  {
    records_.push_back("E");
  }
  /** @brief Record start-of-simulation ordering without entering Python. */
  void start_of_simulation() override
  {
    records_.push_back("S");
  }
  /** @brief Record end-of-simulation ordering. */
  void end_of_simulation() override
  {
    records_.push_back("F");
  }

 public:
  /** @brief Start a delayed native host-thread update request. */
  void start_request()
  {
    worker_ = std::thread(
      [this]
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        async_request_update();
      });
  }
  /** @brief Detach this channel from suspending async simulation. */
  bool detach()
  {
    detached = async_detach_suspending();
    return detached;
  }

 private:
  sc_core::sc_event& ready_;
  std::vector<std::string>& records_;
};

inline int AsyncChannel::destroyed = 0;

/** @brief Request a borrowed channel update from a native host thread.
 * @details The channel must outlive this requester and its joined worker. The
 * worker calls only the native SystemC API and never enters the Python C API.
 */
class HostRequester
{
 public:
  /** @brief Borrow the channel that will receive a native host-thread request.
   * @param channel Channel retained by the caller until join completes.
   */
  explicit HostRequester(sc_core::sc_prim_channel& channel) : channel_(channel)
  {
  }
  /** @brief Join the worker before releasing its borrowed channel. */
  ~HostRequester()
  {
    join();
  }
  /** @brief Start one bounded worker that calls only the native async API. */
  void start()
  {
    worker_ = std::thread(
      [this]
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        channel_.async_request_update();
      });
  }
  /** @brief Join the worker if it has been started. */
  void join()
  {
    if (worker_.joinable()) worker_.join();
  }

 private:
  sc_core::sc_prim_channel& channel_;
  std::thread worker_;
};
}  // namespace prim_channel_public_probe
