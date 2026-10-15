"""Experimental composition bridge; not a supported package API.

A Python Protocol documents the implementation contract. The native interface
remains the authoritative signature; adapters retain implementations and must
be retained by their caller through simulation.
"""

from amal.eda.systemc import sc_core as sc


def implement(interface, implementation, name):
    """Adapt ordinary Python methods to one native interface.

    Args:
        interface: Native SystemC interface specialization.
        implementation: Python object with the interface's ordinary method names.
        name: Native adapter module name.

    Returns:
        Native interface/module adapter retaining the Python implementation.

    Raises:
        TypeError: A required implementation method is absent or not callable.
        NotImplementedError: The native interface has no prototype mapping.

    Ports borrow the result; retain it through simulation. Callback exceptions
    propagate through existing cppyy dispatch. Blocking callers still need GIL
    configuration; this prototype does not establish reset or reference safety.
    """
    mappings = {
        "tlm::tlm_transport_if": {"transport_value": "transport"},
        "tlm::tlm_blocking_get_if": {"get_value": "get"},
        "tlm::tlm_blocking_peek_if": {"peek_value": "peek"},
        "tlm::tlm_analysis_if": {"write_value": "write"},
    }
    family = interface.__cpp_name__.split("<", 1)[0]
    if family not in mappings:
        raise NotImplementedError(f"prototype has no mapping for {family}")
    callbacks = {}
    for native_name, python_name in mappings[family].items():
        callback = getattr(implementation, python_name, None)
        if not callable(callback):
            raise TypeError(f"{family} requires callable {python_name}")

        def forward(self, *args, _callback=callback):
            """Forward native value callbacks to the retained implementation."""
            return _callback(*args)

        callbacks[native_name] = forward
    base = sc.channel_interface(interface)
    adapter_class = type(f"{type(implementation).__name__}NativeAdapter", (base,), callbacks)
    module_name = sc.sc_module_name(name)
    adapter = adapter_class(module_name)
    adapter.implementation = implementation
    return adapter
