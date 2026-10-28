"""Initialize fixed-point owners directly with exact native source types."""

from pathlib import Path

import cppyy

from ._integers import _integer_text
from ._native_methods import NativeMethod

cppyy.include(str(Path(__file__).with_name("fixed_constructors.h")))


def _source_signature(value):
    """Resolve the declared native source type without copying observed numbers."""
    if type(value) is int:
        return "int"
    if type(value) is float:
        return "double"
    if type(value) is str:
        return "const char*"
    source_type = type(value)
    if not hasattr(source_type, "__cpp_name__"):
        return None
    scalar = str(cppyy.gbl.pysystemc_datatypes.ConstructorScalar[source_type].name())
    if scalar:
        return scalar
    native = cppyy.gbl.sc_dt
    for name in (
        "sc_fxval",
        "sc_fxval_fast",
        "sc_fxnum",
        "sc_fxnum_fast",
        "sc_int_base",
        "sc_uint_base",
        "sc_signed",
        "sc_unsigned",
    ):
        if isinstance(value, getattr(native, name)):
            return f"const sc_dt::{name}&"
    return None


def _adapt(native_type):
    """Keep observed native initialization in place and preserve exact sources."""
    if native_type.__dict__.get("_pysystemc_exact_constructor", False):
        return
    native_type._pysystemc_exact_constructor = True
    native_constructor = native_type.__init__
    fast = "fast" in native_type.__cpp_name__.split("<", 1)[0]
    observer_type = cppyy.gbl.sc_dt.sc_fxnum_fast_observer if fast else cppyy.gbl.sc_dt.sc_fxnum_observer
    observer_signature = "sc_dt::sc_fxnum_fast_observer*" if fast else "sc_dt::sc_fxnum_observer*"
    configurable = native_type.__cpp_name__ in {
        "sc_dt::sc_fix",
        "sc_dt::sc_ufix",
        "sc_dt::sc_fix_fast",
        "sc_dt::sc_ufix_fast",
    }

    def construct(owner, *args, **kwargs):
        """Initialize native fixed-point storage with its borrowed observer.

        Args:
            args: Optional native source and constructor parameters, including
                width/integer-width, quantization/overflow, overflow bit count,
                native type parameters, cast switch and borrowed observer.
                Template owners encode width and policy parameters in their type.
            kwargs: Native keyword arguments, forwarded to native initialization.
        Returns:
            None, matching native construction.
        Raises:
            TypeError: The native constructor or argument conversion is unavailable.
            Native exception: Allocation, conversion or configured reports throw.
        Initialization occurs directly in the owner's native storage, preserving
        observer identity and native callback ordering. Keep the observer owner
        live for the number's lifetime. Calls hold the GIL and retain no extra
        Python references. Arbitrary widths and native precision rules apply.
        """
        if kwargs or not args or (len(args) == 1 and isinstance(args[0], native_type)):
            return native_constructor(owner, *args, **kwargs)
        arguments = list(args)
        if configurable:
            selected = _configured_signature(arguments, observer_type, observer_signature)
            if selected is None:
                return native_constructor(owner, *args, **kwargs)
            signatures, arguments = selected
            return native_constructor.__overload__(", ".join(signatures))(owner, *arguments)
        if type(arguments[0]) is int and not -(1 << 31) <= arguments[0] < (1 << 31):
            arguments[0] = _integer_text(arguments[0])
        source_signature = _source_signature(arguments[0])
        if source_signature is None:
            return native_constructor(owner, *args, **kwargs)
        signatures = [source_signature]
        observer_supplied = False
        for argument in arguments[1:]:
            if isinstance(argument, cppyy.gbl.sc_dt.sc_fxcast_switch):
                signatures.append("const sc_dt::sc_fxcast_switch&")
            elif argument is cppyy.nullptr or isinstance(argument, observer_type):
                signatures.append(observer_signature)
                observer_supplied = True
            else:
                return native_constructor(owner, *args, **kwargs)
        if not observer_supplied:
            signatures.append(observer_signature)
        return native_constructor.__overload__(", ".join(signatures))(owner, *arguments)

    native_type.__init__ = NativeMethod(construct, native_constructor)


def _configured_signature(arguments, observer_type, observer_signature):
    """Distinguish native parameter-only constructors from source constructors.

    Native enum identities distinguish policies from scalar numeric sources.
    Observer pointers remain borrowed and construction stays in place.
    """
    native = cppyy.gbl.sc_dt
    arguments = list(arguments)
    observer = bool(arguments) and (arguments[-1] is cppyy.nullptr or isinstance(arguments[-1], observer_type))
    values = arguments[:-1] if observer else arguments
    tokens = []
    for value in values:
        if type(value) is int:
            tokens.append("int")
        else:
            name = getattr(type(value), "__cpp_name__", "")
            if name in {"sc_dt::sc_q_mode", "sc_dt::sc_o_mode"}:
                tokens.append(name)
            elif isinstance(value, native.sc_fxcast_switch):
                tokens.append("const sc_dt::sc_fxcast_switch&")
            elif isinstance(value, native.sc_fxtype_params):
                tokens.append("const sc_dt::sc_fxtype_params&")
            else:
                tokens.append(None)
    q, o = "sc_dt::sc_q_mode", "sc_dt::sc_o_mode"
    cast = "const sc_dt::sc_fxcast_switch&"
    params = "const sc_dt::sc_fxtype_params&"
    shapes = {(), ("int", "int"), (q, o), (q, o, "int"), ("int", "int", q, o), ("int", "int", q, o, "int"), (params,)}
    shapes |= {shape + (cast,) for shape in tuple(shapes)}
    if tuple(tokens) in shapes:
        signatures = tokens
    elif values and tuple(tokens[1:]) in shapes:
        if type(values[0]) is int and not -(1 << 31) <= values[0] < (1 << 31):
            arguments[0] = _integer_text(values[0])
        source = _source_signature(arguments[0])
        if source is None:
            return None
        signatures = [source, *tokens[1:]]
    else:
        return None
    return [*signatures, observer_signature], arguments


def _pythonize(native_type, name):
    """Adapt all native fixed-point template widths and policy parameters."""
    if name in {"sc_fix", "sc_ufix", "sc_fix_fast", "sc_ufix_fast"} or name.startswith(
        ("sc_fixed<", "sc_ufixed<", "sc_fixed_fast<", "sc_ufixed_fast<")
    ):
        _adapt(native_type)


cppyy.py.add_pythonization(_pythonize, "sc_dt")
for _name in ("sc_fix", "sc_ufix", "sc_fix_fast", "sc_ufix_fast"):
    _adapt(getattr(cppyy.gbl.sc_dt, _name))
