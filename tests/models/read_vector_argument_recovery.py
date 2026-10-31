"""Exercise public callback and vector argument errors with native consumers."""

from amal.eda.systemc import sc_core as sc
import cppyy


def expect_type_error(operation, expected_text):
    """Require a public API call to reject an argument with the expected error."""
    try:
        operation()
    except TypeError as error:
        assert expected_text in str(error), (expected_text, str(error))
    else:
        raise AssertionError(f"expected TypeError containing {expected_text!r}")


cppyy.cppdef(
    """
    namespace read_vector_argument_recovery {
    /** @brief Native interface used to exercise unsupported structural binding.
     * @details Ports borrow implementations for their lifetime. Implementations
     * must outlive bound ports. This interface dispatches only native C++ calls;
     * it does not retain Python callbacks or acquire or release the GIL.
     */
    struct UnsupportedReadInterface : virtual sc_core::sc_interface {
      virtual int sample() const = 0;
    };

    /** @brief Concrete native implementation used after structural rejection.
     * @details The Python test retains this object until the port is destroyed.
     * No Python callback, exception translation, or GIL transition is involved.
    */
    struct NativeReadImplementation final : UnsupportedReadInterface {
      /** @brief Return the fixed sample consumed by the native port.
       * @return The integer value 91.
       * @throws No exceptions are raised.
       * @note Runs as native C++ with no Python callback or GIL transition.
       */
      int sample() const override { return 91; }
    };

    /** @brief Consume a signal read interface through native virtual dispatch.
     * @param source Borrowed interface owned by a live native channel.
     * @return The current signal value.
     * @throws sc_core::sc_report Native SystemC reports propagate unchanged.
     * @note No callback is made and no GIL transition is needed.
     */
    int consume_signal(sc_core::sc_signal_in_if<int>& source) {
      return source.read();
    }

    /** @brief Sum all values in a native vector of integer signals.
     * @param signals Borrowed vector whose elements remain alive for this call.
     * @return Sum of the current native signal values.
     * @throws sc_core::sc_report Native SystemC reports propagate unchanged.
     * @note This synchronous C++ call invokes native read methods without Python
     * callbacks or GIL transitions.
     */
    int consume_signal_vector(sc_core::sc_vector<sc_core::sc_signal<int>>& signals) {
      int total = 0;
      for (std::size_t index = 0; index < signals.size(); ++index) {
        total += signals[index].read();
      }
      return total;
    }

    /** @brief Consume a custom interface through a native SystemC port.
     * @param port Borrowed port whose interface remains bound for this call.
     * @return The value returned by the C++ implementation.
     * @throws sc_core::sc_report Native SystemC reports propagate unchanged.
     * @note No Python callback or GIL transition is involved.
     */
    int consume_custom(sc_core::sc_port<UnsupportedReadInterface>& port) {
      return port->sample();
    }
    }  // namespace read_vector_argument_recovery
    """
)

module_name = sc.sc_module_name("read_vector_argument_recovery")
module = sc.sc_channel(module_name)

expect_type_error(
    lambda: sc.read_interface(int, "invalid_read_callback", object()),
    "read interface callback must be callable",
)
read_calls = []


def read_value():
    """Return the recovered callback value and record its native sample."""
    read_calls.append(37)
    return 37


read_channel = sc.read_interface(int, "recovered_read_callback", read_value)
read_port = sc.sc_in[int]("recovered_read_port")
read_port.bind(read_channel)

signal_vector_type = sc.sc_vector[sc.sc_signal[int]]
signal_vector = signal_vector_type("recovered_signal_vector")
expect_type_error(
    lambda: signal_vector.init(3, object()),
    "vector creator must be callable",
)
creator_calls = []


def create_signal(name, index):
    """Return a native signal initialized from its requested vector index."""
    creator_calls.append((str(name), int(index)))
    return sc.sc_signal[int](name, (index + 1) * 11)


signal_vector.init(3, create_signal)

unsupported_interface = cppyy.gbl.read_vector_argument_recovery.UnsupportedReadInterface
custom_port = sc.sc_port[unsupported_interface]("recovered_custom_port")


class PythonCustomImplementation:
    """Offer a structural method that the custom native interface cannot adapt."""

    def sample(self):
        """Return an unused value for the unsupported binding attempt."""
        return -1


expect_type_error(
    lambda: custom_port.bind(PythonCustomImplementation()),
    "automatic Python binding is not supported",
)
native_implementation = cppyy.gbl.read_vector_argument_recovery.NativeReadImplementation()
custom_port.bind(native_implementation)

del module_name
read_channel.notify_changed()
sc.sc_start(sc.SC_ZERO_TIME)

read_value_result = int(cppyy.gbl.read_vector_argument_recovery.consume_signal(read_port.get_interface()))
vector_value_result = int(cppyy.gbl.read_vector_argument_recovery.consume_signal_vector(signal_vector))
custom_value_result = int(cppyy.gbl.read_vector_argument_recovery.consume_custom(custom_port))

assert read_calls == [37], read_calls
assert read_value_result == 37, read_value_result
assert tuple(index for _name, index in creator_calls) == (0, 1, 2), creator_calls
assert signal_vector.size() == 3, signal_vector.size()
assert tuple(int(signal_vector[index].read()) for index in range(3)) == (11, 22, 33)
assert vector_value_result == 66, vector_value_result
assert custom_value_result == 91, custom_value_result
sc.sc_stop()
print("READ_VECTOR_ARGUMENT_RECOVERY_OK read=37/1 vector=66/3 custom=91")
