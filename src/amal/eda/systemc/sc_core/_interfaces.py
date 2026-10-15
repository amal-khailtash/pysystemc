"""Adapt structural Python implementations to native SystemC port interfaces.

Native signatures remain authoritative. Adapters and implementations are retained
until interpreter shutdown because SystemC ports borrow their interfaces.
"""

import inspect
from typing import get_type_hints

import cppyy

from ._channel_interfaces import declare_interface

_owners = []
_classes = {}
_protocol_interfaces = {}


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
    variadic parameters raise TypeError. References require explicit trusted C++ signatures; repeated-name overload
    declarations are not supported by the methods dictionary. Owners survive simulation; errors propagate.
    """
    def register(protocol):
        """Associate the structural contract with a lazily compiled native interface."""
        declarations = methods if methods is not None else _infer_methods(protocol)
        native = declare_interface(f"ProtocolInterface{len(_protocol_interfaces)}", declarations)
        _protocol_interfaces[protocol] = native
        _METHODS[native.__cpp_name__] = {name: name for name in declarations}
        return protocol

    return register(protocol) if protocol is not None else register

# Native callback spellings stay private to the composition bridge.
_METHODS = {
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
        "get_value": "nb_get", "can_get": "nb_can_get",
        "peek_value": "nb_peek", "can_peek": "nb_can_peek",
    },
}


def _adapt(interface, implementation, factory):
    """Validate and retain one native adapter for a Python implementation."""
    family = interface.__cpp_name__.split("<", 1)[0]
    methods = _METHODS.get(family)
    if methods is None:
        raise TypeError(f"automatic Python binding is not supported for {interface.__cpp_name__}")
    for method in methods.values():
        if not callable(getattr(implementation, method, None)):
            raise TypeError(f"{family} requires callable {method}")
    for native_interface, owner, adapter in _owners:
        if native_interface == interface and owner is implementation:
            return adapter
    if interface not in _classes:
        callbacks = {}
        for native_name, python_name in methods.items():
            def forward(self, *args, _method=python_name):
                """Forward a native callback to the retained ordinary Python method."""
                result = getattr(self._implementation, _method)(*args)
                if _method in ("nb_get", "nb_peek"):
                    if not isinstance(result, tuple) or len(result) != 2 or not isinstance(result[0], bool):
                        raise TypeError(f"{_method} must return a (bool, value) tuple")
                    value_type = interface.__cpp_name__.split("<", 1)[1].removesuffix(">")
                    return cppyy.gbl.std.pair[bool, value_type](*result)
                return result
            callbacks[native_name] = forward
        base = factory(interface, _copy_put=True) if family == "tlm::tlm_blocking_put_if" else factory(interface)
        _classes[interface] = type(f"PythonPortAdapter{len(_classes)}", (base,), callbacks)
    name = cppyy.gbl.sc_core.sc_module_name(cppyy.gbl.sc_core.sc_gen_unique_name("python_interface", False))
    adapter = _classes[interface](name)
    adapter._implementation = implementation
    for method in ("transport", "get", "peek", "put"):
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
                if isinstance(implementation, cppyy.gbl.sc_core.sc_interface) or isinstance(
                    implementation, cppyy.gbl.sc_core.sc_port_base
                ):
                    return native_bind(port, implementation)
                adapter = _adapt(interface, implementation, factory)
                return native_bind(port, adapter)

            port_class.bind = bind
            port_class._python_interface_bind = True
        return port_class
