"""Native kernel, channels, processes and Python callback adapters.

Callbacks are retained for their native process lifetime. Blocking scheduler
operations release the GIL. Guarded Python waits preserve native reset/kill
unwind in tested cases. User-exception injection accepts native copyable
exception values; targets must catch them before returning to the kernel.
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
from ._hashes import install as _install_hashes

_install_hashes()

SCRIPT_PATH = pathlib.Path(__file__).parent.absolute()
for _bridge in (
    "sc_main.cpp",
    "native_entry.cpp",
    "sc_module.cpp",
    "interface_teardown.cpp",
    "hash_helpers.cpp",
    "report_legacy.cpp",
    "report_handler.cpp",
    "event_lists.cpp",
    "event_finders.cpp",
    "trace_printf.cpp",
    "time_operations.cpp",
    "byte_heap.cpp",
    "process_utilities.cpp",
    "event_expressions.cpp",
    "pvector_access.cpp",
):
    with (SCRIPT_PATH / _bridge).open() as _source:
        cppyy.cppdef(_source.read())

from ._event_finders import (
    EventFinderTemplate as _EventFinderTemplate,
    CachedFinderCreator as _CachedFinderCreator,
)

sc_event_finder_t = _EventFinderTemplate()
sc_event_finder_cache = cppyy.gbl.pysystemc_public.event_finder_cache
cppyy.gbl.sc_core.sc_event_finder.cached_create = _CachedFinderCreator()

cppyy.gbl.sc_core.sc_start.__release_gil__ = True
cppyy.gbl.sc_core.sc_initialize.__release_gil__ = True
cppyy.gbl.sc_core.sc_elab_and_sim.__release_gil__ = True
cppyy.gbl.sc_main.__release_gil__ = True


def _event_list_constructor(list_type):
    """Preserve native constructors while consuming matching event expressions."""
    native_init = list_type.__init__
    expression_type = cppyy.gbl.sc_core.sc_event_expr[list_type]

    def initialize(self, *args):
        """Construct a list from native arguments or a single-use expression.

        Referenced events must outlive the list and any waiting processes.
        Expressions are consumed once; other arguments retain native conversion
        errors. Construction runs with the GIL held on the kernel thread.
        """
        if len(args) == 1 and isinstance(args[0], expression_type):
            native_init(self)
            cppyy.gbl.sc_core.python_assign_event_expression[list_type](self, args[0])
        else:
            native_init(self, *args)

    list_type.__init__ = initialize


for _event_list_type in (cppyy.gbl.sc_core.sc_event_or_list, cppyy.gbl.sc_core.sc_event_and_list):
    _event_list_constructor(_event_list_type)

from ._event_expressions import install as _install_event_expressions

_install_event_expressions()

from ._clock import install as _install_clock

cppyy.include(str(pathlib.Path(__file__).with_name("vector_creator.cpp")))
from ._vectors import install as _install_vectors, assemble_vector as _assemble_vector

_install_vectors()
cppyy.include(str(pathlib.Path(__file__).with_name("vector_policies.cpp")))
from ._vector_policies import install as _install_vector_policies, MemberAccessTemplate as _MemberAccessTemplate

_install_vector_policies()


from ._pvector import install as _install_pvector
from ._list_handles import install as _install_list_handles

_install_clock()
_install_pvector()
_install_list_handles()


def _event_notify_delayed(self, *args):
    """Notify a borrowed event with native no-argument, time or value/unit forms.

    Native pending-notification reports propagate as sc_report. Keep the event
    alive until delivery; call on the kernel thread with the GIL held.
    """
    if not args:
        return cppyy.gbl.sc_core.python_event_notify_delayed(self)
    if len(args) == 1:
        return cppyy.gbl.sc_core.python_event_notify_delayed_time(self, args[0])
    if len(args) == 2:
        return cppyy.gbl.sc_core.python_event_notify_delayed_units(self, *args)
    raise TypeError("notify_delayed expects zero, one or two arguments")


cppyy.gbl.sc_core.sc_event.notify_delayed = _event_notify_delayed


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
from ._exports import ExportTemplate as _ExportTemplate

from ._streams import install_fifo_streams as _install_fifo_streams

_install_fifo_streams()
sc_fifo = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo)
sc_fifo_in = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo_in)
sc_fifo_out = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo_out)
sc_fifo_in_if = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo_in_if)
sc_fifo_out_if = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo_out_if)
sc_fifo_blocking_in_if = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo_blocking_in_if)
sc_fifo_blocking_out_if = _FifoTemplate(cppyy.gbl.sc_core.sc_fifo_blocking_out_if)
from ._blocking import guard_native_channels as _guard_native_channels

_guard_native_channels()
from ._blocking import ScopedLock as _ScopedLock

sc_scoped_lock = _ScopedLock

# Keep the public port specialization native; only Python bind gains adaptation.
sc_port = _PortTemplate(cppyy.gbl.sc_core.sc_port, channel_interface)
sc_export = _ExportTemplate(_PortTemplate(cppyy.gbl.sc_core.sc_export, channel_interface))
_read_interface_callbacks = []


def _legacy_report_method(name):
    """Publish an omitted native static method with its native call signature."""
    native = getattr(cppyy.gbl.pysystemc, "report_" + name)

    def method(*args):
        """Invoke native legacy report policy on the simulation thread.

        Args:
            args: Native method arguments: an ID/message, ID/bool, or bool.

        Returns:
            Native result; message strings are copied, suppression is bool, setters
            return None. Global configuration persists for the kernel lifetime.

        Raises:
            TypeError: Arguments cannot convert to native parameters.
            Exception: Native report policy rejects registration or a callback fails.

        Hold the GIL; no ownership transfer. Native deprecation reports and any
        configured callback execute synchronously with their native action policy.
        """
        return native(*args)

    return staticmethod(method)


for _method in (
    "register_id",
    "get_message",
    "is_suppressed",
    "suppress_id",
    "suppress_infos",
    "suppress_warnings",
    "make_warnings_errors",
):
    setattr(cppyy.gbl.sc_core.sc_report, _method, _legacy_report_method(_method))


def _assign_report(self, source):
    """Copy source report fields into this report and return the borrowed self.

    Args:
        source: Live native report; self-assignment preserves values.

    Returns:
        Borrowed native reference to self. Both reports retain their own fields.

    Raises:
        TypeError: source cannot convert to a native report.
        Exception: Native copying/allocation fails.

    Call on the simulation thread with the GIL held; no callbacks or ownership
    transfer. Any borrowed field views require the owning report to remain alive.
    """
    return cppyy.gbl.pysystemc.report_assign(self, source)


cppyy.gbl.sc_core.sc_report.__assign__ = _assign_report


def _report_stop_after(subject, *args):
    """Set a native severity, message or message/severity stop threshold.

    Args:
        subject: Native severity enum or message-type string.
        args: Optional integer limit, or severity enum and optional limit for
            a message. Omitted limit is -1, removing the selected threshold.

    Returns:
        Previous native threshold, -1 when no selected threshold was set.

    Raises:
        TypeError: Arguments do not match a native stop_after signature.

    Call on the simulation thread with GIL held. Limits persist in native policy
    storage. No ownership transfers; diagnostics follow the configured handler.
    """
    bridge = cppyy.gbl.pysystemc
    severity_type = cppyy.gbl.sc_core.sc_severity
    if isinstance(subject, severity_type) and len(args) <= 1:
        return bridge.report_stop_severity(subject, args[0] if args else -1)
    if isinstance(subject, (str, bytes)):
        if args and isinstance(args[0], severity_type) and len(args) <= 2:
            return bridge.report_stop_message_severity(subject, args[0], args[1] if len(args) == 2 else -1)
        if len(args) <= 1:
            return bridge.report_stop_message(subject, args[0] if args else -1)
    raise TypeError("stop_after expects severity/limit, message/limit or message/severity/limit")


cppyy.gbl.sc_core.sc_report_handler.stop_after = staticmethod(_report_stop_after)


def _report_get_handler():
    """Return a borrowed native handler pointer token usable by set_handler.

    Returns:
        Native pointer token; it does not retain Python callback ownership.

    Hold the GIL on the simulation thread. Python dispatchers require the callback
    registered through set_report_handler to remain retained and active.
    """
    return cppyy.gbl.pysystemc.report_get_handler()


def _report_set_handler(handler):
    """Install a native pointer token or restore the default handler with None.

    Args:
        handler: Token from get_handler/set_handler, or None for native default.

    Returns:
        Previous borrowed pointer token; tokens do not own callback references.

    Raises:
        TypeError: handler is not a native pointer token or None.

    Use set_report_handler for Python callables and callback retention. Hold the
    GIL on the simulation thread; saved Python dispatcher tokens require an active
    retained callback before reinstalling them.
    """
    if handler is None:
        return cppyy.gbl.pysystemc.report_reset_handler()
    if not isinstance(handler, cppyy.gbl.pysystemc.report_handler_pointer):
        raise TypeError("handler must be a native handler pointer token or None")
    return cppyy.gbl.pysystemc.report_set_handler(handler)


cppyy.gbl.sc_core.sc_report_handler.get_handler = staticmethod(_report_get_handler)
cppyy.gbl.sc_core.sc_report_handler.set_handler = staticmethod(_report_set_handler)


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

from ._time import install as _install_time

_install_time(cppyy.gbl.sc_core.sc_time)


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


def _prepare_entry_arguments(argc, argv):
    """Validate and own the selected strings before entering a native callback."""
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

    return argc, argv


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
    argc, argv = _prepare_entry_arguments(argc, argv)

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


def _join_wait(join, clocked=False):
    """Wait on a borrowed join, releasing GIL and preserving reset/kill cleanup.

    Keep the join alive until all monitored processes exit, including after a
    waiter is reset or killed. Clocked waits require static sensitivity and
    always wait at least one cycle. Native process/wait reports propagate.
    """
    operation = cppyy.gbl.sc_core.python_join_wait
    operation.__release_gil__ = True
    if not operation(join, clocked):
        raise _ProcessUnwind()


def _join_wait_clocked(join):
    """Wait using native static sensitivity and guarded scheduler unwind.

    Join storage and monitored processes remain borrowed for the entire wait.
    The call releases GIL; invalid native waiting context reports propagate.
    """
    return _join_wait(join, True)


cppyy.gbl.sc_core.sc_join.wait = _join_wait
cppyy.gbl.sc_core.sc_join.wait_clocked = _join_wait_clocked
_spawn_callbacks = []


cppyy.include(str(pathlib.Path(__file__).with_name("bind.cpp")))
sc_placeholder = cppyy.gbl.sc_core.python_placeholder
_bind_owners = []


def sc_bind(callback: Any, *arguments: Any) -> Any:
    """Bind native callable arguments with copying, references and placeholders.

    Args:
        callback: Native functor, function pointer or typed std::function proxy.
        *arguments: Native values, numeric/string scalars, sc_ref/sc_cref wrappers,
            nested bindings or sc_placeholder[index] instances.

    Returns:
        Owning native callable, suitable for direct calls or sc_spawn.

    Raises:
        TypeError: Callable or arguments cannot select a native value type.

    Values are copied by std::bind; reference wrappers borrow their targets.
    Proxies remain retained for the single kernel lifetime. Python callbacks in
    typed std::function values must also remain alive through their invocations.
    Guarded waits preserve reset/kill cleanup. Placeholder indices are one-based
    and instantiate on demand. Native conversion and callback errors propagate.
    """
    function_type = getattr(callback, "__cpp_name__", None) or getattr(type(callback), "__cpp_name__", None)
    if function_type is None or not callable(callback):
        raise TypeError("sc_bind requires a native callable or typed std::function")
    types = []
    for argument in arguments:
        native_type = getattr(type(argument), "__cpp_name__", None)
        if native_type is None:
            native_type = {bool: "bool", float: "double", str: "std::string"}.get(type(argument))
            if type(argument) is int:
                native_type = "int" if -(1 << 31) <= argument < (1 << 31) else "long long"
        if native_type is None:
            raise TypeError("sc_bind arguments require native values or numeric/string scalars")
        types.append(native_type)
    bound = cppyy.gbl.sc_core.python_bound[function_type, *types](callback, *arguments)
    _bind_owners.append((bound, callback, arguments))
    return bound


cppyy.include(str(pathlib.Path(__file__).with_name("spawn_result.cpp")))


def sc_spawn(callback: Any, *args: Any, name: str = "", options: Any = None) -> Any:
    """Spawn a dynamic process, optionally storing its typed native result.

    Args:
        callback: No-argument callable, or caller-owned native result storage
            for the ``sc_spawn(result, callback, name, options)`` overload.
        *args: Callback after result storage, then optional name and options.
        name: Optional native process name (positional or keyword).
        options: Optional native sc_spawn_options (positional or keyword).

    Returns:
        Native sc_process_handle with termination event and process controls.

    Raises:
        TypeError: Callback is not callable, arguments are duplicated/invalid,
            or result storage has no writable native value type.
        sc_report: Native process creation or execution reports a failure.

    Without result storage, return values are ignored. Result storage may be any
    native C++ value proxy or a ctypes scalar; the callback must return a value
    convertible to that type. The result is assigned only after a successful
    callback return. Reset/kill executes Python cleanup before restoring native
    scheduler unwind. Callbacks and result proxies remain retained for the
    interpreter's single kernel lifetime. Use fresh processes for unrelated runs.
    """
    result = None
    if not callable(callback) or (args and callable(args[0])):
        result = callback
        if result is None:
            raise TypeError("sc_spawn result storage cannot be null")
        if not args or not callable(args[0]):
            raise TypeError("sc_spawn callback must be callable")
        callback, args = args[0], args[1:]
    if len(args) > 2:
        raise TypeError("sc_spawn accepts a callback, optional name and options")
    if args:
        if name:
            raise TypeError("sc_spawn name supplied twice")
        name = args[0]
    if len(args) == 2:
        if options is not None:
            raise TypeError("sc_spawn options supplied twice")
        options = args[1]
    if result is None:
        operation = (
            cppyy.gbl.sc_core.sc_spawn[type(callback).__cpp_name__]
            if getattr(type(callback), "__cpp_name__", None)
            else cppyy.gbl.sc_core.spawn_callback
        )
        handle = operation(callback, name) if options is None else operation(callback, name, options)
        owners = callback
    else:
        result_type = getattr(type(result), "__cpp_name__", None)
        if result_type is None:
            import ctypes

            if not isinstance(result, ctypes._SimpleCData):
                raise TypeError("sc_spawn result must be a native C++ value or ctypes scalar")
            result_type = {
                "?": "bool",
                "b": "signed char",
                "B": "unsigned char",
                "h": "short",
                "H": "unsigned short",
                "i": "int",
                "I": "unsigned int",
                "l": "long",
                "L": "unsigned long",
                "q": "long long",
                "Q": "unsigned long long",
                "f": "float",
                "d": "double",
                "g": "long double",
            }.get(result._type_)
            if result_type is None:
                raise TypeError("sc_spawn result requires scalar value storage, not a pointer")
        operation = cppyy.gbl.sc_core.python_spawn_result[result_type]
        handle = operation(result, callback, name) if options is None else operation(result, callback, name, options)
        owners = (result, callback)
    _spawn_callbacks.append(owners)
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
            once; referenced events must outlive the scheduled invocation. An
            optional native simulation context follows the sensitivity arguments.

    Returns:
        None after configuring the next method invocation.

    Raises:
        sc_report: The native process context does not permit dynamic sensitivity.
        TypeError: Arguments do not match a native sensitivity overload, or cppyy
            wraps an invalid-process native report; its diagnostic is retained.
    """
    if any(
        isinstance(value, cppyy.gbl.sc_core.sc_event_expr[list_type])
        for value in args
        for list_type in (cppyy.gbl.sc_core.sc_event_or_list, cppyy.gbl.sc_core.sc_event_and_list)
    ):
        return cppyy.gbl.sc_core.python_next_trigger(*args)
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


cppyy.include(str(pathlib.Path(__file__).with_name("process_exceptions.cpp")))
_throw_adapters = {}


def _process_throw_it(self, exception: Any, descendants: Any = None) -> None:
    """Inject a native exception into a process, optionally including descendants.

    Args:
        exception: Copyable native C++ exception value, caught by the target.
        descendants: Native descendant policy; omitted means SC_NO_DESCENDANTS.

    Raises:
        TypeError: Exception has no native C++ type.
        sc_report: Native process restrictions or an uncaught exception fail.

    The kernel copies the borrowed exception during this call. Target Python
    callbacks can catch native exception classes around guarded waits. Injection
    releases the GIL until the target yields, and preserves reset/kill cleanup.
    Keep the handle and target dependencies alive through process termination.
    """
    exception_type = getattr(type(exception), "__cpp_name__", None)
    if exception_type is None:
        raise TypeError("throw_it requires a native C++ exception value")
    adapter = _throw_adapters.get(exception_type)
    if adapter is None:
        adapter = cppyy.gbl.sc_core.python_throw_helper[exception_type].invoke
        adapter.__release_gil__ = True
        _throw_adapters[exception_type] = adapter
    if descendants is None:
        descendants = cppyy.gbl.sc_core.SC_NO_DESCENDANTS
    if not adapter(self, exception, descendants):
        raise _ProcessUnwind()


cppyy.gbl.sc_core.sc_process_handle.throw_it = _process_throw_it


cppyy.gbl.sc_core.sc_process_handle.kill = _process_control(cppyy.gbl.sc_core.python_kill)
cppyy.gbl.sc_core.sc_process_handle.reset = _process_control(cppyy.gbl.sc_core.python_reset)
# Native sticky synchronous-reset controls retain their descendant policy.
cppyy.gbl.sc_core.sc_process_handle.sync_reset_on.__release_gil__ = True
cppyy.gbl.sc_core.sc_process_handle.sync_reset_off.__release_gil__ = True

_native_process_suspend = cppyy.gbl.sc_core.sc_process_handle.suspend
_running_process_suspend = _process_control(cppyy.gbl.sc_core.python_suspend)


def _process_suspend(self, *args):
    """Suspend a process using native descendant inclusion semantics.

    Args:
        *args: Optional native descendant inclusion policy.

    Suspension before simulation or while paused retains native behavior.
    Self-suspension from a running callback releases the GIL and preserves native
    reset/kill unwind. Retain this handle and callback dependencies until resumed
    or terminated. Native report/conversion errors propagate.
    """
    if cppyy.gbl.sc_core.sc_get_status() == cppyy.gbl.sc_core.SC_RUNNING:
        return _running_process_suspend(self, *args)
    return _native_process_suspend(self, *args)


cppyy.gbl.sc_core.sc_process_handle.suspend = _process_suspend


cppyy.gbl.sc_core.python_wait.__release_gil__ = True


class _ProcessUnwind(BaseException):
    """Private signal that unwinds Python frames before native reset or kill."""


def wait(*args: Any) -> None:
    """Suspend a native thread with cycle, time, event or event-list sensitivity.

    Args:
        *args: Native wait arguments, including event OR/AND expressions and an
            optional trailing borrowed simulation context.

    Returns:
        None when the native wait completes normally.

    Raises:
        sc_report: The native process context does not permit the selected wait.
        TypeError: Arguments do not match a native wait overload.

    Reset and kill unwind Python frames, execute finally blocks, and then restore
    the original native scheduler exception. Do not suppress BaseException or
    wait during unwind cleanup. Events and sensitivity targets must outlive waits.
    Blocking scheduler calls release the GIL.
    """
    native = cppyy.gbl.sc_core
    operation = native.python_wait
    # A native diagnostic must not retry a different wait and enter the kernel.
    if args and isinstance(args[0], int) and (len(args) == 1 or isinstance(args[1], native.sc_simcontext)):
        operation = operation.__overload__("int, sc_core::sc_simcontext*")
    elif args and isinstance(args[0], (native.sc_event_or_list, native.sc_event_and_list)):
        name = type(args[0]).__cpp_name__
        operation = operation.__overload__(f"const {name}&, sc_core::sc_simcontext*")
    operation.__release_gil__ = True
    if not operation(*args):
        raise _ProcessUnwind()


def _time_left_multiply(time, factor):
    """Return native factor * time, preserving rounding and leaving time unchanged.

    The time is borrowed for this non-suspending call with GIL held. The result
    owns independent native storage; invalid numeric conversion raises TypeError.
    """
    return cppyy.gbl.pysystemc_public.time_left_multiply(factor, time)


cppyy.gbl.sc_core.sc_time.__rmul__ = _time_left_multiply

from ._byte_heap import allocate as _byte_heap_allocate

cppyy.gbl.sc_core.sc_byte_heap.allocate = _byte_heap_allocate


def halt(simc=None):
    """Halt the current clocked thread with native scheduling and Python cleanup.

    Args:
        simc: Borrowed native context; defaults to the current simulation context.

    Returns:
        Does not return normally; the native thread terminates after suspension.

    Raises:
        sc_report: Called outside a clocked thread.

    The call releases GIL, executes Python finally blocks, and restores native
    halt/reset/kill control flow. Do not suppress the cleanup exception or wait
    during cleanup. Keep the context alive through the suspended call.
    """
    operation = cppyy.gbl.sc_core.python_halt
    operation.__release_gil__ = True
    if simc is None:
        simc = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    if not operation(simc):
        raise _ProcessUnwind()


def sc_set_stack_size(thread_h, size):
    """Set native thread stack size before its first execution.

    Args:
        thread_h: Borrowed native thread pointer or thread/clocked-thread handle.
        size: Positive native byte count.

    Returns:
        None after native stack configuration.

    Raises:
        TypeError: Size or thread argument has an incompatible type.
        ValueError: Size is not positive or the selected handle/pointer is invalid.
        sc_report: Native stack configuration report.

    The GIL remains held; this does not advance simulation. Retain the thread
    through configuration. Changing an already prepared coroutine's stack does
    not resize its active native stack.
    """
    if isinstance(size, bool) or not isinstance(size, int):
        raise TypeError("stack size must be an integer")
    if size <= 0:
        raise ValueError("stack size must be positive")
    if isinstance(thread_h, cppyy.gbl.sc_core.sc_process_handle):
        if not thread_h.valid() or thread_h.proc_kind() not in (
            cppyy.gbl.sc_core.SC_THREAD_PROC_,
            cppyy.gbl.sc_core.SC_CTHREAD_PROC_,
        ):
            raise ValueError("stack size requires a thread or clocked-thread handle")
    elif thread_h is None or thread_h is cppyy.nullptr:
        raise ValueError("stack size requires a live thread pointer")
    native = cppyy.gbl.pysystemc_public
    operation = (
        native.set_stack_size_handle
        if isinstance(thread_h, cppyy.gbl.sc_core.sc_process_handle)
        else native.set_stack_size_pointer
    )
    return operation(thread_h, size)


from ._trace_printf import tprintf as tprintf

from ._scheduler import sc_start as sc_start, at_posedge as at_posedge, at_negedge as at_negedge, timed_out as timed_out

# Publish native classes/enums and Python adapters for wildcard imports.

from ._references import sc_ref as sc_ref, sc_cref as sc_cref

sc_assemble_vector = _assemble_vector
sc_member_access = _MemberAccessTemplate()

__all__ = sorted(
    set(_public_names(cppyy.gbl.sc_core))
    | {
        "ScStatus",
        "sc_status",
        "sc_trace",
        "sc_spawn",
        "sc_bind",
        "sc_placeholder",
        "sc_assemble_vector",
        "sc_member_access",
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

from ._kernel_flags import install as _install_kernel_flags

_install_kernel_flags(sys.modules[__name__])
