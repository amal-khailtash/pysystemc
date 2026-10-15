"""Python wiring port of SystemC 2.3.4's hierarchical sc_export example.

Adapted from Accellera's examples/sysc/2.1/sc_export/main.cpp (Apache-2.0;
see the upstream LICENSE and NOTICE). Native interfaces/channels are retained
by their enclosing Python modules until simulation ends.
"""

from amal.eda.systemc import main, sc_core as sc
import cppyy

cppyy.cppdef("""

namespace export_example
{

/** @brief Native interface exported through the Python module hierarchy. */
class ChannelInterface : virtual public sc_core::sc_interface
{
 public:
  virtual void run() = 0;
};

/** @brief Native channel; calls execute in C++ without acquiring the GIL. */
class Channel : public ChannelInterface, public sc_core::sc_channel
{
 public:
  explicit Channel(sc_core::sc_module_name name) : sc_core::sc_channel(name) {}
  /** @brief Print the time of interface invocation. */
  void run() override { std::cout << sc_core::sc_time_stamp() << " In Channel run()" << std::endl; }
};

}  // namespace export_example
""")
interface = cppyy.gbl.export_example.ChannelInterface
channel = cppyy.gbl.export_example.Channel


class D(sc.sc_module):
    """Export an owned native channel through one interface export."""

    def __init__(self, name: str):
        """Create the channel and bind its interface to IFP."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.IFP = sc.sc_export[interface]("IFP")
        self.channel = channel("C")
        self.IFP(self.channel)


class E(sc.sc_module):
    """Export a local channel and a child's export through two interfaces."""

    def __init__(self, name: str):
        """Retain channel, child module and hierarchical interface bindings."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.channel = channel("C")
        self.child = D("D")
        self.IFP1 = sc.sc_export[interface]("IFP1")
        self.IFP2 = sc.sc_export[interface]("IFP2")
        self.IFP1(self.channel)
        self.IFP2(self.child.IFP.get_interface())


class X(sc.sc_module):
    """Call exported native channel interfaces from a Python thread."""

    def __init__(self, name: str):
        """Register a finite thread and two interface ports."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.P1 = sc.sc_port[interface]("P1")
        self.P2 = sc.sc_port[interface]("P2")
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
