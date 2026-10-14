"""A finite producer/consumer example using the native SystemC FIFO."""
from amal.eda.systemc import main, sc_core

# Blocking native FIFO calls must release the GIL so other processes can run.
sc_core.sc_fifo_out[int].write.__release_gil__ = True
sc_core.sc_fifo_in[int].read.__release_gil__ = True

MESSAGE = "Visit www.accellera.org and see what SystemC can do for you today!\n"


class Producer(sc_core.sc_module):
    def __init__(self, name):
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)
        self.out = sc_core.sc_fifo_out[int]("out")
        self.sc_thread("produce", self.produce)

    def produce(self):
        for character in MESSAGE:
            self.out.write(ord(character))


class Consumer(sc_core.sc_module):
    def __init__(self, name):
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)
        self.input = sc_core.sc_fifo_in[int]("input")
        self.received = ""
        self.sc_thread("consume", self.consume)

    def consume(self):
        for _ in MESSAGE:
            self.received += chr(self.input.read())
        assert self.received == MESSAGE
        print(self.received, end="")
        sc_core.sc_stop()


class Top(sc_core.sc_module):
    def __init__(self, name):
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)
        self.fifo = sc_core.sc_fifo[int]("fifo", 10)
        self.producer = Producer("producer")
        self.consumer = Consumer("consumer")
        self.producer.out(self.fifo)
        self.consumer.input(self.fifo)


def sc_main(args):
    top = Top("top")
    sc_core.sc_start()
    assert top.consumer.received == MESSAGE
    return 0


if __name__ == "__main__":
    main()
