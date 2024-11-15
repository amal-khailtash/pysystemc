# from abc import ABC, abstractmethod
import sys
from typing import Protocol

from amal.eda.systemc import main, sc_core


# Define a common metaclass
# class CommonMeta(type(sc_core.sc_interface), type):
class CommonMeta(type(sc_core.sc_channel), type):
    pass


# class write_if(Protocol):
#     def write(self, c: str) -> None:
#         ...

#     def reset(self) -> None:
#         ...


# class read_if(Protocol):
#     def read(self, c: str) -> None:
#         ...

#     def num_available(self) -> int:
#         ...

# class write_if(sc_core.sc_interface, ABC):
#     @abstractmethod
#     def write(self, c: str) -> None:
#         pass

#     @abstractmethod
#     def reset(self) -> None:
#         pass


# class read_if(sc_core.sc_interface, ABC):
#     @abstractmethod
#     def read(self, c: str) -> None:
#         pass

#     @abstractmethod
#     def num_available(self) -> int:
#         pass


# class write_if(sc_core.sc_interface):
class write_if(metaclass=CommonMeta):
    def write(self, c: str) -> None:
        raise NotImplementedError

    def reset(self) -> None:
        raise NotImplementedError


# class read_if(sc_core.sc_interface):
class read_if(metaclass=CommonMeta):
    def read(self, c: str) -> None:
        raise NotImplementedError

    def num_available(self) -> int:
        raise NotImplementedError


MAX = 10    # Maximum number of elements in the FIFO

# class Fifo(sc_core.sc_channel, write_if, read_if):
# class Fifo(sc_core.sc_channel, write_if, read_if, metaclass=CommonMeta):
class Fifo(sc_core.sc_channel):
    def __init__(self, name: str):
        # module_name = sc_core.sc_module_name(name)
        # super().__init__(module_name)
        # super().__init__(name)

        self.num_elements = 0
        self.first = 0
        self.data = [0] * MAX
        self.write_event = sc_core.sc_event()
        self.read_event = sc_core.sc_event()

    def write(self, c: str) -> None:
        if self.num_elements == MAX:
            self.wait(self.read_event)

        self.data[(self.first + self.num_elements) % MAX] = c
        self.num_elements += 1
        self.write_event.notify()

    def read(self, c: str) -> None:
        if self.num_elements == 0:
            self.wait(self.write_event)

        c = self.data[self.first]
        self.num_elements -= 1
        self.first = (self.first + 1) % MAX
        self.read_event.notify()

    def reset(self) -> None:
        self.num_elements = 0
        self.first = 0

    def num_available(self) -> int:
        return self.num_elements


class Producer(sc_core.sc_module):
    def __init__(self, name: str):
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)

        self.out_ = sc_core.sc_port(write_if)("out")

        self.sc_thread("main", self.main)

    def main(self):
        str = "Visit www.accellera.org and see what SystemC can do for you today!\n"

        while str:
            self.out_.write(str)
            str += 1


class Consumer(sc_core.sc_module):
    def __init__(self, name: str):
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)

        self.in_ = sc_core.sc_port(read_if)("in")

        self.sc_thread("main", self.main)

    def main(self):
        c = ""
        print("\n\n")

        while True:
            self.in_.read(c)
            print(c, end="")

            if self.in_.num_available() == 1:
                print("<1>", end="")
            if self.in_.num_available() == 9:
                print("<9>", end="")


class Top(sc_core.sc_module):

    def __init__(self, name: str):
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)

        self.fifo_inst = Fifo("Fifo1")

        self.prod_inst = Producer("Producer1")
        self.prod_inst.out_(self.fifo_inst)

        self.cons_inst = Consumer("Consumer1")
        self.cons_inst.in_(self.fifo_inst)


def sc_main(args: list) -> int:

    top = Top("Top1")

    sc_core.sc_start()

    return 0


if __name__ == "__main__":
    main()
