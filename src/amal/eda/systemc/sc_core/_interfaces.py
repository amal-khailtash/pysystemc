"""Adapt structural Python implementations to native SystemC port interfaces.

Native signatures remain authoritative. Adapters and implementations are retained
until interpreter shutdown because SystemC ports borrow their interfaces.
"""

import inspect
from typing import get_type_hints

import cppyy

from ._channel_interfaces import declare_interface, _reference_callbacks, _SCALAR_TYPES

_owners = []
_classes = {}
_protocol_interfaces = {}


def teardown_interfaces():
    """Delete retained structural native adapters after all borrowers are destroyed.

    Returns:
        Number of native adapters destroyed; subsequent calls return zero.

    Raises:
        RuntimeError: Unless the kernel is stopped and every native port/export
            has been destroyed. Rejection occurs before callback release.

    Call with the GIL held after end hooks, trace closure and all final reads.
    Destroy ports/exports first, including native-created borrowers. This
    conservative gate rejects unrelated live ports too. Borrowed adapter,
    event and result proxies become invalid and must never be used afterward.
    Process callbacks and directly constructed channels are outside this API.
    The stopped kernel is not reset or restarted.
    """
    try:
        cppyy.gbl.pysystemc_interfaces.require_interface_teardown()
    except cppyy.gbl.std.runtime_error as error:
        raise RuntimeError(str(error)) from error
    release_interface_callbacks()
    count = len(_owners)
    while _owners:
        _, _, adapter = _owners[-1]
        adapter.__destruct__()
        _owners.pop()
    return count


def release_interface_callbacks():
    """Release structural callback owners after the kernel has stopped.

    Returns:
        Number of adapters whose Python owners were released.

    Raises:
        RuntimeError: If simulation has not completed a native stop.

    Native adapters remain retained because ports borrow their addresses. Call
    this after all end-of-simulation hooks and final interface/trace reads have
    completed. Subsequent structural callbacks raise RuntimeError. This does
    not delete native channels, reset the kernel or release process callbacks.
    Call from Python while holding the GIL, outside a running simulation.
    """
    if cppyy.gbl.sc_core.sc_get_status() != cppyy.gbl.sc_core.SC_STOPPED:
        raise RuntimeError("interface callbacks can only be released after simulation stop")
    released = 0
    for index, (interface, owner, adapter) in enumerate(_owners):
        if owner is not None:
            adapter._implementation = None
            adapter._reference_results.clear()
            _owners[index] = (interface, None, adapter)
            released += 1
    return released


def _fifo_integer(value, native_type, method):
    """Validate fundamental callbacks before conversion to native FIFO storage.

    Args:
        value: Python callback result.
        native_type: Exact fundamental C++ type spelling.
        method: Callback name used in error diagnostics.

    Returns:
        The unchanged value for valid native-width integers or other datatypes.

    Raises:
        TypeError: For invalid bool/numeric types or out-of-range integral results.
    """
    if native_type == "bool" and not isinstance(value, bool):
        raise TypeError(f"{method} must return bool for a bool FIFO")
    if native_type in ("float", "double", "long double") and (
        isinstance(value, bool) or not isinstance(value, (int, float))
    ):
        raise TypeError(f"{method} must return a numeric value for a {native_type} FIFO")
    if native_type in _SCALAR_TYPES and native_type not in ("bool", "float", "double", "long double"):
        bits = cppyy.sizeof(native_type) * 8
        unsigned = native_type.startswith("unsigned") or (
            native_type == "char" and not cppyy.gbl.std.numeric_limits["char"].is_signed
        )
        lower, upper = (0, 2**bits) if unsigned else (-(2 ** (bits - 1)), 2 ** (bits - 1))
        if isinstance(value, bool) or not isinstance(value, int) or not lower <= value < upper:
            raise TypeError(f"{method} must return a native {native_type} integer in range")
    return value


def _annotation_type(annotation, *, result=False):
    """Translate unambiguous annotations, rejecting implicit integer widths/references."""
    if annotation in (None, type(None)) and result:
        return "void"
    if annotation is bool:
        return "bool"
    if annotation is float:
        return "double"
    native = getattr(annotation, "__cpp_name__", None)
    if native:
        return native
    raise TypeError(f"unsupported native interface annotation {annotation!r}; use a native type or explicit methods")


def _infer_methods(protocol):
    """Collect inherited annotated instance methods with fixed positional signatures."""
    methods = {}
    for base in reversed(protocol.__mro__):
        for name, function in vars(base).items():
            if name.startswith("_") or not inspect.isfunction(function):
                continue
            signature = inspect.signature(function)
            hints = get_type_hints(function)
            parameters = list(signature.parameters.values())
            if not parameters or parameters[0].name != "self":
                raise TypeError(f"{name} must be an instance method with self")
            arguments = []
            for parameter in parameters[1:]:
                if parameter.kind not in (parameter.POSITIONAL_ONLY, parameter.POSITIONAL_OR_KEYWORD):
                    raise TypeError(f"{name} requires fixed positional parameters")
                if parameter.default is not parameter.empty:
                    raise TypeError(f"{name} defaults require explicit native methods")
                arguments.append(f"{_annotation_type(hints.get(parameter.name))} {parameter.name}")
            if "return" not in hints:
                raise TypeError(f"{name} requires a return annotation")
            methods[name] = (_annotation_type(hints["return"], result=True), ", ".join(arguments))
    return methods


def interface(protocol=None, *, methods=None):
    """Decorate a Protocol with inferred or explicit native SystemC signatures.

    Args:
        protocol: Annotated interface class when used as @interface.
        methods: Optional mapping to C++ (return type, arguments), overriding inference.

    Returns:
        Decorator retaining the Protocol for typing and registering its native type.

    None returns map to void, bool to bool, float to double, and cppyy native
    classes retain their C++ type. Python int, missing annotations, defaults and
    variadic parameters raise TypeError. Lvalue reference results require explicit trusted C++ signatures and native
    object storage; returned objects are retained through shutdown and keep native
    identity. Reference arguments must have names; defaults and template-valued
    argument declarations require a native adapter. Repeated-name overload
    declarations are not supported by the methods dictionary. Owners survive simulation; errors propagate.
    """

    def register(protocol):
        """Associate the structural contract with a lazily compiled native interface."""
        declarations = methods if methods is not None else _infer_methods(protocol)
        native = declare_interface(f"ProtocolInterface{len(_protocol_interfaces)}", declarations)
        _protocol_interfaces[protocol] = native
        _METHODS[native.__cpp_name__] = {
            _reference_callbacks[native.__cpp_name__].get(name, name): name for name in declarations
        }
        return protocol

    return register(protocol) if protocol is not None else register


# Native callback spellings stay private to the composition bridge.
_METHODS = {
    "tlm::tlm_fifo_config_size_if": {
        "expand_value": "nb_expand",
        "unbound_value": "nb_unbound",
        "reduce_value": "nb_reduce",
        "bound_value": "nb_bound",
    },
    "tlm::tlm_fifo_debug_if": {
        "used": "used",
        "size": "size",
        "debug": "debug",
        "debug_peek": "nb_peek",
        "debug_poke": "nb_poke",
    },
    "tlm::tlm_fw_transport_if": {
        "nb_transport_fw": "nb_transport_fw",
        "b_transport": "b_transport",
        "debug_transport": "transport_dbg",
        "get_direct_mem_ptr": "get_direct_mem_ptr",
    },
    "tlm::tlm_bw_transport_if": {
        "nb_transport_bw": "nb_transport_bw",
        "invalidate_direct_mem_ptr": "invalidate_direct_mem_ptr",
    },
    "tlm::tlm_fw_nonblocking_transport_if": {"nb_transport_fw": "nb_transport_fw"},
    "tlm::tlm_bw_nonblocking_transport_if": {"nb_transport_bw": "nb_transport_bw"},
    "tlm::tlm_fw_direct_mem_if": {"get_direct_mem_ptr": "get_direct_mem_ptr"},
    "tlm::tlm_bw_direct_mem_if": {"invalidate_direct_mem_ptr": "invalidate_direct_mem_ptr"},
    "tlm::tlm_transport_dbg_if": {"debug_transport": "transport_dbg"},
    "tlm::tlm_blocking_transport_if": {"b_transport": "b_transport"},
    "tlm::tlm_blocking_get_peek_if": {"get_value": "get", "peek_value": "peek"},
    "tlm::tlm_get_peek_if": {
        "get_value": "get",
        "peek_value": "peek",
        "try_get_value": "nb_get",
        "try_peek_value": "nb_peek",
        "can_get": "nb_can_get",
        "can_peek": "nb_can_peek",
    },
    "tlm::tlm_peek_if": {"peek_value": "peek", "try_peek_value": "nb_peek", "can_peek": "nb_can_peek"},
    "tlm::tlm_put_if": {"put_value": "put", "try_put_value": "nb_put", "can_put": "nb_can_put"},
    "tlm::tlm_get_if": {"get_value": "get", "try_get_value": "nb_get", "can_get": "nb_can_get"},
    "tlm::tlm_blocking_put_if": {"put_value": "put"},
    "tlm::tlm_transport_if": {"transport_value": "transport"},
    "tlm::tlm_blocking_get_if": {"get_value": "get"},
    "tlm::tlm_blocking_peek_if": {"peek_value": "peek"},
    "tlm::tlm_analysis_if": {"write_value": "write"},
    "tlm::tlm_nonblocking_put_if": {"put_value": "nb_put", "can_put": "nb_can_put"},
    "tlm::tlm_nonblocking_get_if": {"get_value": "nb_get", "can_get": "nb_can_get"},
    "tlm::tlm_nonblocking_peek_if": {"peek_value": "nb_peek", "can_peek": "nb_can_peek"},
    "tlm::tlm_nonblocking_get_peek_if": {
        "get_value": "nb_get",
        "can_get": "nb_can_get",
        "peek_value": "nb_peek",
        "can_peek": "nb_can_peek",
    },
}

for _direction in ("master", "slave"):
    for _mode in ("", "blocking_", "nonblocking_"):
        _callbacks = {}
        if _mode != "nonblocking_":
            _callbacks.update(put_value="put", get_value="get", peek_value="peek")
        if _mode != "blocking_":
            _callbacks.update(
                try_put_value="nb_put",
                try_get_value="nb_get",
                try_peek_value="nb_peek",
                can_put="nb_can_put",
                can_get="nb_can_get",
                can_peek="nb_can_peek",
            )
        _METHODS[f"tlm::tlm_{_mode}{_direction}_if"] = _callbacks

_METHODS["tlm::tlm_fifo_put_if"] = {
    **_METHODS["tlm::tlm_fifo_debug_if"],
    **_METHODS["tlm::tlm_put_if"],
}
_METHODS["tlm::tlm_fifo_get_if"] = {
    **_METHODS["tlm::tlm_fifo_debug_if"],
    **_METHODS["tlm::tlm_get_peek_if"],
}


for _family in ("sc_core::sc_signal_in_if", "sc_core::sc_signal_inout_if"):
    _METHODS[_family] = {
        "python_reference_read": "read",
        "python_reference_get_data_ref": "get_data_ref",
        "python_reference_value_changed_event": "value_changed_event",
        "event": "event",
    }
    if _family.endswith("inout_if"):
        _METHODS[_family]["write_value"] = "write"
        _METHODS[_family]["writer_policy"] = "get_writer_policy"

_METHODS["sc_core::sc_fifo_in_if"] = {
    "read_value": "read",
    "try_read_value": "nb_read",
    "num_available": "num_available",
    "python_reference_data_written_event": "data_written_event",
}
_METHODS["sc_core::sc_fifo_out_if"] = {
    "write_value": "write",
    "try_write_value": "nb_write",
    "num_free": "num_free",
    "python_reference_data_read_event": "data_read_event",
}

for _direction, _event in (("in", "data_written_event"), ("out", "data_read_event")):
    _METHODS[f"sc_core::sc_fifo_blocking_{_direction}_if"] = (
        {"read_value": "read"} if _direction == "in" else {"write_value": "write"}
    )
    _METHODS[f"sc_core::sc_fifo_nonblocking_{_direction}_if"] = {
        **({"try_read_value": "nb_read"} if _direction == "in" else {"try_write_value": "nb_write"}),
        f"python_reference_{_event}": _event,
    }

_METHODS["sc_core::sc_mutex_if"] = {name: name for name in ("lock", "trylock", "unlock")}
_METHODS["sc_core::sc_semaphore_if"] = {name: name for name in ("wait", "trywait", "post", "get_value")}
_METHODS["sc_core::sc_event_queue_if"] = {
    "notify_time": "notify",
    "cancel_all": "cancel_all",
    "event_pointer": "default_event",
}
_METHODS["sc_core::sc_signal_write_if"] = {"write_value": "write", "writer_policy": "get_writer_policy"}


def _adapt(interface, implementation, factory):
    """Validate and retain one native adapter for a Python implementation."""
    family = interface.__cpp_name__.split("<", 1)[0]
    methods = _METHODS.get(family)
    if methods is None:
        raise TypeError(f"automatic Python binding is not supported for {interface.__cpp_name__}")
    for native_interface, owner, adapter in _owners:
        if owner is implementation and (native_interface == interface or isinstance(adapter, interface)):
            return adapter
    candidates = {
        "sc_core::sc_signal_in_if": "sc_signal_inout_if",
        "sc_core::sc_signal_write_if": "sc_signal_inout_if",
        "sc_core::sc_fifo_blocking_in_if": "sc_fifo_in_if",
        "sc_core::sc_fifo_nonblocking_in_if": "sc_fifo_in_if",
        "sc_core::sc_fifo_blocking_out_if": "sc_fifo_out_if",
        "sc_core::sc_fifo_nonblocking_out_if": "sc_fifo_out_if",
    }
    combined = candidates.get(family)
    if combined:
        value = interface.__cpp_name__.split("<", 1)[1].removesuffix(">")
        required = set(_METHODS["sc_core::" + combined].values())
        if combined == "sc_signal_inout_if":
            required.discard("get_writer_policy")
            if value in _SCALAR_TYPES:
                required.discard("get_data_ref")
            if value in ("bool", "sc_dt::sc_logic"):
                required.update(("posedge", "negedge", "posedge_event", "negedge_event"))
        if all(callable(getattr(implementation, method, None)) for method in required):
            return _adapt(getattr(cppyy.gbl.sc_core, combined)[value], implementation, factory)
    tlm_candidates = {
        "tlm::tlm_blocking_put_if": "tlm_put_if",
        "tlm::tlm_nonblocking_put_if": "tlm_put_if",
        "tlm::tlm_blocking_get_if": "tlm_get_peek_if",
        "tlm::tlm_nonblocking_get_if": "tlm_get_peek_if",
        "tlm::tlm_blocking_peek_if": "tlm_get_peek_if",
        "tlm::tlm_nonblocking_peek_if": "tlm_get_peek_if",
        "tlm::tlm_get_if": "tlm_get_peek_if",
        "tlm::tlm_peek_if": "tlm_get_peek_if",
        "tlm::tlm_blocking_get_peek_if": "tlm_get_peek_if",
        "tlm::tlm_nonblocking_get_peek_if": "tlm_get_peek_if",
    }
    combined = tlm_candidates.get(family)
    if combined and all(
        callable(getattr(implementation, method, None)) for method in _METHODS["tlm::" + combined].values()
    ):
        value = interface.__cpp_name__.split("<", 1)[1].removesuffix(">")
        return _adapt(getattr(cppyy.gbl.tlm, combined)[value], implementation, factory)
    transport_candidates = {
        "tlm::tlm_fw_nonblocking_transport_if": "tlm_fw_transport_if",
        "tlm::tlm_blocking_transport_if": "tlm_fw_transport_if",
        "tlm::tlm_fw_direct_mem_if": "tlm_fw_transport_if",
        "tlm::tlm_transport_dbg_if": "tlm_fw_transport_if",
        "tlm::tlm_bw_nonblocking_transport_if": "tlm_bw_transport_if",
        "tlm::tlm_bw_direct_mem_if": "tlm_bw_transport_if",
    }
    combined = transport_candidates.get(family)
    if combined:
        candidate = getattr(cppyy.gbl.tlm, combined)[cppyy.gbl.tlm.tlm_base_protocol_types]
        # Native inheritance validates payload/phase compatibility without
        # guessing a protocol from the partial interface's template spelling.
        if issubclass(candidate, interface) and all(
            callable(getattr(implementation, method, None)) for method in _METHODS["tlm::" + combined].values()
        ):
            return _adapt(candidate, implementation, factory)
    if family in ("sc_core::sc_signal_in_if", "sc_core::sc_signal_inout_if"):
        methods = dict(methods)
        value = interface.__cpp_name__.split("<", 1)[1].removesuffix(">")
        if value in _SCALAR_TYPES:
            methods.pop("python_reference_read")
            methods.pop("python_reference_get_data_ref")
            methods["read_value"] = "read"
        if value in ("bool", "sc_dt::sc_logic"):
            for edge in ("posedge", "negedge"):
                methods[edge] = edge
                methods[f"python_reference_{edge}_event"] = f"{edge}_event"
    for method in methods.values():
        if (
            family == "sc_core::sc_signal_inout_if"
            and method == "get_writer_policy"
            and not hasattr(implementation, method)
        ):
            continue
        if not callable(getattr(implementation, method, None)):
            raise TypeError(f"{family} requires callable {method}")
    if interface not in _classes:
        base = factory(interface, _copy_put=True) if family == "tlm::tlm_blocking_put_if" else factory(interface)
        output_pair = getattr(base, "output_pair", None)
        scalar_pair = getattr(base, "scalar_pair", None)
        fifo_scalar_pair = getattr(base, "fifo_scalar_pair", None)
        blocking_value_pair = getattr(base, "blocking_value_pair", None)
        callbacks = {}
        for native_name, python_name in methods.items():

            def forward(self, *args, _method=python_name):
                """Forward a native callback to the retained ordinary Python method."""
                if self._implementation is None:
                    raise RuntimeError("structural interface callbacks have been released")
                if (
                    family == "sc_core::sc_signal_inout_if"
                    and _method == "get_writer_policy"
                    and not hasattr(self._implementation, _method)
                ):
                    return base.default_writer_policy()
                if (
                    (
                        family in ("sc_core::sc_signal_inout_if", "sc_core::sc_signal_write_if")
                        or family.startswith("sc_core::sc_fifo_")
                        or family.startswith("tlm::")
                    )
                    and _method in ("write", "nb_write", "put", "nb_put", "nb_poke")
                    and hasattr(type(args[0]), "__cpp_name__")
                ):
                    copy = type(args[0])()
                    copy.__assign__(args[0])
                    args = (copy, *args[1:])
                result = getattr(self._implementation, _method)(*args)
                if fifo_scalar_pair is not None and _method == "read":
                    result = _fifo_integer(result, interface.__cpp_name__.split("<", 1)[1].removesuffix(">"), _method)
                    return fifo_scalar_pair(result, True)
                if (
                    (
                        _method == "write"
                        and (
                            family
                            in ("sc_core::sc_signal_inout_if", "sc_core::sc_signal_write_if", "tlm::tlm_analysis_if")
                            or family.startswith("sc_core::sc_fifo_")
                        )
                    )
                    or (_method == "put" and family.startswith("tlm::"))
                ) and result is not None:
                    raise TypeError(f"{_method} must return None")
                if (
                    (family == "sc_core::sc_event_queue_if" and _method in ("notify", "cancel_all"))
                    or (family.startswith("tlm::") and _method in ("b_transport", "invalidate_direct_mem_ptr", "debug"))
                ) and result is not None:
                    raise TypeError(f"{_method} must return None")
                if family in ("tlm::tlm_fifo_debug_if", "tlm::tlm_fifo_put_if", "tlm::tlm_fifo_get_if") and _method in (
                    "used",
                    "size",
                ):
                    if isinstance(result, bool) or not isinstance(result, int) or not -(2**31) <= result < 2**31:
                        raise TypeError(f"{_method} must return a native int in range")
                if family.startswith("sc_core::sc_fifo_") and _method in ("num_available", "num_free"):
                    if isinstance(result, bool) or not isinstance(result, int) or not -(2**31) <= result < 2**31:
                        raise TypeError(f"{_method} must return a native int in range")
                if family == "tlm::tlm_fifo_config_size_if" and _method in ("nb_expand", "nb_unbound"):
                    if result is not None:
                        raise TypeError(f"{_method} must return None")
                if (family == "tlm::tlm_fifo_config_size_if" and _method in ("nb_reduce", "nb_bound")) or (
                    family in ("tlm::tlm_fifo_debug_if", "tlm::tlm_fifo_put_if", "tlm::tlm_fifo_get_if")
                    and _method == "nb_poke"
                ):
                    if not isinstance(result, bool):
                        raise TypeError(f"{_method} must return bool")
                if (
                    family.startswith("tlm::")
                    and _method in ("nb_put", "nb_can_put", "nb_can_get", "nb_can_peek", "get_direct_mem_ptr")
                    and not isinstance(result, bool)
                ):
                    raise TypeError(f"{_method} must return bool")
                if family.startswith("tlm::") and _method in ("nb_transport_fw", "nb_transport_bw"):
                    if isinstance(result, bool) or not isinstance(result, cppyy.gbl.tlm.tlm_sync_enum):
                        raise TypeError(f"{_method} must return native tlm_sync_enum")
                    if result not in (
                        cppyy.gbl.tlm.TLM_ACCEPTED,
                        cppyy.gbl.tlm.TLM_UPDATED,
                        cppyy.gbl.tlm.TLM_COMPLETED,
                    ):
                        raise ValueError(f"{_method} returned invalid tlm_sync_enum")
                if family.startswith("tlm::") and _method == "transport_dbg":
                    if isinstance(result, bool) or not isinstance(result, int):
                        raise TypeError("transport_dbg must return int")
                    if not 0 <= result < 2**32:
                        raise ValueError("transport_dbg result outside unsigned int range")
                if family in ("sc_core::sc_mutex_if", "sc_core::sc_semaphore_if"):
                    if isinstance(result, bool) or not isinstance(result, int) or not -(2**31) <= result < 2**31:
                        raise TypeError(f"{_method} must return a native int in range")
                if family in ("sc_core::sc_signal_in_if", "sc_core::sc_signal_inout_if") and _method in (
                    "event",
                    "posedge",
                    "negedge",
                ):
                    if not isinstance(result, bool):
                        raise TypeError(f"{_method} must return bool")
                if (
                    family in ("sc_core::sc_signal_write_if", "sc_core::sc_signal_inout_if")
                    and _method == "get_writer_policy"
                ):
                    if isinstance(result, bool) or result not in (
                        cppyy.gbl.sc_core.SC_ONE_WRITER,
                        cppyy.gbl.sc_core.SC_MANY_WRITERS,
                        cppyy.gbl.sc_core.SC_UNCHECKED_WRITERS,
                    ):
                        raise ValueError("get_writer_policy must return a native writer policy")
                if _reference_callbacks.get(interface.__cpp_name__, {}).get(_method) and (
                    _method.endswith("event") or family == "sc_core::sc_event_queue_if"
                ):
                    if not isinstance(result, cppyy.gbl.sc_core.sc_event) or not cppyy.addressof(result):
                        raise TypeError(f"{_method} must return a live native sc_event")
                if blocking_value_pair is not None and _method in ("get", "peek"):
                    if family == "tlm::tlm_fifo_get_if":
                        result = _fifo_integer(
                            result, interface.__cpp_name__.split("<", 1)[1].removesuffix(">"), _method
                        )
                    return blocking_value_pair(result, True)
                if scalar_pair is not None and _method == "read":
                    scalar = interface.__cpp_name__.split("<", 1)[1].removesuffix(">")
                    if scalar == "bool":
                        if not isinstance(result, bool):
                            raise TypeError("read must return bool for a bool signal")
                    elif scalar in ("float", "double", "long double"):
                        if isinstance(result, bool) or not isinstance(result, (int, float)):
                            raise TypeError(f"read must return a numeric value for a {scalar} signal")
                    else:
                        bits = cppyy.sizeof(scalar) * 8
                        unsigned = scalar.startswith("unsigned") or (
                            scalar == "char" and not cppyy.gbl.std.numeric_limits["char"].is_signed
                        )
                        lower, upper = (0, 2**bits) if unsigned else (-(2 ** (bits - 1)), 2 ** (bits - 1))
                        if isinstance(result, bool) or not isinstance(result, int) or not lower <= result < upper:
                            raise TypeError(f"read must return a native {scalar} integer in range")
                    return scalar_pair(result, True)
                if _reference_callbacks.get(interface.__cpp_name__, {}).get(_method):
                    if not hasattr(type(result), "__cpp_name__"):
                        raise TypeError(f"{_method} must return retained native storage for a reference result")
                    self._reference_results[cppyy.addressof(result)] = result
                if family.startswith("sc_core::sc_fifo_") and _method == "nb_write" and not isinstance(result, bool):
                    raise TypeError("nb_write must return bool")
                if _method in ("nb_get", "nb_peek", "nb_read"):
                    if not isinstance(result, tuple) or len(result) != 2 or not isinstance(result[0], bool):
                        raise TypeError(f"{_method} must return a (bool, value) tuple")
                    if _method == "nb_read":
                        value_type = interface.__cpp_name__.split("<", 1)[1].removesuffix(">")
                        result = (result[0], _fifo_integer(result[1], value_type, _method))
                    if family == "tlm::tlm_fifo_get_if" and result[0]:
                        result = (
                            True,
                            _fifo_integer(
                                result[1], interface.__cpp_name__.split("<", 1)[1].removesuffix(">"), _method
                            ),
                        )
                    if output_pair is not None:
                        if _method in ("nb_get", "nb_peek") and not result[0] and result[1] is None:
                            return output_pair()
                        return output_pair(*result)
                    value_type = interface.__cpp_name__.split("<", 1)[1].removesuffix(">")
                    pair = cppyy.gbl.std.pair[bool, value_type]
                    if _method in ("nb_get", "nb_peek") and not result[0] and result[1] is None:
                        return pair()
                    return pair(*result)
                return result

            callbacks[native_name] = forward
        _classes[interface] = type(f"PythonPortAdapter{len(_classes)}", (base,), callbacks)
    name = cppyy.gbl.sc_core.sc_module_name(cppyy.gbl.sc_core.sc_gen_unique_name("python_interface", False))
    adapter = _classes[interface](name)
    adapter._implementation = implementation
    adapter._reference_results = {}
    for method in ("transport", "get", "peek", "put", "b_transport", "read", "write", "lock", "wait"):
        overload = getattr(type(adapter), method, None)
        if overload is not None and hasattr(overload, "__release_gil__"):
            overload.__release_gil__ = True
    _owners.append((interface, implementation, adapter))
    return adapter


class PortTemplate:
    """Specialize native sc_port while enabling structural Python binding.

    Template arguments and constructed objects retain native SystemC types.
    Python adaptation currently supports TLM blocking get, peek, transport and
    analysis interfaces. Callback errors propagate; blocking callbacks use
    guarded waits. Adapters introduce a child module and live until shutdown.
    """

    def __init__(self, template, factory):
        """Retain the native template and combined-interface factory."""
        self._template = template
        self._factory = factory

    def __getitem__(self, arguments):
        """Return the native specialization with an adapting bind overload."""
        parameters = arguments if isinstance(arguments, tuple) else (arguments,)
        interface = _protocol_interfaces.get(parameters[0], parameters[0])
        native_arguments = (interface, *parameters[1:])
        port_class = self._template[native_arguments if len(native_arguments) > 1 else interface]
        if not getattr(port_class, "_python_interface_bind", False):
            native_bind = port_class.bind
            factory = self._factory

            def bind(port, implementation):
                """Bind native interfaces/ports directly or adapt a Python implementation.

                Raises TypeError for unsupported interfaces or missing callbacks.
                Native binding diagnostics and policies remain SystemC's own.
                """
                if isinstance(implementation, cppyy.gbl.sc_core.sc_export_base):
                    return native_bind(port, implementation.get_interface())
                if isinstance(implementation, cppyy.gbl.sc_core.sc_interface) or isinstance(
                    implementation, cppyy.gbl.sc_core.sc_port_base
                ):
                    return native_bind(port, implementation)
                adapter = _adapt(interface, implementation, factory)
                return native_bind(port, adapter)

            port_class.bind = bind
            port_class.__call__ = bind
            port_class._python_interface_bind = True
        return port_class
