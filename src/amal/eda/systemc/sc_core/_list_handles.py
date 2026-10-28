"""Convert Python null handles for native pointer-list insertion."""

import cppyy


def _wrap(native, name=""):
    """Wrap base and arbitrary typed lists when cppyy reflects their classes."""
    spelling = getattr(native, "__cpp_name__", "")
    if spelling != "sc_core::sc_plist_base" and not spelling.startswith("sc_core::sc_plist<"):
        return
    if native.__dict__.get("_null_list_handles_wrapped", False):
        return
    for method in ("insert_before", "insert_after"):
        setattr(native, method, _insertion(getattr(native, method)))
    native._null_list_handles_wrapped = True


def _insertion(operation):
    """Retain the exact native insertion overload for null-handle adaptation."""

    def insert(values, handle, value):
        """Insert a borrowed pointer using a native node handle or Python None.

        Args:
            values: Owning native list.
            handle: Borrowed live node handle, or None for the native null branch.
            value: Borrowed correctly typed pointer to store.

        Returns:
            Borrowed native handle to the newly owned list node.

        Raises:
            TypeError: Handle or value conversion is incompatible.

        GIL stays held; lists own nodes, not pointees. Node handles invalidate on
        removal. Pass only an interior handle or null to native insert_after;
        its tail-handle path dereferences the null successor.
        """
        return operation(values, cppyy.nullptr if handle is None else handle, value)

    return insert


def install():
    """Enable null-handle insertion for existing and future native lists."""
    _wrap(cppyy.gbl.sc_core.sc_plist_base)
    for candidate in tuple(vars(cppyy.gbl.sc_core).values()):
        if isinstance(candidate, type):
            _wrap(candidate)
    cppyy.py.add_pythonization(_wrap)
