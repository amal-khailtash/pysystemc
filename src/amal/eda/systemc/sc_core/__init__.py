"""Native kernel, channels, processes and Python callback adapters.

Callbacks are retained for their native process lifetime. Blocking scheduler
operations release the GIL. Guarded Python waits preserve native reset/kill
unwind in tested cases; direct native waits and arbitrary throw_it require
further compatibility validation.
"""

import atexit
from collections.abc import Callable
from typing import Any
from enum import Enum
import pathlib
import sys

# The parent package selects the JIT C++ standard before importing cppyy.

import cppyy

from .._logging import log
from .._namespace import (
    public_names as _public_names,
    document_member as _document_member,
    namespace_help as _namespace_help,
)
from ._channel_interfaces import channel_interface as channel_interface
from ._channel_interfaces import declare_interface as declare_interface
from ._interfaces import PortTemplate as _PortTemplate
from ._interfaces import interface as interface
from ._interfaces import release_interface_callbacks as release_interface_callbacks
from ._interfaces import teardown_interfaces as teardown_interfaces
from ._tracing import sc_trace as _python_sc_trace
from ._tracing import vcd_trace as _python_vcd_trace
from . import _tracing

# Compile first-party bridges before publishing native aliases or configuring
# their methods. The parent package has already loaded SystemC and selected C++17.
SCRIPT_PATH = pathlib.Path(__file__).parent.absolute()
for _bridge in ("sc_main.cpp", "native_entry.cpp", "sc_module_new.cpp", "interface_teardown.cpp", "hash_helpers.cpp"):
    with (SCRIPT_PATH / _bridge).open() as _source:
        cppyy.cppdef(_source.read())

cppyy.gbl.sc_core.sc_start.__release_gil__ = True
cppyy.gbl.sc_core.sc_initialize.__release_gil__ = True
cppyy.gbl.sc_core.sc_elab_and_sim.__release_gil__ = True
cppyy.gbl.sc_main.__release_gil__ = True


# cppyy's std::function conversion does not retain bound Python callables.
# Keep them alive on their owning module for the native process lifetime.
def _retain_process_callback(register):
    """Wrap registration to retain bound callables on their owning module."""

    def wrapper(self, name, callback, *args):
        """Retain callable ownership after successful native registration."""
        if not callable(callback):
            raise TypeError("process callback must be callable")
        if not hasattr(self, "_process_callbacks"):
            self._process_callbacks = []
        result = register(self, name, callback, *args)
        self._process_callbacks.append(callback)
        return result

    return wrapper


for _name in ("sc_method", "sc_thread", "sc_cthread"):
    setattr(cppyy.gbl.sc_core.ScModule, _name, _retain_process_callback(getattr(cppyy.gbl.sc_core.ScModule, _name)))

# Access the C++ sc_core namespace
sc_core_namespace = cppyy.gbl.sc_core


def add_sc_core_members():
    """Populate resolvable native names while preserving Python print()."""
    module_globals = globals()
    for name in dir(sc_core_namespace):
        if name == "print":
            continue
        try:
            member = getattr(sc_core_namespace, name)
        except AttributeError:
            continue
        module_globals[name] = member


# Add all sc_core members to this module
add_sc_core_members()


# Manually add each public member of the `sc_core` namespace to the module"s globals
for name, member in sc_core_namespace.__dict__.items():
    # Filter out internal or irrelevant attributes
    if not name.startswith("__"):
        globals()[name] = member


# Publish Python helpers after namespace copying so native overloads cannot
# overwrite the adapters. Native C++ sc_module remains intact.
sc_trace = _python_sc_trace
vcd_trace = _python_vcd_trace
sc_create_vcd_trace_file = _tracing.sc_create_vcd_trace_file
sc_create_wif_trace_file = _tracing.sc_create_wif_trace_file
sc_close_vcd_trace_file = _tracing.sc_close_vcd_trace_file
sc_close_wif_trace_file = _tracing.sc_close_wif_trace_file
sc_module = cppyy.gbl.sc_core.ScModule
sc_channel = sc_module
sc_interface = cppyy.gbl.sc_core.PythonInterface
sc_prim_channel = cppyy.gbl.sc_core.PythonPrimChannel
signal_channel = cppyy.gbl.sc_core.PythonSignalChannel
from ._fifo import FifoTemplate as _FifoTemplate

fifo_channel = _FifoTemplate(cppyy.gbl.sc_core.PythonFifoChannel)
sc_fifo = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo)
sc_fifo_in = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo_in)
sc_fifo_out = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo_out)
from ._blocking import guard_native_channels as _guard_native_channels

_guard_native_channels()
# Keep the public port specialization native; only Python bind gains adaptation.
sc_port = _PortTemplate(cppyy.gbl.sc_core.sc_port, channel_interface)
sc_export = _PortTemplate(cppyy.gbl.sc_core.sc_export, channel_interface)
_read_interface_callbacks = []


def default_str_hash_fn(key):
    """Hash a string or borrowed native NUL-terminated key pointer.

    Args:
        key: Python str/bytes, or native pointer; a null pointer hashes to zero.

    Returns:
        Native unsigned string hash.

    Raises:
        TypeError: key cannot convert to the native parameter.

    Pointer storage must survive the call. Call on the simulation thread with
    the GIL held; no ownership transfers or callbacks occur.
    """
    if isinstance(key, (str, bytes)):
        return cppyy.gbl.pysystemc.string_hash(key)
    return cppyy.gbl.sc_core.default_str_hash_fn(key)


def sc_strhash_kdup(key):
    """Allocate a native copy of a nonnull NUL-terminated string key.

    Args:
        key: Python str/bytes or a borrowed valid native string pointer.

    Returns:
        Native void pointer owned by the caller. Free it exactly once using
        sc_strhash_kfree; never use the pointer after freeing it.

    Raises:
        TypeError: key cannot convert to the native parameter.
        Exception: Native allocation fails.

    Call on the simulation thread with the GIL held; no callbacks occur.
    """
    if isinstance(key, (str, bytes)):
        return cppyy.gbl.pysystemc.duplicate_string_key(key)
    return cppyy.gbl.sc_core.sc_strhash_kdup(key)


def sc_strhash_cmp(left, right):
    """Compare two nonnull NUL-terminated string keys with native strcmp.

    Args:
        left: Python str/bytes or borrowed native string pointer.
        right: Python str/bytes or borrowed native string pointer.

    Returns:
        Native integer whose sign indicates lexical ordering.

    Raises:
        TypeError: A key cannot convert to its native parameter.

    Native pointer storage must survive the call. Call on the simulation thread
    with the GIL held; no callbacks or ownership transfers occur.
    """
    if isinstance(left, (str, bytes)) or isinstance(right, (str, bytes)):
        return cppyy.gbl.pysystemc.compare_string_keys(left, right)
    return cppyy.gbl.sc_core.sc_strhash_cmp(left, right)


def sc_gen_unique_name(basename, preserve_first=False):
    """Generate a native unique name in the current module, process or context.

    Args:
        basename: Nonempty base name convertible to a native string.
        preserve_first: Return the unchanged base on its first use when true.

    Returns:
        Python string copy. Counters belong to the current construction module,
        otherwise the executing process, otherwise the current simulation context.

    Raises:
        TypeError: Arguments cannot convert to the native parameter types.
        Exception: Native report handler rejects an empty or null basename.

    Call from the simulation thread with the GIL held. Native scopes own counters;
    returned strings survive subsequent calls and scope destruction. No callbacks
    are retained by this function.
    """
    return cppyy.gbl.sc_core.sc_gen_unique_name(basename, preserve_first)


def _iterate_hierarchy_vector(vector):
    """Iterate borrowed pointers without cppyy's unsafe empty data() conversion."""
    for index in range(vector.size()):
        yield vector[index]


for _element in ("sc_core::sc_object*", "sc_core::sc_event*"):
    cppyy.gbl.std.vector[_element].__iter__ = _iterate_hierarchy_vector


def sc_get_top_level_objects(context=None):
    """Return the native vector of objects without a parent in a context.

    Args:
        context: Borrowed simulation context; None selects the current context.

    Returns:
        Borrowed, read-only native vector. The context and objects must remain
        alive while using it; hierarchy changes can invalidate element views.

    Raises:
        TypeError: context cannot convert to a native simulation-context pointer.

    Call from the simulation thread with the GIL held. No ownership transfers.
    """
    if context is None:
        context = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    return cppyy.gbl.sc_core.sc_get_top_level_objects(context)


def sc_get_top_level_events(context=None):
    """Return the native vector of events without a parent in a context.

    Args:
        context: Borrowed simulation context; None selects the current context.

    Returns:
        Borrowed, read-only native vector. The context and events must remain
        alive while using it; hierarchy changes can invalidate element views.

    Raises:
        TypeError: context cannot convert to a native simulation-context pointer.

    Call from the simulation thread with the GIL held. No ownership transfers.
    """
    if context is None:
        context = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    return cppyy.gbl.sc_core.sc_get_top_level_events(context)


def sc_pending_activity_at_current_time(context=None) -> bool:
    """Return whether work remains at the current simulation time.

    Args:
        context: Borrowed native context; None selects the current context.

    Returns:
        Native pending-work state without advancing simulation.

    Raises:
        TypeError: context cannot convert to a native context pointer.

    Use on the simulation thread with the GIL held and a live context.
    """
    if context is None:
        context = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    return cppyy.gbl.sc_core.sc_pending_activity_at_current_time(context)


def sc_pending_activity_at_future_time(context=None) -> bool:
    """Return whether timed notifications remain in the selected context.

    Args:
        context: Borrowed native context; None selects the current context.

    Returns:
        Native future-activity state without advancing simulation.

    Raises:
        TypeError: context cannot convert to a native context pointer.

    Use on the simulation thread with the GIL held and a live context.
    """
    if context is None:
        context = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    return cppyy.gbl.sc_core.sc_pending_activity_at_future_time(context)


def sc_pending_activity(context=None) -> bool:
    """Return whether current or future work remains in the selected context.

    Args:
        context: Borrowed native context; None selects the current context.

    Returns:
        Native pending-activity state without advancing simulation.

    Raises:
        TypeError: context cannot convert to a native context pointer.

    Use on the simulation thread with the GIL held and a live context.
    """
    if context is None:
        context = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    return cppyy.gbl.sc_core.sc_pending_activity(context)


def sc_time_to_pending_activity(context=None):
    """Return the native interval until the next activity in a context.

    Args:
        context: Borrowed native context; None selects the current context.

    Returns:
        Native sc_time value: zero for current work, or the next notification
        interval. With no work, native maximum time minus current time.

    Raises:
        TypeError: context cannot convert to a native context pointer.

    Use on the simulation thread with the GIL held and a live context. The
    returned time is a value, without borrowed context ownership.
    """
    if context is None:
        context = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    return cppyy.gbl.sc_core.sc_time_to_pending_activity(context)


def sc_is_running(context=None) -> bool:
    """Return whether the native simulation is running in the selected context.

    Args:
        context: Borrowed native simulation context; defaults to the current context.

    Returns:
        Native running state. The context must remain alive during this call.
    """
    if context is None:
        context = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    return cppyy.gbl.sc_core.sc_is_running(context)


def read_interface(value_type: Any, name: str, callback: Callable[[], Any]) -> Any:
    """Create a native signal read interface backed by a Python value callback.

    Args:
        value_type: Native scalar/datatype used as the interface template argument.
        name: Channel name within the current module construction scope.
        callback: Nonblocking callable returning a value convertible to value_type.

    Returns:
        Native channel with stable read/event references and notify_changed().

    Raises:
        TypeError: Callback is not callable or native value conversion fails.

    Callback and channel must survive simulation. Callback retention lasts through
    interpreter shutdown. notify_changed samples the callback and schedules a native signal update; reads
    never invoke Python. Call it from the simulation thread after changing state.
    Native bool/logic edge interfaces use the same stable signal storage.
    """
    if not callable(callback):
        raise TypeError("read interface callback must be callable")
    channel = cppyy.gbl.sc_core.PythonReadInterface[value_type](name, callback)
    _read_interface_callbacks.append(callback)
    return channel


def _read_resolved(self: Any) -> Any:
    """Return a copy of native resolved logic after the channel update phase."""
    return cppyy.gbl.sc_core.python_read_resolved(self)


if not hasattr(cppyy.gbl.sc_core.sc_signal_resolved, "read"):
    cppyy.gbl.sc_core.sc_signal_resolved.read = _read_resolved


def _time_modulo(self: Any, other: Any) -> Any:
    """Return native time remainder; reject a zero divisor with ValueError."""
    if other == cppyy.gbl.sc_core.SC_ZERO_TIME:
        raise ValueError("time modulo divisor must be nonzero")
    return cppyy.gbl.sc_core.python_time_modulo(self, other)


cppyy.gbl.sc_core.sc_time.__mod__ = _time_modulo


def __getattr__(name):
    """Resolve a native kernel name or raise AttributeError."""
    mod = _document_member(getattr(cppyy.gbl.sc_core, name))
    return mod


# Define a custom __dir__ for listing available attributes
def __dir__():
    """List Python adapters and resolvable native kernel API names."""
    return sorted(set(globals()) | set(_public_names(cppyy.gbl.sc_core)))


def sc_argv() -> tuple[str, ...]:
    """Copy the original native entry arguments into an immutable tuple.

    Returns:
        Argument strings in native order, including argv[0]. The snapshot remains
        valid after entry returns and is independent of the sc_main argument list.

    Raises:
        RuntimeError: Called outside an active native sc_main invocation.

    Call on the entry/simulation thread with the GIL held. The native array is
    borrowed only during entry; copying avoids exposing dangling pointers or
    cppyy's pointer-address conversion. No native ownership transfers.
    """
    if not cppyy.gbl.sc_core.sc_in_action:
        raise RuntimeError("native argv is only valid during sc_main")
    return tuple(str(cppyy.gbl.sc_core.python_argv_at(index)) for index in range(cppyy.gbl.sc_core.sc_argc()))


def sc_elab_and_sim(argc=None, argv=None) -> int:
    """Enter native sc_main with explicit arguments or the current sys.argv.

    Args:
        argc: Nonnegative native argument count. Omit both arguments to use sys.argv.
        argv: List or tuple of strings containing at least argc entries; only
            the first argc entries are passed, matching native count semantics.

    Returns:
        Native integer entry status, including nonzero callback failure status.

    Raises:
        TypeError: Arguments are incomplete or have invalid Python types.
        ValueError: Count is negative, exceeds native int or available entries,
            or a selected argument contains a NUL character.

    The selected argument strings are copied for the duration of entry. This
    releases the GIL while native code runs and reacquires it for Python callbacks.
    Invoke once per independent simulation process; kernel state is not reset.
    """
    if argc is None and argv is None:
        argv = list(sys.argv)
        argc = len(argv)
    elif argc is None or argv is None:
        raise TypeError("argc and argv must be supplied together")
    if isinstance(argc, bool) or not isinstance(argc, int):
        raise TypeError("argc must be an integer")
    if not isinstance(argv, (list, tuple)):
        raise TypeError("argv must be a list or tuple of strings")
    if argc < 0 or argc > 2**31 - 1 or argc > len(argv):
        raise ValueError("argc must fit native int and the available argument entries")
    argv = list(argv[:argc])
    if any(not isinstance(value, str) for value in argv):
        raise TypeError("argv entries must be strings")
    if any("\0" in value for value in argv):
        raise ValueError("argv entries must not contain NUL characters")

    log(f"Entering native simulation with {argc} arguments")

    return cppyy.gbl.sc_core.sc_elab_and_sim(argc, argv)


def sc_stop() -> None:
    """Request kernel stop unless already stopped; also used at interpreter exit."""
    if cppyy.gbl.sc_core.sc_get_status() != cppyy.gbl.sc_core.SC_STOPPED:
        log("Stopping simulation")
        cppyy.gbl.sc_core.sc_stop()


# Register the cleanup function to be called at exit
atexit.register(sc_stop)


class ScStatus(Enum):
    """Python names for native simulation lifecycle status values."""

    SC_UNITIALIZED = cppyy.gbl.sc_core.SC_UNITIALIZED
    SC_ELABORATION = cppyy.gbl.sc_core.SC_ELABORATION
    SC_BEFORE_END_OF_ELABORATION = cppyy.gbl.sc_core.SC_BEFORE_END_OF_ELABORATION
    SC_END_OF_ELABORATION = cppyy.gbl.sc_core.SC_END_OF_ELABORATION
    SC_START_OF_SIMULATION = cppyy.gbl.sc_core.SC_START_OF_SIMULATION
    SC_RUNNING = cppyy.gbl.sc_core.SC_RUNNING
    SC_PAUSED = cppyy.gbl.sc_core.SC_PAUSED
    SC_STOPPED = cppyy.gbl.sc_core.SC_STOPPED
    SC_END_OF_SIMULATION = cppyy.gbl.sc_core.SC_END_OF_SIMULATION
    SC_END_OF_INITIALIZATION = cppyy.gbl.sc_core.SC_END_OF_INITIALIZATION
    # SC_END_OF_EVALUATION         = cppyy.gbl.sc_core.SC_END_OF_EVALUATION
    SC_END_OF_UPDATE = cppyy.gbl.sc_core.SC_END_OF_UPDATE
    SC_BEFORE_TIMESTEP = cppyy.gbl.sc_core.SC_BEFORE_TIMESTEP
    SC_STATUS_LAST = cppyy.gbl.sc_core.SC_STATUS_LAST
    SC_STATUS_ANY = cppyy.gbl.sc_core.SC_STATUS_ANY

    @classmethod
    def from_cpp_enum(cls, cpp_enum_value):
        """Convert a SystemC sc_status int value to the corresponding ScStatus enum."""
        return ScStatus(cpp_enum_value)

    def __str__(self):
        """Return the string representation of the ScStatus enum."""
        return self.name


def sc_status() -> ScStatus:
    """Return the current kernel lifecycle state as ScStatus."""
    status = cppyy.gbl.sc_core.sc_get_status()
    return ScStatus(status)  # Convert to Python enum


# Native waits suspend an OS thread. Release the GIL for every overload so
# other Python SystemC processes can acquire it while this process waits.
cppyy.gbl.sc_core.wait.__release_gil__ = True
cppyy.gbl.sc_core.sc_join.wait.__release_gil__ = True
_spawn_callbacks = []


def sc_spawn(callback: Callable[[], Any], name: str = "", options: Any = None) -> Any:
    """Spawn a Python callable as a native dynamic SystemC process.

    Args:
        callback: No-argument callable. Return values are ignored; use a Python
            closure to retain results when joining processes.
        name: Optional native process name.
        options: Optional native sc_spawn_options object.

    Returns:
        Native sc_process_handle with termination event and process controls.

    Callbacks remain retained for this interpreter's single simulation lifetime.
    Blocking native calls within callbacks must release the GIL.
    """
    if not callable(callback):
        raise TypeError("sc_spawn callback must be callable")
    if options is None:
        handle = cppyy.gbl.sc_core.spawn_callback(callback, name)
    else:
        handle = cppyy.gbl.sc_core.spawn_callback(callback, name, options)
    _spawn_callbacks.append(callback)
    return handle


_report_callback = None


def set_report_handler(callback: Callable[[Any, int], None] | None = None) -> None:
    """Install a retained Python report handler, or restore the native default.

    The callback receives a native sc_report and an integer action bitmask.
    This adapter avoids cppyy's const-reference scalar callback conversion.
    """
    global _report_callback
    if callback is None:
        cppyy.gbl.sc_core.restore_report_handler()
        _report_callback = None
    else:
        if not callable(callback):
            raise TypeError("report handler must be callable")
        _report_callback = callback
        cppyy.gbl.sc_core.set_python_report_handler(callback)


_native_next_trigger = cppyy.gbl.sc_core.next_trigger


def next_trigger(*args: Any) -> Any:
    """Set dynamic method sensitivity using native arguments or event expressions.

    Args:
        *args: Native next_trigger time/event arguments. Expressions are consumed
            once; referenced events must outlive the scheduled invocation.
    """
    if len(args) == 1 and any(
        isinstance(args[0], cppyy.gbl.sc_core.sc_event_expr[list_type])
        for list_type in (cppyy.gbl.sc_core.sc_event_or_list, cppyy.gbl.sc_core.sc_event_and_list)
    ):
        return cppyy.gbl.sc_core.python_next_trigger(args[0])
    return _native_next_trigger(*args)


def _process_control(adapter):
    """Bind an explicit native control adapter with GIL released."""
    adapter.__release_gil__ = True

    def control(self, *args):
        """Control a target from a running SystemC process; paused calls raise RuntimeError."""
        if cppyy.gbl.sc_core.sc_get_status() != cppyy.gbl.sc_core.SC_RUNNING:
            raise RuntimeError("process reset/kill requires a running SystemC process")
        if not adapter(self, *args):
            raise _ProcessUnwind()

    return control


cppyy.gbl.sc_core.sc_process_handle.kill = _process_control(cppyy.gbl.sc_core.python_kill)
cppyy.gbl.sc_core.sc_process_handle.reset = _process_control(cppyy.gbl.sc_core.python_reset)


cppyy.gbl.sc_core.python_wait.__release_gil__ = True


class _ProcessUnwind(BaseException):
    """Private signal that unwinds Python frames before native reset or kill."""


def wait(*args: Any) -> None:
    """Suspend a native thread with cycle, time, event or event-list sensitivity.

    Args:
        *args: Native wait arguments, including event OR/AND expressions.

    Reset and kill unwind Python frames, execute finally blocks, and then restore
    the original native scheduler exception. Do not suppress BaseException or
    wait during unwind cleanup. Events and sensitivity targets must outlive waits.
    Blocking scheduler calls release the GIL.
    """
    if not cppyy.gbl.sc_core.python_wait(*args):
        raise _ProcessUnwind()


# Publish native classes/enums and Python adapters for wildcard imports.

__all__ = sorted(
    set(_public_names(cppyy.gbl.sc_core))
    | {
        "ScStatus",
        "sc_status",
        "sc_trace",
        "sc_spawn",
        "wait",
        "set_report_handler",
        "sc_elab_and_sim",
        "sc_stop",
        "vcd_trace",
        "interface",
        "release_interface_callbacks",
        "teardown_interfaces",
        "declare_interface",
        "channel_interface",
        "read_interface",
        "signal_channel",
        "fifo_channel",
    }
)

__doc__ = _namespace_help(globals(), cppyy.gbl.sc_core)
