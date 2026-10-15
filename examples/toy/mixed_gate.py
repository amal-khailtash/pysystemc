"""Drive a native C++ AND gate from Python and verify its complete truth table."""
from amal.eda.systemc import main, sc_core as sc
import cppyy

cppyy.cppdef('''


/** @brief Native combinational gate; ports bind to Python-owned signals.
 * SystemC owns scheduling. No Python callback or GIL is needed by evaluate().
 */
struct ToyAndGate : sc_core::sc_module
{
  sc_core::sc_in<bool> a, b;
  sc_core::sc_out<bool> y;
  SC_HAS_PROCESS(ToyAndGate);

  explicit ToyAndGate(sc_core::sc_module_name name) : sc_module(name)
  {
    SC_METHOD(evaluate);
    sensitive << a << b;
  }

  /** @brief Update the output in the next native signal update phase. */
  void evaluate() { y.write(a.read() && b.read()); }
};
''')


def sc_main(args: list[str]) -> int:
    """Bind native ports, drive all input pairs and compare native outputs."""
    a = sc.sc_signal[bool]('a')
    b = sc.sc_signal[bool]('b')
    y = sc.sc_signal[bool]('y')
    gate = cppyy.gbl.ToyAndGate('gate')
    gate.a(a)
    gate.b(b)
    gate.y(y)
    for left, right in ((False, False), (False, True), (True, False), (True, True)):
        a.write(left)
        b.write(right)
        sc.sc_start(1, sc.SC_NS)
        assert y.read() == (left and right)
    assert sc.sc_time_stamp() == sc.sc_time(4, sc.SC_NS)
    print('MIXED_GATE_OK: all four input pairs match at 4 ns')
    return 0


if __name__ == '__main__':
    main()
