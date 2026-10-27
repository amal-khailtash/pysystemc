"""Native reference wrappers retaining their Python-owned referents."""

import cppyy


def sc_ref(value):
    """Return a native reference wrapper retaining the original Python proxy.

    Args:
        value: Native C++ object of any type inferred by SystemC's template.

    Returns:
        Native std::reference_wrapper with the original Python proxy retained.
        Native constness is preserved; mutable inputs produce mutable wrappers.

    Raises:
        TypeError: The value cannot bind to the native template.

    The wrapper keeps the original Python proxy alive until its own destruction.
    References obtained through get() borrow native storage; retain the wrapper
    while using them, and ensure separately managed native storage also remains
    alive. Calls hold the GIL and do not advance simulation.
    """
    wrapper = cppyy.gbl.sc_core.sc_ref(value)
    wrapper._reference_owner = value
    return wrapper


def sc_cref(value):
    """Return a const native reference wrapper retaining its referent.

    Args:
        value: Native C++ object of any type inferred by SystemC's template.

    Returns:
        Native const std::reference_wrapper retaining the original Python proxy.

    Raises:
        TypeError: The value cannot bind to the native template.

    The wrapper owns a Python reference, rather than a copy of the C++ value.
    Keep it alive while using borrowed get() results. Calls hold the GIL and do
    not advance simulation; native consumers preserve the const contract.
    """
    wrapper = cppyy.gbl.sc_core.sc_cref(value)
    wrapper._reference_owner = value
    return wrapper
