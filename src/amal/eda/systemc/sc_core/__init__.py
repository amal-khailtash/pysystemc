"""Native kernel, channels, processes and Python callback adapters.

Callbacks are retained for their native process lifetime. Blocking scheduler
operations release the GIL; process reset remains explicitly unsupported.
"""

from .._logging import log
import atexit
from contextlib import contextmanager
from collections.abc import Callable, Iterator
from typing import Any
from enum import Enum
import pathlib
import sys
import re

# The parent package selects the JIT C++ standard before importing cppyy.

import cppyy

#

SCRIPT_PATH = pathlib.Path(__file__).parent.absolute()


with open(SCRIPT_PATH / "sc_main.cpp") as fd:
    cppyy.cppdef(fd.read())

# Compile the licensed local SystemC entry adapter; no upstream source tree is needed.
with open(SCRIPT_PATH / "native_entry.cpp") as fd:
    cppyy.cppdef(fd.read())

# Store the sc_core.sc_module and its metaclass
# sc_module           = cppyy.gbl.sc_core.sc_module
# sc_module_metaclass = type(cppyy.gbl.sc_core.sc_module)

with open(SCRIPT_PATH / "sc_module_new.cpp") as fd:
    cppyy.cppdef(fd.read())


# print(f"type(sc_core.sc_module): {type(cppyy.gbl.sc_core.sc_module)}")

# # ScModule           = cppyy.gbl.sc_core.ScModule
# # ScModule.__class__ = sc_module_metaclass

# # Replace sc_core.sc_module with the newly defined ScModule
# # Replace the C++ sc_module with the C++ wrapper ScModule

# # cppyy.gbl.sc_core.sc_module = ScModule

# cppyy.gbl.sc_core.ScModule.__release_gil__ = True
# cppyy.gbl.sc_core.ScModule.call_process.__release_gil__ = True
cppyy.gbl.sc_core.sc_start.__release_gil__ = True
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

# Replace with custom sc_module
# Keep the native namespace intact; expose the adapter only in Python.
# # cppyy.gbl.sc_core.sc_module.__class__ = sc_module_metaclass
# # globals()["sc_module"] = sc_core.sc_module

# print(f"type(sc_core.sc_module): {type(cppyy.gbl.sc_core.sc_module)}")


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


# Compatibility alias for Python modeling; native C++ sc_module remains intact.
sc_module = cppyy.gbl.sc_core.ScModule
sc_channel = sc_module
sc_interface = cppyy.gbl.sc_core.PythonInterface
sc_prim_channel = cppyy.gbl.sc_core.PythonPrimChannel
signal_channel = cppyy.gbl.sc_core.PythonSignalChannel
fifo_channel = cppyy.gbl.sc_core.PythonFifoChannel
_read_interface_callbacks = []


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


_interface_declarations = {}
_channel_interfaces = {}


def channel_interface(interface: Any) -> Any:
    """Return a Python-subclassable native module implementing an interface.

    Args:
        interface: Native interface class returned by declare_interface or cppyy.

    Returns:
        Native combined base with an sc_module_name constructor.

    The caller retains the channel throughout simulation. This combines native
    bases before Python subclassing to avoid conflicting cppyy metaclasses.
    Abstract interface methods must be implemented by the Python subclass.
    Combined TLM forward interfaces currently use Python debug_transport callbacks.
    Native callers use transport_dbg, which validates the unsigned return range.
    Debug callbacks must never wait. TLM-1 transport and blocking peek use
    transport_value and peek_value callbacks; native methods preserve value and
    reference-output overloads. Callback naming parity remains pending.
    """
    name = interface.__cpp_name__
    cache_name = name
    if name not in _channel_interfaces:
        identifier = f"ChannelInterface{len(_channel_interfaces)}"
        methods = ""
        if name.startswith("tlm::tlm_nonblocking_get_peek_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "NonblockingGetPeekAdapter"):
                cppyy.cppdef(
                    "/** @brief Combined nonblocking get/peek bridge. @details Ports borrow the owner; "
                    "the adapter owns both readiness events. cppyy acquires GIL for callbacks, which "
                    "must not wait. Exceptions propagate; failed calls preserve output references. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class NonblockingGetPeekAdapter : public tlm::tlm_nonblocking_get_peek_if<T> { "
                    "sc_core::sc_event get_ready, peek_ready; public: "
                    "bool nb_get(T& value) override { auto result = get_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_peek(T& value) const override { auto result = peek_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_can_get(tlm::tlm_tag<T>* = nullptr) const override { return can_get(); } "
                    "bool nb_can_peek(tlm::tlm_tag<T>* = nullptr) const override { return can_peek(); } "
                    "const sc_core::sc_event& ok_to_get(tlm::tlm_tag<T>* = nullptr) const override { return get_ready; } "
                    "const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { "
                    "get_ready.notify(delay); peek_ready.notify(delay); } "
                    "virtual std::pair<bool, T> get_value() = 0; "
                    "virtual std::pair<bool, T> peek_value() const = 0; "
                    "virtual bool can_get() const = 0; virtual bool can_peek() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_nonblocking_get_peek_if<").removesuffix(">")
            name = f"pysystemc_interfaces::NonblockingGetPeekAdapter<{argument}>"
        if name.startswith("tlm::tlm_nonblocking_get_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "NonblockingGetAdapter"):
                cppyy.cppdef(
                    "/** @brief Nonblocking get value/event bridge. @details Ports borrow the owner; "
                    "event storage belongs to the adapter. Callbacks acquire GIL and must not wait. "
                    "Value assignment occurs only after successful callback; exceptions propagate. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class NonblockingGetAdapter : public tlm::tlm_nonblocking_get_if<T> { "
                    "sc_core::sc_event ready; public: "
                    "bool nb_get(T& value) override { auto result = get_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_can_get(tlm::tlm_tag<T>* = nullptr) const override { return can_get(); } "
                    "const sc_core::sc_event& ok_to_get(tlm::tlm_tag<T>* = nullptr) const override { return ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { ready.notify(delay); } "
                    "virtual std::pair<bool, T> get_value() = 0; virtual bool can_get() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_nonblocking_get_if<").removesuffix(">")
            name = f"pysystemc_interfaces::NonblockingGetAdapter<{argument}>"
        if name.startswith("tlm::tlm_nonblocking_peek_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "NonblockingPeekAdapter"):
                cppyy.cppdef(
                    "/** @brief Nonblocking peek value/event bridge. @details Ports borrow the owner; "
                    "event storage belongs to the adapter. Callbacks acquire GIL and must not wait. "
                    "Value assignment occurs only after successful callback; exceptions propagate. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class NonblockingPeekAdapter : public tlm::tlm_nonblocking_peek_if<T> { "
                    "sc_core::sc_event ready; public: "
                    "bool nb_peek(T& value) const override { auto result = peek_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_can_peek(tlm::tlm_tag<T>* = nullptr) const override { return can_peek(); } "
                    "const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<T>* = nullptr) const override { return ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { ready.notify(delay); } "
                    "virtual std::pair<bool, T> peek_value() const = 0; virtual bool can_peek() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_nonblocking_peek_if<").removesuffix(">")
            name = f"pysystemc_interfaces::NonblockingPeekAdapter<{argument}>"
        if name.startswith("tlm::tlm_nonblocking_put_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "NonblockingPutAdapter"):
                cppyy.cppdef(
                    "/** @brief Nonblocking put value/event bridge. @details Ports borrow the owner. "
                    "The adapter owns its event; callbacks acquire GIL and must not wait. Errors propagate. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class NonblockingPutAdapter : public tlm::tlm_nonblocking_put_if<T> { "
                    "sc_core::sc_event ready; public: "
                    "bool nb_put(const T& value) override { return put_value(value); } "
                    "bool nb_can_put(tlm::tlm_tag<T>* = nullptr) const override { return can_put(); } "
                    "const sc_core::sc_event& ok_to_put(tlm::tlm_tag<T>* = nullptr) const override { return ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { ready.notify(delay); } "
                    "virtual bool put_value(T) = 0; virtual bool can_put() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_nonblocking_put_if<").removesuffix(">")
            name = f"pysystemc_interfaces::NonblockingPutAdapter<{argument}>"
        if name.startswith("tlm::tlm_transport_if<"):
            # Template parameter names avoid parsing nested C++ type arguments.
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "TransportAdapter"):
                cppyy.cppdef(
                    "/** @brief Value-copy TLM response bridge. @details Ports borrow the owner; "
                    "cppyy acquires GIL for callbacks. Blocking callers release GIL. Errors propagate. */ "
                    "namespace pysystemc_interfaces { template<class Req, class Rsp> "
                    "class TransportAdapter : public tlm::tlm_transport_if<Req, Rsp> { public: "
                    "Rsp transport(const Req& req) override { return transport_value(req); } "
                    "void transport(const Req& req, Rsp& rsp) override { rsp = transport_value(req); } "
                    "virtual Rsp transport_value(Req) = 0; }; }"
                )
            arguments = name.removeprefix("tlm::tlm_transport_if<").removesuffix(">")
            name = f"pysystemc_interfaces::TransportAdapter<{arguments}>"
        if name.startswith("tlm::tlm_blocking_get_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "GetAdapter"):
                cppyy.cppdef(
                    "/** @brief Value-copy TLM get bridge. @details Ports borrow the owner; "
                    "cppyy acquires GIL for callbacks. Blocking callers release GIL. Errors propagate. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class GetAdapter : public tlm::tlm_blocking_get_if<T> { public: "
                    "T get(tlm::tlm_tag<T>* = nullptr) override { return get_value(); } "
                    "void get(T& value) override { value = get_value(); } "
                    "virtual T get_value() = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_blocking_get_if<").removesuffix(">")
            name = f"pysystemc_interfaces::GetAdapter<{argument}>"
        if name.startswith("tlm::tlm_blocking_peek_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "PeekAdapter"):
                cppyy.cppdef(
                    "/** @brief Value-copy TLM peek bridge. @details Ports borrow the owner; "
                    "cppyy acquires GIL for callbacks. Errors propagate; retain owner through simulation. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class PeekAdapter : public tlm::tlm_blocking_peek_if<T> { public: "
                    "T peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_value(); } "
                    "void peek(T& value) const override { value = peek_value(); } "
                    "virtual T peek_value() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_blocking_peek_if<").removesuffix(">")
            name = f"pysystemc_interfaces::PeekAdapter<{argument}>"
        if name.startswith("tlm::tlm_analysis_if<"):
            value_type = name.removeprefix("tlm::tlm_analysis_if<").removesuffix(">")
            methods = (
                f"void write(const {value_type}& value) override {{ write_value(value); }} "
                f"virtual void write_value({value_type}) = 0; "
            )
        if name.startswith("tlm::tlm_fw_transport_if<") or name.startswith("tlm::tlm_bw_transport_if<"):
            # Redeclare inherited pure methods so cppyy builds every Python dispatcher override.
            types = f"{name}::protocol_types"
            payload = f"typename {types}::tlm_payload_type"
            phase = f"typename {types}::tlm_phase_type"
            direction = "fw" if "tlm_fw_transport_if" in name else "bw"
            methods = (
                f"virtual tlm::tlm_sync_enum nb_transport_{direction}({payload}&, {phase}&, "
                "sc_core::sc_time&) override = 0; "
            )
            if direction == "fw":
                methods += (
                    f"virtual void b_transport({payload}&, sc_core::sc_time&) override = 0; "
                    f"virtual bool get_direct_mem_ptr({payload}&, tlm::tlm_dmi&) override = 0; "
                    f"unsigned int transport_dbg({payload}& trans) override {{ auto value = debug_transport(trans); "
                    "if (value < 0 || static_cast<unsigned long>(value) > std::numeric_limits<unsigned int>::max()) "
                    'throw std::out_of_range("transport_dbg result outside unsigned int range"); '
                    "return static_cast<unsigned int>(value); } "
                    f"virtual long debug_transport({payload}&) = 0; "
                )
            else:
                methods += "virtual void invalidate_direct_mem_ptr(sc_dt::uint64, sc_dt::uint64) override = 0; "
        cppyy.cppdef(
            "/** @brief Combined Python channel interface. "
            "@details Ports borrow the owner through simulation. cppyy acquires the GIL for overrides; "
            "blocking native calls must release it. Callback exceptions propagate to the caller. "
            "Debug transport validates unsigned range and must not wait. */ "
            f"namespace pysystemc_interfaces {{ class {identifier} : public sc_core::ScModule, public {name} {{ "
            f"public: explicit {identifier}(sc_core::sc_module_name name) : sc_core::ScModule(name) {{}} "
            f"~{identifier}() override = default; {methods} }}; }}"
        )
        base = getattr(cppyy.gbl.pysystemc_interfaces, identifier)
        _channel_interfaces[cache_name] = base
    return _channel_interfaces[cache_name]


def declare_interface(name: str, methods: dict[str, tuple[str, str]]) -> Any:
    """Declare a native interface for Python overrides and templated ports.

    Args:
        name: Unique C++ identifier for the interface.
        methods: Map method names to (C++ return type, C++ argument declarations).
            Reference arguments preserve native semantics; retain their storage.

    Returns:
        Native class usable as a Python base and sc_port template argument.

    Raises:
        ValueError: Names are invalid or a declaration conflicts with an earlier one.
        RuntimeError: Cling cannot compile the supplied native signatures.

    Signatures are trusted C++ declarations. Ports borrow implementations; retain
    Python owners through simulation. cppyy acquires the GIL for overrides.
    Blocking implementations must use guarded wait; do not suppress reset unwind.
    """
    if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name) or not methods:
        raise ValueError("interface requires a C++ identifier and at least one method")
    for method, signature in methods.items():
        if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", method) or len(signature) != 2:
            raise ValueError("method requires an identifier and (return type, arguments)")
    declaration = tuple(sorted(methods.items()))
    if name in _interface_declarations:
        previous, interface = _interface_declarations[name]
        if previous != declaration:
            raise ValueError(f"interface already declared with different methods: {name}")
        return interface
    signatures = "\n".join(
        f"virtual {result} {method}({arguments}) = 0;" for method, (result, arguments) in declaration
    )
    cppyy.cppdef(
        f"namespace pysystemc_interfaces {{ class {name} : public virtual sc_core::PythonInterface {{ "
        f"public: {name}() = default; ~{name}() override = default; {signatures} }}; }}"
    )
    interface = getattr(cppyy.gbl.pysystemc_interfaces, name)
    _interface_declarations[name] = (declaration, interface)
    return interface


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


def sc_elab_and_sim() -> int:
    """Enter native sc_main with sys.argv and return its integer exit status.

    This releases the GIL while native code runs and reacquires it for Python
    callbacks. Invoke once per independent simulation process.
    """
    log(f"sc_elab_and_sim({sys.argv})")

    argc = len(sys.argv)
    argv = sys.argv

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
        # try:
        #     return ScStatus(cpp_enum_value)
        # except ValueError:
        #     return ScStatus.SC_INVALID_STATUS  # Fallback if the value is invalid

    def __str__(self):
        """Return the string representation of the ScStatus enum."""
        return self.name


def sc_status() -> ScStatus:
    """Return the current kernel lifecycle state as ScStatus."""
    status = cppyy.gbl.sc_core.sc_get_status()
    return ScStatus(status)  # Convert to Python enum


@contextmanager
def vcd_trace(filename: str | pathlib.Path, *, time_unit: tuple[float, Any] | None = None) -> Iterator[Any]:
    """Open a VCD trace and close it on normal exit or a Python exception.

    Args:
        filename: Native trace filename stem (SystemC appends .vcd).
        time_unit: Optional (value, sc_time_unit) pair for trace timestamps.

    Yields:
        Native sc_trace_file accepted by sc_trace().

    Raises:
        RuntimeError: If the kernel cannot create the trace file.
        Exception: Exceptions from the body propagate after trace closure.

    Retain traced signals until the context exits. Closing a trace does not
    stop the kernel; independent simulations still require new subprocesses.
    Native process aborts cannot execute Python cleanup.
    """
    trace_path = pathlib.Path(str(filename) + ".vcd")
    if not trace_path.parent.is_dir():
        raise RuntimeError(f"Cannot create VCD trace: directory does not exist: {trace_path.parent}")
    trace = cppyy.gbl.sc_core.sc_create_vcd_trace_file(str(filename))
    if not trace:
        raise RuntimeError(f"Cannot create VCD trace: {filename}")
    try:
        if time_unit is not None:
            trace.set_time_unit(*time_unit)
        yield trace
    finally:
        cppyy.gbl.sc_core.sc_close_vcd_trace_file(trace)


_native_sc_trace = cppyy.gbl.sc_core.sc_trace


def sc_trace(file: Any, value: Any, name: str, *args: Any) -> Any:
    """Register a named waveform using stable native signal storage.

    Args:
        file: Open SystemC trace file.
        value: Native signal/interface or scalar accepted by SystemC.
        name: Name to record in the waveform.
        *args: Additional native scalar tracing arguments.

    Signal template types are instantiated on demand. Keep the traced object
    alive until the trace file closes; Python scalar temporaries are unsuitable
    for tracking mutable native state.
    """
    if hasattr(value, "read"):
        sample = value.read()
        value_type = bool if isinstance(sample, bool) else type(sample)
        return cppyy.gbl.sc_core.trace_signal[value_type](file, value, name)
    return _native_sc_trace(file, value, name, *args)


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
from .._namespace import (
    public_names as _public_names,
    document_member as _document_member,
    namespace_help as _namespace_help,
)

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
        "declare_interface",
        "channel_interface",
        "read_interface",
        "signal_channel",
        "fifo_channel",
    }
)

__doc__ = _namespace_help(globals(), cppyy.gbl.sc_core)
