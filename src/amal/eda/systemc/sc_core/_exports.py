"""Preserve native export reports without cppyy const-overload fallback."""

import cppyy


class ExportTemplate:
    """Instantiate exports for unrestricted native interface types."""

    def __init__(self, template):
        """Retain the native template without precompiling specializations."""
        self.template = template

    def __getitem__(self, interface):
        """Return a native export class with unambiguous arrow access.

        Args:
            interface: Native interface class or native type-name string.

        Returns:
            Native specialization retaining ordinary construction and binding.

        Raises:
            TypeError: If the template parameter cannot instantiate a native export.
        """
        if isinstance(interface, str):
            interface = getattr(cppyy.gbl, interface)
        native = self.template[interface]
        if not getattr(native, "_selected_export_arrow", False):
            native_interface = native.__cpp_name__.split("<", 1)[1].removesuffix(">")
            bridge = cppyy.gbl.sc_core.python_export_access[native_interface].arrow

            def arrow(self):
                """Borrow the bound interface or propagate the native unbound report."""
                return bridge(self)

            setattr(native, "operator->", arrow)
            native._selected_export_arrow = True
        return native

    def __call__(self, interface):
        """Support cppyy's alternate template specialization spelling."""
        return self[interface]
