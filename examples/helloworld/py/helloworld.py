"""Finite native method example with verified callback and lifecycle ordering."""

from amal.eda.systemc import main, sc_core


def hello1():
    """Print the direct Python greeting."""
    print("\nHello world using approach 1")


class HelloWorld(sc_core.sc_module):
    """Retain a method callback and record native lifecycle order."""

    def __init__(self, name: str):
        """Register the method within a live native module-name scope."""
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)
        self.events = []

        self.sc_method("hello2", self.hello2)

    def hello2(self):
        """Record tick and print its native timestamp."""
        self.events.append("tick")
        print("-" * 80)
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - hello2()")
        print("Hello world using approach 3")

    def before_end_of_elaboration(self):
        """Record before and print its native timestamp."""
        self.events.append("before")
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Before End of Elaboration")

    def end_of_elaboration(self):
        """Record elaborated and print its native timestamp."""
        self.events.append("elaborated")
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Elaboration finished")

    def start_of_simulation(self):
        """Record start and print its native timestamp."""
        self.events.append("start")
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Simulation started")

    def end_of_simulation(self):
        """Record end and print its native timestamp."""
        self.events.append("end")
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Simulation finished")


def sc_main(args: list) -> int:
    """Run one bounded simulation and assert callback/lifecycle order."""
    hello1()

    model = HelloWorld("helloworld")  # Retain the native process host.

    sc_core.sc_start(1, sc_core.SC_NS)
    sc_core.sc_stop()
    assert model.events == ["before", "elaborated", "start", "tick", "end"]

    return 0


if __name__ == "__main__":
    main()
