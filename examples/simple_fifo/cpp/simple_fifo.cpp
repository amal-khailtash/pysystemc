/** @file simple_fifo.cpp
 *  @brief Run a finite producer/consumer example using the native SystemC FIFO.
 */

#include <cassert>
#include <iostream>
#include <string>
#include <systemc>

#include "hierarchy.h"

namespace
{

/** @brief Message transferred through the bounded FIFO by both processes. */
const std::string MESSAGE = "Visit www.accellera.org and see what SystemC can do for you today!\n";

/** @brief Write each message byte to the connected FIFO. */
class Producer : public sc_core::sc_module
{
 public:
  sc_core::sc_fifo_out<int> output{"output"};

  /** @brief Construct the output port and register the producer thread.
   *  @param name Native hierarchy name retained by SystemC.
   */
  explicit Producer(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
    SC_THREAD(produce);
  }

  /** @brief Write all message bytes as integer values to the FIFO. */
  void produce()
  {
    for (const unsigned char character : MESSAGE) output.write(static_cast<int>(character));
  }

  SC_HAS_PROCESS(Producer);
};

/** @brief Read, verify, and print each message byte from the connected FIFO. */
class Consumer : public sc_core::sc_module
{
 public:
  sc_core::sc_fifo_in<int> input{"input"};
  std::string received;

  /** @brief Construct the input port and register the consumer thread.
   *  @param name Native hierarchy name retained by SystemC.
   */
  explicit Consumer(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
    SC_THREAD(consume);
  }

  /** @brief Verify each value, retain the message, print it, and stop the kernel. */
  void consume()
  {
    for (const unsigned char expected : MESSAGE)
    {
      const int value = input.read();
      assert(value == static_cast<int>(expected));
      received.push_back(static_cast<char>(value));
    }
    assert(received == MESSAGE);
    std::cout << received;
    sc_core::sc_stop();
  }

  SC_HAS_PROCESS(Consumer);
};

/** @brief Own and connect the bounded FIFO and its producer/consumer modules. */
class Top : public sc_core::sc_module
{
 public:
  sc_core::sc_fifo<int> fifo;
  Producer producer;
  Consumer consumer;

  /** @brief Create all hierarchy children and bind both FIFO ports.
   *  @param name Native hierarchy name retained by SystemC.
   */
  explicit Top(sc_core::sc_module_name name)
    : sc_core::sc_module(name), fifo("fifo", 10), producer("producer"), consumer("consumer")
  {
    producer.output(fifo);
    consumer.input(fifo);
  }
};

}  // namespace

/** @brief Run the finite FIFO simulation and verify its complete readback.
 *  @param argc Native argument count supplied by the SystemC runner.
 *  @param argv Native argument values supplied by the SystemC runner.
 *  @return Zero after the consumer has received every expected byte.
 */
int sc_main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  Top top("top");
  print_hierarchy();
  sc_core::sc_start();
  assert(top.consumer.received == MESSAGE);
  assert(sc_core::sc_time_stamp() == sc_core::SC_ZERO_TIME);
  return 0;
}
