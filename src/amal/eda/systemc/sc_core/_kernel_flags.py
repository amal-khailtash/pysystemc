"""Keep mutable native kernel flags shared through Python module assignment."""

from types import ModuleType

import cppyy

_FLAGS = frozenset(("sc_enable_name_checking", "sc_allow_process_control_corners"))


class _KernelModule(ModuleType):
    """Read and assign supported boolean flags through native kernel storage."""

    def __getattribute__(self, name):
        """Read native flags live while preserving ordinary module lookup.

        Args:
            name: Module member name.

        Returns:
            Current native bool for a kernel flag; otherwise the ordinary module
            member.

        Native flag reads hold the GIL and must occur on the simulation thread.
        They do not advance the kernel. ``from module import flag`` evaluates
        this once and retains an independent Python bool snapshot.
        """
        if name in _FLAGS:
            return getattr(cppyy.gbl.sc_core, name)
        return super().__getattribute__(name)

    def __setattr__(self, name, value):
        """Assign native flags synchronously, retaining ordinary module attributes.

        Args:
            name: Module member name.
            value: Boolean for a native flag, or an ordinary Python member value.

        Raises:
            TypeError: A native flag receives a nonboolean value.

        Flag access holds the GIL and must occur on the simulation thread. Imported
        scalar snapshots remain copies; access the module member for live values.
        """
        if name in _FLAGS:
            if not isinstance(value, bool):
                raise TypeError(f"{name} requires a bool")
            setattr(cppyy.gbl.sc_core, name, value)
        else:
            super().__setattr__(name, value)


def install(module):
    """Enable native boolean assignments on the existing kernel module.

    Args:
        module: Loaded sc_core Python module.

    Returns:
        None after assigning the module type; no simulation or callbacks run.
    """
    module.__class__ = _KernelModule
