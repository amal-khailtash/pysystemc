"""Preserve exact native scalar overloads for the legacy SystemC bit type."""

import cppyy


def _exact_scalar_member(native):
    """Wrap a bit member without changing its native type or ownership."""
    scalar_calls = {kind: native.__overload__(kind) for kind in ("bool", "char", "int")}

    def call(owner, *args):
        """Invoke the native bit overload matching a Python scalar argument.

        Args:
            args: Native member arguments; a bool, int or single-character str
                selects its corresponding native scalar overload.
        Returns:
            The native result, including a borrowed self for assignment, or
            None for construction. Other signatures retain native dispatch.
        Raises:
            TypeError: A native argument cannot convert to the selected type.
            ValueError: A Python integer exceeds the native int range.
            Native exception: A configured native report throws.
        Calls hold the GIL, do not suspend and retain no arguments. Native
        invalid-bit reports remain in effect; configured actions can terminate
        the process. The wrapper does not substitute Python validation.
        """
        if len(args) == 1:
            value = args[0]
            if type(value) is bool:
                return scalar_calls["bool"](owner, value)
            if type(value) is int:
                return scalar_calls["int"](owner, value)
            if type(value) is str and len(value) == 1:
                return scalar_calls["char"](owner, value)
        return native(owner, *args)

    call.__overload__ = native.__overload__
    return call


for _name in ("__init__", "__assign__", "__iand__", "__ior__", "__ixor__"):
    setattr(cppyy.gbl.sc_dt.sc_bit, _name, _exact_scalar_member(getattr(cppyy.gbl.sc_dt.sc_bit, _name)))
