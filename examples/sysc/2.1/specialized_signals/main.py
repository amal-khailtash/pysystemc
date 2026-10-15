"""Exercise specialized integer signal/port concatenation and arithmetic.

Original source: examples/sysc/2.1/specialized_signals/main.cpp.
Original author: Andy Goodrich, Forte Design Systems, 11 April 2005.
Support sources: scx_signal_int.{h,cpp}, scx_signal_uint.{h,cpp},
scx_signal_signed.{h,cpp}, scx_signal_unsigned.{h,cpp} in the same directory.
"""

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy

cppyy.cppdef("""
namespace specialized_example {
/** @brief Select native mixed integer operators without cppyy fallback overloads.
 * @details Arguments are borrowed for this call; results are copied. No callbacks or GIL release.
 */
template<class L, class R> auto multiply(const L& a, const R& b) { return a * b; }
template<class L, class R> auto add(const L& a, const R& b) { return a + b; }
template<class L, class R> auto subtract(const L& a, const R& b) { return a - b; }
template<class A, class B, class C, class D>
auto expression(const A& a, const B& b, const C& c, const D& d) { return c + d * a - b; }
}
""")


class SignalValue:
    """Provide deferred writes and value operations on native signals or ports."""

    def __init__(self, channel, value_type, width):
        """Retain native storage/type and the width used for concatenation."""
        self.channel = channel
        self.value_type = value_type
        self.width = width

    def read(self):
        """Return the current native value after the preceding update phase."""
        return self.channel.read()

    def write(self, value):
        """Convert to native width before scheduling a channel write."""
        self.channel.write(self.value_type(value))

    def bits(self):
        """Return unsigned bit representation independent of signedness."""
        return self.read().to_int64() & ((1 << self.width) - 1)


def concat_read(*values):
    """Read channel/port bits in most-significant to least-significant order."""
    result = 0
    for value in values:
        result = (result << value.width) | value.bits()
    return result


def concat_write(value, *targets):
    """Schedule native writes to each concatenation target, rightmost first."""
    for target in reversed(targets):
        target.write(value & ((1 << target.width) - 1))
        value >>= target.width


class DUT(sc.sc_module):
    """Preserve the upstream six-edge concatenation and mixed arithmetic matrix."""

    def __init__(self, name):
        """Create typed backing signals, ports and eight-bit result channels."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_in[bool]("clock")
        self.inputs = []
        self.outputs = []
        self.results = []
        for stem, template in (
            ("bigint", dt.sc_bigint),
            ("biguint", dt.sc_biguint),
            ("int", dt.sc_int),
            ("uint", dt.sc_uint),
        ):
            input_signal = sc.sc_signal[template[4]](f"back_in_{stem}")
            output_signal = sc.sc_signal[template[4]](f"back_inout_{stem}")
            input_port = sc.sc_in[template[4]](f"in_{stem}")
            output_port = sc.sc_inout[template[4]](f"inout_{stem}")
            input_port.bind(input_signal)
            output_port.bind(output_signal)
            self.inputs.append((SignalValue(input_signal, template[4], 4), SignalValue(input_port, template[4], 4)))
            self.outputs.append((output_signal, SignalValue(output_port, template[4], 4)))
            self.results.append(SignalValue(sc.sc_signal[template[8]](f"signal_{stem}8"), template[8], 8))
        self.sc_thread("exec", self.execute)
        self.sensitive(self.clock.pos())
        self.math_checks = 0

    def display(self, name, value, unsigned=False):
        """Print the same hexadecimal representation as the native example."""
        prefix = "0" if unsigned else ""
        print(f"{name} = {prefix}{value.bits():x}")

    def check_math(self, channels, constants):
        """Compare every upstream mixed signedness expression with native values."""
        values = [channel.read() for channel in channels]
        references = [channel.value_type(value) for channel, value in zip(channels, constants)]
        operators = cppyy.gbl.specialized_example
        for left, right in ((0, 1), (0, 3), (2, 1), (2, 3)):
            for name in ("multiply", "add", "subtract"):
                operation = getattr(operators, name)[type(values[left]), type(values[right])]
                result = operation(values[left], values[right])
                reference = operation(references[left], references[right])
                assert result == reference
                self.math_checks += 1
        expression = operators.expression[*(type(value) for value in values)]
        assert expression(*values) == expression(*references)
        self.math_checks += 1

    def execute(self):
        """Run all upstream assignments, deferred updates and26 math comparisons."""
        ports = []
        for index, ((backing, port), (_, output)) in enumerate(zip(self.inputs, self.outputs)):
            backing.write(2 * index + 1)
            output.write(2 * index + 2)
            ports.extend((port, output))
        sc.wait()
        label = "( m_in_bigint4, m_inout_bigint4, m_in_biguint4, m_inout_biguint4, m_in_int4, m_inout_int4, m_in_uint4, m_inout_uint4 )"
        assert concat_read(*ports) == 0x12345678
        print(f"{label} = 0{concat_read(*ports):x}")
        for target, pair in zip(self.results, ((0, 7), (1, 6), (2, 5), (3, 4))):
            target.write(concat_read(*(ports[index] for index in pair)))
        sc.wait()
        names = ("bigint", "biguint", "int", "uint")
        for index, result in enumerate(self.results):
            self.display(f"m_signal_{names[index]}8", result, index % 2 == 1)
        concat_write(0x123, ports[7], self.results[1])
        sc.wait()
        concat_write(concat_read(ports[7], self.results[1]), self.results[0], ports[1])
        concat_write(0x456, self.results[1], ports[3])
        concat_write(0x978, ports[5], self.results[2])
        concat_write(0xCAB, ports[7], self.results[3])
        sc.wait()
        for index, result in enumerate(self.results):
            self.display(f"m_signal_{names[index]}8", result, index % 2 == 1)
            self.display(f"m_inout_{names[index]}4", ports[2 * index + 1], index % 2 == 1)
        constants = (-1, 255, -2, 128)
        for target, value in zip(self.results, constants):
            target.write(value)
        sc.wait()
        self.check_math(self.results, constants)
        constants = (-1, 15, -2, 8)
        for (backing, _), value in zip(self.inputs, constants):
            backing.write(value)
        sc.wait()
        self.check_math([port for _, port in self.inputs], constants)
        assert self.math_checks == 26
        sc.sc_stop()


clock = sc.sc_clock("clock")
model = DUT("dut")
model.clock.bind(clock)
sc.sc_start()
assert model.math_checks == 26
