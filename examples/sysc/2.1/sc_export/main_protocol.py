"""Protocol implementation of SystemC 2.3.4's hierarchical sc_export example.

Adapted from Accellera's examples/sysc/2.1/sc_export/main.cpp (Apache-2.0;
see the upstream LICENSE and NOTICE). Generated native adapters and Python implementations are retained
by their enclosing Python modules until simulation ends.

Original source: examples/sysc/2.1/sc_export/main.cpp
"""

from amal.eda.systemc import main, sc_core as sc
from typing import Protocol


@sc.interface(methods={"run": ("void", "")})
class C_if(Protocol):
    """Structural contract exported through the module hierarchy."""

    def run(self) -> None:
        """Report the invocation time without waiting."""
        ...


class C:
    """Implement the exported interface using ordinary Python methods."""

    def __init__(self, name):
        """Retain the upstream channel identity for the Python model."""
        self.name = name

    def run(self):
        """Print the time of interface invocation."""
        print(f"{sc.sc_time_stamp()} In Channel run()", flush=True)


class D(sc.sc_module):
    """Export an owned Python channel through one interface export."""

    def __init__(self, name: str):
        """Create the channel and bind its interface to IFP."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.IFP = sc.sc_export[C_if]("IFP")
        self.channel = C("C")
        self.IFP.bind(self.channel)


class E(sc.sc_module):
    """Export a local channel and a child's export through two interfaces."""

    def __init__(self, name: str):
        """Retain channel, child module and hierarchical interface bindings."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.channel = C("C")
        self.child = D("D")
        self.IFP1 = sc.sc_export[C_if]("IFP1")
        self.IFP2 = sc.sc_export[C_if]("IFP2")
        self.IFP1.bind(self.channel)
        self.IFP2.bind(self.child.IFP.get_interface())


class X(sc.sc_module):
    """Call exported Python channel interfaces from a Python thread."""

    def __init__(self, name: str):
        """Register a finite thread and two interface ports."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.P1 = sc.sc_port[C_if]("P1")
        self.P2 = sc.sc_port[C_if]("P2")
        self.sc_thread("run", self.run)

    def run(self):
        """Call interfaces after native waits at 10 and 20 ns."""
        sc.wait(10, sc.SC_NS)
        self.P1.get_interface().run()
        sc.wait(10, sc.SC_NS)
        self.P2.get_interface().run()


def sc_main(args: list[str]) -> int:
    """Bind the hierarchy and preserve the native reference's bounded starts."""
    provider = E("E")
    consumer = X("X")
    consumer.P1(provider.IFP1.get_interface())
    consumer.P2(provider.IFP2.get_interface())
    sc.sc_start(17, sc.SC_NS)
    provider.IFP1.get_interface().run()
    sc.sc_start(50, sc.SC_NS)
    assert sc.sc_time_stamp() == sc.sc_time(67, sc.SC_NS)
    return 0


if __name__ == "__main__":
    main()
