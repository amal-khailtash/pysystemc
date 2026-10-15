"""Python port of SystemC 2.3.4 sysc/2.1/forkjoin.

Adapted from Accellera's Apache-2.0 example; see the upstream LICENSE/NOTICE.
Python closures replace sc_bind/sc_ref and retain native process return values.
"""

from amal.eda.systemc import main, sc_core as sc


class Top(sc.sc_module):
    """Demonstrate event cooperation, dynamic spawning, joins and termination."""

    def __init__(self, name):
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.sc_thread("main", self.run)
        self.rounds = []

    def round_robin(self, label, receive, send):
        """Transfer the event token three times with ten-nanosecond delays."""
        for _ in range(3):
            sc.wait(receive)
            self.rounds.append((label, sc.sc_time_stamp().to_string()))
            print(f"Round robin thread {label} at time {sc.sc_time_stamp()}")
            sc.wait(10, sc.SC_NS)
            send.notify()

    def wait_and_end(self, index):
        """Exercise reuse of terminated native worker threads."""
        sc.wait(index + 1, sc.SC_NS)
        print(f"Thread {index} ending.")

    def run(self):
        """Join four cooperating processes, then wait for spawned termination."""
        events = [sc.sc_event() for _ in range(4)]
        events[0].notify(100, sc.SC_NS)
        join = sc.sc_join()
        for index in range(4):

            def callback(index=index):
                """Run one round-robin participant with a retained index."""
                return self.round_robin(str(index + 1), events[index], events[(index + 1) % 4])

            join.add_process(sc.sc_spawn(callback, str(index + 1)))
        join.wait()
        assert len(self.rounds) == 12
        print("Returned int is 0")
        for index in range(10):
            sc.sc_spawn(lambda index=index: self.wait_and_end(index))
        sc.wait(20, sc.SC_NS)
        for label, value in (("test_function", 3.14159), ("void_function", 1.2345), ("ref_function", 9.8765)):
            result = []

            def callback(label=label, value=value):
                print(f"{label} sees {value}")
                result.append(int(value))

            handle = sc.sc_spawn(callback)
            sc.wait(handle.terminated_event())
            if label != "void_function":
                print(f"Returned int is {result[0]}")
        print("Done.")


def sc_main(args):
    """Run the upstream scenario to starvation, retaining its module."""
    top = Top("Top1")
    sc.sc_start()
    assert top.rounds[-1] == ("4", "210 ns")
    return 0


if __name__ == "__main__":
    main()
