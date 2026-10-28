"""Preserve native scalar storage and type when registering trace-file values."""

import cppyy

_PREFIX = "pysystemc_tracing::trace_value<"


def value_type(value):
    """Return a stable holder's unrestricted native type, or None for other values."""
    name = getattr(type(value), "__cpp_name__", "")
    return name[len(_PREFIX) : -1] if name.startswith(_PREFIX) else None


def register(file, holder, name, *arguments, native_free=False):
    """Register stable native storage and retain its owner through trace closure.

    Args:
        file: Live native trace file, or None for namespace sc_trace's null case.
        holder: Native sc_trace_value[Type] with owned, mutable member storage.
        name: Native waveform name.
        *arguments: Optional native integer width, or enumeration string labels.
        native_free: Select free sc_trace overloads, including default widths.

    Returns:
        None after registration, matching native tracing.

    Raises:
        TypeError: Value type or arguments lack a native tracing overload.
        sc_report: Native trace registration rejects the input.

    Calls hold the GIL and do not advance simulation. Update holder.value to
    change the recorded native value. The file retains the holder until closure;
    manual native destruction and native-created files require caller lifetime
    management. Native conversion and width rules remain in force.
    """
    if not file:
        return None
    owners = [holder]
    if len(arguments) == 1 and isinstance(arguments[0], (list, tuple)):
        names = list(arguments[0])
        if names and names[-1] is None:
            names.pop()
        if not all(isinstance(label, str) for label in names):
            raise TypeError("enumeration labels must be strings with an optional final None")
        labels = cppyy.gbl.pysystemc_tracing.enum_labels(names)
        dispatch = cppyy.gbl.pysystemc_tracing.enum_dispatch
        call = dispatch.free_trace if native_free else dispatch.trace
        result = call(file, holder, name, labels)
        owners.append(labels)
    else:
        dispatch = (
            cppyy.gbl.pysystemc_tracing.free_value_dispatch
            if native_free
            else cppyy.gbl.pysystemc_tracing.value_dispatch
        )
        result = dispatch[value_type(holder)].trace(file, holder, name, *arguments)
    file.__dict__.setdefault("_trace_value_owners", []).extend(owners)
    return result


def _pythonize(file_type, name):
    """Adapt the abstract trace-file class and reflected native-derived classes."""
    if not any(getattr(base, "__cpp_name__", "") == "sc_core::sc_trace_file" for base in file_type.__mro__):
        return
    native = file_type.trace

    def trace(self, value, name, *arguments):
        """Register an owned native value holder or call the native trace overload.

        Args:
            value: sc_trace_value[Type] holder or native reference argument.
            name: Native trace name.
            *arguments: Optional native width or enumeration literals.

        Holder storage is retained until file closure. Native reference arguments
        remain caller-owned; primitive Python temporaries cannot represent a
        persistent waveform. Calls hold the GIL and native errors propagate.
        Explicit native overload selection remains available with __overload__.
        """
        if value_type(value) is not None:
            return register(self, value, name, *arguments)
        return native(self, value, name, *arguments)

    trace.__overload__ = native.__overload__
    file_type.trace = trace


def install():
    """Install native trace-file dispatch before reflecting derived file types."""
    cppyy.py.add_pythonization(_pythonize, "sc_core")
