"""Adapt structural Python implementations to native SystemC port interfaces.

Native signatures remain authoritative. Adapters and implementations are retained
until interpreter shutdown because SystemC ports borrow their interfaces.
"""

import cppyy

_owners = []
_classes = {}
_protocol_interfaces = {}


def interface(*, methods):
    """Decorate a Protocol with explicit native SystemC method signatures.

    Args:
        methods: Mapping of method names to C++ (return type, arguments).

    Returns:
        Decorator retaining the Protocol for typing and registering its native type.

    Signatures are trusted C++ declarations, including reference qualifications.
    Python annotations do not determine native widths or ownership. Native
    compilation errors propagate; implementations are checked when bound.
    """
    def register(protocol):
        """Associate the structural contract with a lazily compiled native interface."""
        from ._channel_interfaces import declare_interface

        native = declare_interface(f"ProtocolInterface{len(_protocol_interfaces)}", methods)
        _protocol_interfaces[protocol] = native
        _METHODS[native.__cpp_name__] = {name: name for name in methods}
        return protocol

    return register

# Native callback spellings stay private to the composition bridge.
_METHODS = {
    "tlm::tlm_transport_if": {"transport_value": "transport"},
    "tlm::tlm_blocking_get_if": {"get_value": "get"},
    "tlm::tlm_blocking_peek_if": {"peek_value": "peek"},
    "tlm::tlm_analysis_if": {"write_value": "write"},
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
                return getattr(self._implementation, _method)(*args)
            callbacks[native_name] = forward
        _classes[interface] = type(f"PythonPortAdapter{len(_classes)}", (factory(interface),), callbacks)
    name = cppyy.gbl.sc_core.sc_module_name(cppyy.gbl.sc_core.sc_gen_unique_name("python_interface", False))
    adapter = _classes[interface](name)
    adapter._implementation = implementation
    for method in ("transport", "get", "peek"):
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
