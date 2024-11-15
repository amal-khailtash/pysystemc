import sys

from amal.eda.systemc import main, sc_core


def hello1():
    print("Hello world using approach 1")


class HelloWorld(sc_core.sc_module):

    def __init__(self, name: str):
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)

        self.sc_method("hello2", self.hello2)

        self.hello2()

    def hello2(self):
        print("-" * 80)
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - hello2()")
        print("Hello world using approach 2")

    def before_end_of_elaboration(self):
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Before End of Elaboration")

    def end_of_elaboration(self):
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Elaboration finished")

    def start_of_simulation(self):
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Simulation started")

    def end_of_simulation(self):
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Simulation finished")


def sc_main(args: list) -> int:
    hello1()

    helloworld = HelloWorld("helloworld")

    sc_core.sc_start()

    return 0


if __name__ == "__main__":
    main()
