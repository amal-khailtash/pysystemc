"""A finite producer/consumer example using the native SystemC FIFO."""

from amal.eda.systemc import main, print_hierarchy, sc_core

MESSAGE = "Visit www.accellera.org and see what SystemC can do for you today!\n"


class Producer(sc_core.sc_module):
    """Write the finite example message through a native FIFO output port."""

    def __init__(self, name):
        """Create the output port and register the producer thread."""
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)
        self.out = sc_core.sc_fifo_out[int]("out")
        self.sc_thread("produce", self.produce)

    def produce(self):
        """Send each message character as an integer FIFO value."""
        for character in MESSAGE:
            self.out.write(ord(character))


class Consumer(sc_core.sc_module):
    """Read and print the finite example message from a native FIFO port."""

    def __init__(self, name):
        """Create the input port and retain received values for verification."""
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)
        self.input = sc_core.sc_fifo_in[int]("input")
        self.received = ""
        self.sc_thread("consume", self.consume)

    def consume(self):
        """Read every expected character, assert the value sequence and stop."""
        for _ in MESSAGE:
            self.received += chr(self.input.read())
        assert self.received == MESSAGE
        print(self.received, end="")
        sc_core.sc_stop()


class Top(sc_core.sc_module):
    """Own the FIFO, producer and consumer under one native top module."""

    def __init__(self, name):
        """Construct child modules and bind both ports to the FIFO channel."""
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)
        self.fifo = sc_core.sc_fifo[int]("fifo", 10)
        self.producer = Producer("producer")
        self.consumer = Consumer("consumer")
        self.producer.out(self.fifo)
        self.consumer.input(self.fifo)


def sc_main(args):
    """Print the native hierarchy, run the FIFO threads and verify readback.

    Args:
        args: Native command-line arguments passed by the SystemC bridge.

    Returns:
        Zero after the consumer receives the complete message.
    """
    top = Top("top")
    print_hierarchy()
    sc_core.sc_start()
    assert top.consumer.received == MESSAGE
    assert sc_core.sc_time_stamp() == sc_core.SC_ZERO_TIME
    return 0


if __name__ == "__main__":
    main()
