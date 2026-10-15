"""Verify annotated Protocol inference, inheritance and unsupported signatures."""

from __future__ import annotations

from typing import Protocol

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt


class Base(Protocol):
    """Supply an inherited void method."""

    def run(self) -> None:
        """Record one interface invocation."""
        ...


@sc.interface
class Contract(Base, Protocol):
    """Declare unambiguous scalar and arbitrary-width native value signatures."""

    def ready(self, enabled: bool) -> bool:
        """Return inverted readiness."""
        ...

    def scale(self, value: float) -> float:
        """Return twice the supplied double."""
        ...

    def wide(self, value: sc_dt.sc_biguint[73]) -> sc_dt.sc_biguint[73]:
        """Preserve a native73bit value across the adapter."""
        ...


class Implementation:
    """Implement the structural contract without interface inheritance."""

    def run(self):
        """Record callback execution."""
        self.called = True

    def ready(self, enabled):
        """Invert readiness."""
        return not enabled

    def scale(self, value):
        """Double a native double value."""
        return value * 2

    def wide(self, value):
        """Copy native arbitrary-width storage."""
        return sc_dt.sc_biguint[73](value)


class Invalid(Protocol):
    """An integer annotation lacks a native width choice."""

    def read(self) -> int:
        """Declare an unsupported ambiguous result."""
        ...


try:
    sc.interface(Invalid)
except TypeError as error:
    assert "unsupported native interface annotation" in str(error)
else:
    raise AssertionError("ambiguous integer accepted")

module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
port = sc.sc_port[Contract]("port")
implementation = Implementation()
port.bind(implementation)
del module_name
sc.sc_start(sc.SC_ZERO_TIME)
interface = port.get_interface()
interface.run()
assert implementation.called
assert interface.ready(False) and not interface.ready(True)
assert interface.scale(1.25) == 2.5
value = sc_dt.sc_biguint[73]()
value[72] = True
result = interface.wide(value)
assert result[72] and result.to_string() == value.to_string()
sc.sc_stop()
print("INTERFACE_ANNOTATIONS_OK")
