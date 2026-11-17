"""Verify interface inference and declaration errors recover in one process."""

from typing import Protocol

from amal.eda.systemc import sc_core as sc
import cppyy


def expect_error(operation, error_type, expected_text):
    """Require a public interface operation to report its expected argument error."""
    try:
        operation()
    except error_type as error:
        assert expected_text in str(error), (expected_text, str(error))
    else:
        raise AssertionError(f"expected {error_type.__name__} containing {expected_text!r}")


class MissingSelf(Protocol):
    """Describe a method with no instance receiver."""

    def read_value() -> bool:
        """Return a protocol value without a self parameter."""
        ...


class VariableArguments(Protocol):
    """Describe a method with a variadic argument list."""

    def read_value(self, *values: bool) -> bool:
        """Declare a method that cannot map to a fixed native signature."""
        ...


class DefaultArgument(Protocol):
    """Describe a method with a Python default argument."""

    def read_value(self, value: bool = False) -> bool:
        """Declare an argument whose default needs an explicit native signature."""
        ...


class MissingReturn(Protocol):
    """Describe a method without a return annotation."""

    def read_value(self):
        """Declare a return type that cannot be inferred."""
        ...


expect_error(lambda: sc.interface(MissingSelf), TypeError, "instance method with self")
expect_error(lambda: sc.interface(VariableArguments), TypeError, "fixed positional parameters")
expect_error(lambda: sc.interface(DefaultArgument), TypeError, "defaults require explicit native methods")
expect_error(lambda: sc.interface(MissingReturn), TypeError, "requires a return annotation")

expect_error(
    lambda: sc.declare_interface("invalid-interface-name", {"read_value": ("int", "")}),
    ValueError,
    "C++ identifier",
)
expect_error(
    lambda: sc.declare_interface("RecoveryBadMethod", {"read-value": ("int", "")}),
    ValueError,
    "method requires an identifier",
)
expect_error(
    lambda: sc.declare_interface("RecoveryBadShape", {"read_value": ("int",)}),
    ValueError,
    "method requires an identifier",
)
expect_error(
    lambda: sc.declare_interface("RecoveryTemplateReference", {"read_value": ("int&", "std::vector<int> value")}),
    ValueError,
    "templates or defaults require a native adapter",
)
expect_error(
    lambda: sc.declare_interface("RecoveryDefaultReference", {"read_value": ("int&", "int value = 0")}),
    ValueError,
    "templates or defaults require a native adapter",
)
expect_error(
    lambda: sc.declare_interface("RecoveryUnnamedReference", {"read_value": ("int&", "int (&)[4]")}),
    ValueError,
    "require named arguments without defaults",
)


@sc.interface
class RecoveredProtocol(Protocol):
    """Declare an inferred protocol successfully after invalid inference cases."""

    def is_ready(self, enabled: bool) -> bool:
        """Return the readiness state for a native bool argument."""
        ...


class StructuralImplementation:
    """Provide the recovered inferred protocol through structural binding."""

    def __init__(self):
        """Initialize an invocation count for native dispatch assertions."""
        self.calls = 0

    def is_ready(self, enabled):
        """Invert the native bool and record the Python callback invocation."""
        self.calls += 1
        return not enabled


recovered_interface = sc.declare_interface("ArgumentRecoveryInterface", {"read_value": ("int", "")})

cppyy.cppdef(
    """
    /** @brief Call the recovered interface through native virtual dispatch.
     *
     * @details The interface reference is borrowed for this call and its Python
     * owner must remain alive until return. cppyy reacquires the GIL when the
     * Python override runs. Callback exceptions propagate to the caller.
     */
    int consume_argument_recovery_interface(
        pysystemc_interfaces::ArgumentRecoveryInterface& interface) {
        return interface.read_value();
    }
    """
)


class Channel(recovered_interface):
    """Implement the recovered native method in retained Python storage."""

    def __init__(self):
        """Initialize the callback result and invocation record."""
        super().__init__()
        self.calls = 0

    def read_value(self):
        """Return the native value and record the Python callback invocation."""
        self.calls += 1
        return 73


module_name = sc.sc_module_name("interface_argument_recovery")
module = sc.sc_channel(module_name)
channel = Channel()
declared_port = sc.sc_port[recovered_interface]("declared_recovery_port")
declared_port.bind(channel)
implementation = StructuralImplementation()
inferred_port = sc.sc_port[RecoveredProtocol]("inferred_recovery_port")
inferred_port.bind(implementation)
del module_name
sc.sc_start(sc.SC_ZERO_TIME)

result = cppyy.gbl.consume_argument_recovery_interface(declared_port.get_interface())
assert result == 73, result
assert channel.calls == 1, channel.calls
inferred_result = inferred_port.get_interface().is_ready(False)
assert inferred_result is True, inferred_result
assert implementation.calls == 1, implementation.calls
sc.sc_stop()
print(
    "INTERFACE_ARGUMENT_RECOVERY_OK "
    f"declared={result}/{channel.calls} inferred={inferred_result}/{implementation.calls}"
)
