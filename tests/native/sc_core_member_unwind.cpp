/** @file sc_core_member_unwind.cpp
 * @brief Native RAII reference for reset/kill while member waits are suspended.
 * @details Kernel processes borrow owners/events throughout waits; RAII cleanup
 * runs on scheduler unwind. No Python callbacks or GIL operations occur.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <iostream>
#include <string>
#include <systemc>
#include <vector>

using namespace sc_core;

/** @brief Expose native module waits without replacing their context. */
struct Module : sc_module
{
  /** @brief Construct a live named module owner. */
  explicit Module(sc_module_name name) : sc_module(name)
  {
  }
  using sc_module::wait;
};
/** @brief Expose native primitive-channel waits. */
struct Channel : sc_prim_channel
{
  /** @brief Register the native channel for the simulation lifetime. */
  explicit Channel(const char* name) : sc_prim_channel(name)
  {
  }
  using sc_prim_channel::wait;
};
/** @brief Emit the current native scheduler time for one lifecycle action. */
void record(const std::string& label, const char* action)
{
  std::cout << "MEMBER_UNWIND " << label << " " << action << " " << sc_time_stamp().value() << "\n";
}

/** @brief Native member-halt RAII reference with a borrowed clock interface. */
struct HaltModule : sc_module
{
  sc_clock clock;
  sc_in<bool> clock_port;
  sc_process_handle handle;
  SC_HAS_PROCESS(HaltModule);
  /** @brief Configure the native clocked process before its first execution. */
  explicit HaltModule(sc_module_name name)
    : sc_module(name), clock("clock", sc_time(2, SC_NS)), clock_port("clock_port")
  {
    clock_port(clock);
    SC_CTHREAD(run, clock_port.pos());
    handle = sc_get_last_created_process_handle();
    set_stack_size(2 * 1024 * 1024);
  }
  /** @brief Record C++ cleanup when native halt terminates at the next edge. */
  void run()
  {
    wait(1);
    std::cout << "MEMBER_HALT start " << sc_time_stamp().value() << '\n';
    struct Cleanup
    {
      ~Cleanup()
      {
        std::cout << "MEMBER_HALT cleanup " << sc_time_stamp().value() << '\n';
      }
    } cleanup;
    halt();
    throw std::runtime_error("native halt returned normally");
  }
};
/** @brief Run RAII cleanup on native reset/kill during a borrowed member wait. */
template <class Owner>
void worker(Owner& owner, const std::string& label, int mode, sc_event& first, sc_event& second)
{
  record(label, "start");
  struct Cleanup
  {
    std::string label;
    ~Cleanup()
    {
      record(label, "cleanup");
    }
  } cleanup{label};
  if (mode == 0)
    owner.wait(sc_time(20, SC_NS));
  else if (mode == 1)
    owner.wait(first | second);
  else
    owner.wait(first & second);
  throw std::runtime_error("blocked member returned");
}
/** @brief Compare reset/kill cleanup across all borrowed member wait forms. */
int sc_main(int, char**)
{
  Module module("top");
  Channel channel("channel");
  HaltModule halt_module("halt_top");
  sc_event first("first"), second("second");
  std::vector<sc_process_handle> handles;
  for (int mode = 0; mode != 3; ++mode)
  {
    auto label = "module" + std::to_string(mode);
    handles.push_back(sc_spawn([&, mode, label] { worker(module, label, mode, first, second); }, label.c_str()));
  }
  for (int mode = 0; mode != 3; ++mode)
  {
    auto label = "channel" + std::to_string(mode);
    handles.push_back(sc_spawn([&, mode, label] { worker(channel, label, mode, first, second); }, label.c_str()));
  }
  auto control = sc_spawn(
    [&]
    {
      wait(2, SC_NS);
      for (auto& handle : handles) handle.reset();
      wait(1, SC_NS);
      for (auto& handle : handles) handle.kill();
    },
    "controller");
  sc_start(6, SC_NS);
  for (const auto& handle : handles)
    if (!handle.terminated()) return 1;
  if (!control.terminated()) return 2;
  if (!halt_module.handle.terminated()) return 3;
  sc_stop();
  return 0;
}
