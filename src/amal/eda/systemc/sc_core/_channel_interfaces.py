"""Native interface declarations and Python-subclassable channel adapters.

Declarations are compiled lazily after the package initializes native bridges.
C++ templates preserve arbitrary type specialization; caches avoid redeclaration.
Ports borrow channel owners for simulation and callback lifetime.
"""

import re
from typing import Any

import cppyy

_interface_declarations = {}
_channel_interfaces = {}
_reference_callbacks = {}
_SCALAR_TYPES = frozenset(
    {
        "bool",
        "char",
        "signed char",
        "unsigned char",
        "short",
        "unsigned short",
        "int",
        "unsigned int",
        "long",
        "unsigned long",
        "long long",
        "unsigned long long",
        "float",
        "double",
        "long double",
    }
)


def channel_interface(interface: Any, *, _copy_put: bool = False) -> Any:
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
    cache_name = (name, _copy_put)

    if cache_name not in _channel_interfaces:
        identifier = f"ChannelInterface{len(_channel_interfaces)}"
        methods = ""
        fifo_family = name.split("<", 1)[0]
        # Native reference wrappers borrow storage retained by the structural owner cache.
        # Pointer overrides let cppyy preserve identity instead of translating references.
        if fifo_family == "sc_core::sc_mutex_if":
            methods = "virtual int lock() override = 0; virtual int trylock() override = 0; virtual int unlock() override = 0; "
        elif fifo_family == "sc_core::sc_semaphore_if":
            methods = "virtual int wait() override = 0; virtual int trywait() override = 0; virtual int post() override = 0; virtual int get_value() const override = 0; "
        elif fifo_family == "sc_core::sc_event_queue_if":
            methods = (
                "virtual void notify_time(sc_core::sc_time) = 0; "
                "void notify(double when, sc_core::sc_time_unit unit) override { notify_time(sc_core::sc_time(when, unit)); } "
                "void notify(const sc_core::sc_time& when) override { notify_time(when); } "
                "virtual void cancel_all() override = 0; "
                "virtual const sc_core::sc_event* event_pointer() const = 0; "
                "const sc_core::sc_event& default_event() const override { auto* value = event_pointer(); "
                'if (!value) throw std::runtime_error("event callback returned null"); return *value; } '
            )
            _reference_callbacks[name] = {"default_event": "event_pointer"}
        elif fifo_family == "sc_core::sc_signal_write_if":
            value = name.split("<", 1)[1].removesuffix(">")
            methods = f"virtual void write_value({value}) = 0; void write(const {value}& value) override {{ write_value(value); }} "
            methods += "virtual sc_core::sc_writer_policy writer_policy() const = 0; sc_core::sc_writer_policy get_writer_policy() const override { return writer_policy(); } "
        elif fifo_family in ("sc_core::sc_signal_in_if", "sc_core::sc_signal_inout_if"):
            value = name.split("<", 1)[1].removesuffix(">")
            references = {
                "read": (value, "const"),
                "get_data_ref": (value, "const"),
                "value_changed_event": ("sc_core::sc_event", "const"),
            }
            callbacks = {}
            methods = "virtual bool event() const override = 0; "
            for method, (result, qualifier) in references.items():
                callback = f"python_reference_{method}"
                callbacks[method] = callback
                methods += (
                    f"virtual const {result}* {callback}() {qualifier} = 0; "
                    f"const {result}& {method}() const override {{ auto* value = {callback}(); "
                    'if (!value) throw std::runtime_error("reference callback returned null"); return *value; } '
                )
            if value in _SCALAR_TYPES:
                # Fundamental Python values need adapter-owned native reference storage.
                methods = methods.replace(
                    f"virtual const {value}* python_reference_read() const = 0;",
                    f"using scalar_value = {value}; using scalar_pair = std::pair<scalar_value, bool>; mutable scalar_value sampled{{}}; virtual scalar_pair read_value() const = 0; "
                    f"const {value}* python_reference_read() const {{ sampled = read_value().first; return &sampled; }}",
                ).replace(
                    f"virtual const {value}* python_reference_get_data_ref() const = 0;",
                    "const " + value + "* python_reference_get_data_ref() const { return python_reference_read(); }",
                )
                callbacks = {"read": "read_value", "value_changed_event": "python_reference_value_changed_event"}
            if value in ("bool", "sc_dt::sc_logic"):
                for edge in ("posedge", "negedge"):
                    callbacks[f"{edge}_event"] = f"python_reference_{edge}_event"
                    methods += (
                        f"virtual bool {edge}() const override = 0; "
                        f"virtual const sc_core::sc_event* python_reference_{edge}_event() const = 0; "
                        f"const sc_core::sc_event& {edge}_event() const override {{ "
                        f"auto* value = python_reference_{edge}_event(); "
                        'if (!value) throw std::runtime_error("event callback returned null"); return *value; } '
                    )
            methods += "const sc_core::sc_event& default_event() const override { return value_changed_event(); } "
            if fifo_family.endswith("inout_if"):
                methods += f"virtual void write_value({value}) = 0; void write(const {value}& value) override {{ write_value(value); }} "
                methods += (
                    "static sc_core::sc_writer_policy default_writer_policy() { return SC_DEFAULT_WRITER_POLICY; } "
                    "virtual sc_core::sc_writer_policy writer_policy() const = 0; "
                    "sc_core::sc_writer_policy get_writer_policy() const override { return writer_policy(); } "
                )
            _reference_callbacks[name] = {k: v for k, v in callbacks.items() if v.startswith("python_reference_")}
        elif fifo_family in (
            "sc_core::sc_fifo_in_if",
            "sc_core::sc_fifo_out_if",
            "sc_core::sc_fifo_blocking_in_if",
            "sc_core::sc_fifo_blocking_out_if",
            "sc_core::sc_fifo_nonblocking_in_if",
            "sc_core::sc_fifo_nonblocking_out_if",
        ):
            value = name.split("<", 1)[1].removesuffix(">")
            incoming = fifo_family.endswith("in_if")
            combined = fifo_family in ("sc_core::sc_fifo_in_if", "sc_core::sc_fifo_out_if")
            blocking = combined or "nonblocking" not in fifo_family
            nonblocking = combined or "nonblocking" in fifo_family
            event = "data_written_event" if incoming else "data_read_event"
            if nonblocking:
                methods += (
                    f"virtual const sc_core::sc_event* python_reference_{event}() const = 0; "
                    f"const sc_core::sc_event& {event}() const override {{ auto* value = python_reference_{event}(); "
                    'if (!value) throw std::runtime_error("event callback returned null"); return *value; } '
                    f"const sc_core::sc_event& default_event() const override {{ return {event}(); }} "
                )
                _reference_callbacks[name] = {event: f"python_reference_{event}"}
                if incoming:
                    methods += (
                        f"using output_pair = std::pair<bool, {value}>; "
                        "virtual output_pair try_read_value() = 0; "
                        f"bool nb_read({value}& value) override {{ auto result = try_read_value(); "
                        "if (!result.first) return false; value = result.second; return true; } "
                    )
                else:
                    methods += (
                        f"virtual bool try_write_value({value}) = 0; "
                        f"bool nb_write(const {value}& value) override {{ return try_write_value(value); }} "
                    )
            if blocking:
                if incoming:
                    if value in _SCALAR_TYPES:
                        methods += (
                            f"using fifo_scalar_pair = std::pair<{value}, bool>; "
                            "virtual fifo_scalar_pair read_value() = 0; "
                            f"void read({value}& value) final {{ value = read_value().first; }} "
                            f"{value} read() final {{ return read_value().first; }} "
                        )
                    else:
                        methods += (
                            f"virtual {value} read_value() = 0; "
                            f"void read({value}& value) override {{ value = read_value(); }} "
                            f"{value} read() override {{ return read_value(); }} "
                        )
                else:
                    methods += (
                        f"virtual void write_value({value}) = 0; "
                        f"void write(const {value}& value) override {{ write_value(value); }} "
                    )
            if combined:
                methods += (
                    "virtual int num_available() const override = 0; "
                    if incoming
                    else "virtual int num_free() const override = 0; "
                )
        elif fifo_family in (
            "tlm::tlm_fifo_debug_if",
            "tlm::tlm_fifo_put_if",
            "tlm::tlm_fifo_get_if",
            "pysystemc_interfaces::FifoCombinedInterface",
        ):
            value = name.split("<", 1)[1].removesuffix(">")
            methods = (
                "virtual int used() const override = 0; virtual int size() const override = 0; "
                "virtual void debug() const override = 0; "
                f"bool nb_peek({value}& value, int n) const override {{ auto result = debug_peek(n); "
                "if (!result.first) return false; value = result.second; return true; } "
                f"bool nb_poke(const {value}& value, int n = 0) override {{ return debug_poke(value, n); }} "
                f"virtual std::pair<bool, {value}> debug_peek(int) const = 0; "
                f"virtual bool debug_poke({value}, int) = 0; "
            )
            if fifo_family in ("tlm::tlm_fifo_put_if", "pysystemc_interfaces::FifoCombinedInterface"):
                methods += (
                    "private: sc_core::sc_event put_ready; public: "
                    f"void put(const {value}& value) override {{ put_value(value); }} "
                    f"bool nb_put(const {value}& value) override {{ return try_put_value(value); }} "
                    f"bool nb_can_put(tlm::tlm_tag<{value}>* = nullptr) const override {{ return can_put(); }} "
                    f"const sc_core::sc_event& ok_to_put(tlm::tlm_tag<{value}>* = nullptr) const override "
                    "{ return put_ready; } "
                    + (
                        ""
                        if fifo_family == "pysystemc_interfaces::FifoCombinedInterface"
                        else "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { put_ready.notify(delay); } "
                    )
                    + f"virtual void put_value({value}) = 0; virtual bool try_put_value({value}) = 0; "
                    "virtual bool can_put() const = 0; "
                )
            if fifo_family in ("tlm::tlm_fifo_get_if", "pysystemc_interfaces::FifoCombinedInterface"):
                methods += (
                    f"using blocking_value_pair = std::pair<{value}, bool>; "
                    "private: sc_core::sc_event get_ready, peek_ready; public: "
                    f"{value} get(tlm::tlm_tag<{value}>* = nullptr) override {{ return get_value().first; }} "
                    f"void get({value}& value) override {{ value = get_value().first; }} "
                    f"{value} peek(tlm::tlm_tag<{value}>* = nullptr) const override {{ return peek_value().first; }} "
                    f"void peek({value}& value) const override {{ value = peek_value().first; }} "
                    f"bool nb_get({value}& value) override {{ auto result = try_get_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    f"bool nb_peek({value}& value) const override {{ auto result = try_peek_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    f"bool nb_can_get(tlm::tlm_tag<{value}>* = nullptr) const override {{ return can_get(); }} "
                    f"bool nb_can_peek(tlm::tlm_tag<{value}>* = nullptr) const override {{ return can_peek(); }} "
                    f"const sc_core::sc_event& ok_to_get(tlm::tlm_tag<{value}>* = nullptr) const override "
                    "{ return get_ready; } "
                    f"const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<{value}>* = nullptr) const override "
                    "{ return peek_ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) "
                    "{ get_ready.notify(delay); peek_ready.notify(delay); "
                    + (
                        "put_ready.notify(delay); "
                        if fifo_family == "pysystemc_interfaces::FifoCombinedInterface"
                        else ""
                    )
                    + "} "
                    "virtual blocking_value_pair get_value() = 0; "
                    "virtual blocking_value_pair peek_value() const = 0; "
                    f"virtual std::pair<bool, {value}> try_get_value() = 0; "
                    f"virtual std::pair<bool, {value}> try_peek_value() const = 0; "
                    "virtual bool can_get() const = 0; virtual bool can_peek() const = 0; "
                )
        if name == "tlm::tlm_fifo_config_size_if":
            methods = (
                "void nb_expand(unsigned int n = 1) override { expand_value(n); } "
                "void nb_unbound(unsigned int n = 16) override { unbound_value(n); } "
                "bool nb_reduce(unsigned int n = 1) override { return reduce_value(n); } "
                "bool nb_bound(unsigned int n) override { return bound_value(n); } "
                "virtual void expand_value(unsigned int) = 0; virtual void unbound_value(unsigned int) = 0; "
                "virtual bool reduce_value(unsigned int) = 0; virtual bool bound_value(unsigned int) = 0; "
            )
        family = name.split("<", 1)[0]
        compound = {
            f"tlm::tlm_{mode}{direction}_if": (mode, direction)
            for mode in ("", "blocking_", "nonblocking_")
            for direction in ("master", "slave")
        }
        if family in compound:
            mode, direction = compound[family]
            # Extract native template arguments in C++, preserving nested types.
            traits = f"{identifier}Types"
            put_type, get_type = ("Req", "Rsp") if direction == "master" else ("Rsp", "Req")
            cppyy.cppdef(
                "/** @brief Native request/response type extraction for compound adapters. "
                "@details Type-only traits own no objects and require no GIL. */ "
                f"namespace pysystemc_interfaces {{ template<class T> struct {traits}; "
                f"template<class Req, class Rsp> struct {traits}<{family}<Req, Rsp>> {{ "
                f"using Put = {put_type}; using Get = {get_type}; }}; }}"
            )
            put = f"typename {traits}<{name}>::Put"
            get = f"typename {traits}<{name}>::Get"
            if mode != "nonblocking_":
                methods += (
                    "/** @brief Copy blocking response values through a native pair. "
                    "@details The adapter borrows its Python owner through simulation; callbacks acquire "
                    "the GIL and blocking native callers release it. Exceptions and scheduler unwind "
                    "propagate before caller output assignment. The pair owns its response copy. */ "
                    f"using blocking_value_pair = std::pair<{get}, bool>; "
                    f"void put(const {put}& value) override {{ put_value(value); }} "
                    f"{get} get(tlm::tlm_tag<{get}>* = nullptr) override {{ return get_value().first; }} "
                    f"void get({get}& value) override {{ value = get_value().first; }} "
                    f"{get} peek(tlm::tlm_tag<{get}>* = nullptr) const override {{ return peek_value().first; }} "
                    f"void peek({get}& value) const override {{ value = peek_value().first; }} "
                    f"virtual void put_value({put}) = 0; virtual blocking_value_pair get_value() = 0; "
                    "virtual blocking_value_pair peek_value() const = 0; "
                )
            if mode != "blocking_":
                methods += (
                    "private: sc_core::sc_event put_ready, get_ready, peek_ready; public: "
                    f"bool nb_put(const {put}& value) override {{ return try_put_value(value); }} "
                    f"bool nb_get({get}& value) override {{ auto result = try_get_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    f"bool nb_peek({get}& value) const override {{ auto result = try_peek_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    f"bool nb_can_put(tlm::tlm_tag<{put}>* = nullptr) const override {{ return can_put(); }} "
                    f"bool nb_can_get(tlm::tlm_tag<{get}>* = nullptr) const override {{ return can_get(); }} "
                    f"bool nb_can_peek(tlm::tlm_tag<{get}>* = nullptr) const override {{ return can_peek(); }} "
                    f"const sc_core::sc_event& ok_to_put(tlm::tlm_tag<{put}>* = nullptr) const override "
                    "{ return put_ready; } "
                    f"const sc_core::sc_event& ok_to_get(tlm::tlm_tag<{get}>* = nullptr) const override "
                    "{ return get_ready; } "
                    f"const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<{get}>* = nullptr) const override "
                    "{ return peek_ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) "
                    "{ put_ready.notify(delay); get_ready.notify(delay); peek_ready.notify(delay); } "
                    f"virtual bool try_put_value({put}) = 0; "
                    f"virtual std::pair<bool, {get}> try_get_value() = 0; "
                    f"virtual std::pair<bool, {get}> try_peek_value() const = 0; "
                    "virtual bool can_put() const = 0; virtual bool can_get() const = 0; "
                    "virtual bool can_peek() const = 0; "
                )
            methods += f"using output_pair = std::pair<bool, {get}>; "
        if name.startswith("tlm::tlm_blocking_get_peek_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "BlockingGetPeekAdapter"):
                cppyy.cppdef(
                    "/** @brief Blocking get-peek value bridge. @details Ports borrow the owner; "
                    "cppyy acquires GIL for callbacks and blocking callers release it. Outputs receive "
                    "copies; Python owns sample state. Exceptions propagate through guarded waits. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class BlockingGetPeekAdapter : public tlm::tlm_blocking_get_peek_if<T> { public: "
                    "using blocking_value_pair = std::pair<T, bool>; "
                    "T get(tlm::tlm_tag<T>* = nullptr) override { return get_value().first; } "
                    "void get(T& value) override { value = get_value().first; } "
                    "T peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_value().first; } "
                    "void peek(T& value) const override { value = peek_value().first; } "
                    "virtual blocking_value_pair get_value() = 0; virtual blocking_value_pair peek_value() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_blocking_get_peek_if<").removesuffix(">")
            name = f"pysystemc_interfaces::BlockingGetPeekAdapter<{argument}>"

        if name.startswith("tlm::tlm_get_peek_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "CombinedGetPeekAdapter"):
                cppyy.cppdef(
                    "/** @brief Complete blocking/nonblocking get-peek bridge. @details Ports borrow "
                    "the owner; adapter owns distinct readiness events. cppyy acquires GIL for callbacks; "
                    "blocking callers release it. Nonblocking callbacks must not wait. Errors propagate. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class CombinedGetPeekAdapter : public tlm::tlm_get_peek_if<T> { "
                    "sc_core::sc_event get_ready, peek_ready; public: "
                    "using blocking_value_pair = std::pair<T, bool>; "
                    "T get(tlm::tlm_tag<T>* = nullptr) override { return get_value().first; } "
                    "void get(T& value) override { value = get_value().first; } "
                    "T peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_value().first; } "
                    "void peek(T& value) const override { value = peek_value().first; } "
                    "bool nb_get(T& value) override { auto result = try_get_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_peek(T& value) const override { auto result = try_peek_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_can_get(tlm::tlm_tag<T>* = nullptr) const override { return can_get(); } "
                    "bool nb_can_peek(tlm::tlm_tag<T>* = nullptr) const override { return can_peek(); } "
                    "const sc_core::sc_event& ok_to_get(tlm::tlm_tag<T>* = nullptr) const override { return get_ready; } "
                    "const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { "
                    "get_ready.notify(delay); peek_ready.notify(delay); } "
                    "virtual blocking_value_pair get_value() = 0; virtual blocking_value_pair peek_value() const = 0; "
                    "virtual std::pair<bool, T> try_get_value() = 0; "
                    "virtual std::pair<bool, T> try_peek_value() const = 0; "
                    "virtual bool can_get() const = 0; virtual bool can_peek() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_get_peek_if<").removesuffix(">")
            name = f"pysystemc_interfaces::CombinedGetPeekAdapter<{argument}>"

        if name.startswith("tlm::tlm_peek_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "CombinedPeekAdapter"):
                cppyy.cppdef(
                    "/** @brief Blocking/nonblocking peek bridge. @details Ports borrow the owner; "
                    "adapter owns readiness event. cppyy acquires GIL for callbacks; blocking callers "
                    "release it. Nonblocking callbacks must not wait. Errors propagate; outputs are copies. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class CombinedPeekAdapter : public tlm::tlm_peek_if<T> { sc_core::sc_event ready; public: "
                    "using blocking_value_pair = std::pair<T, bool>; "
                    "T peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_value().first; } "
                    "void peek(T& value) const override { value = peek_value().first; } "
                    "bool nb_peek(T& value) const override { auto result = try_peek_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_can_peek(tlm::tlm_tag<T>* = nullptr) const override { return can_peek(); } "
                    "const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<T>* = nullptr) const override { return ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { ready.notify(delay); } "
                    "virtual blocking_value_pair peek_value() const = 0; virtual std::pair<bool, T> try_peek_value() const = 0; "
                    "virtual bool can_peek() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_peek_if<").removesuffix(">")
            name = f"pysystemc_interfaces::CombinedPeekAdapter<{argument}>"

        if name.startswith("tlm::tlm_put_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "CombinedPutAdapter"):
                cppyy.cppdef(
                    "/** @brief Blocking/nonblocking put value-copy bridge. @details Ports borrow "
                    "the owner; adapter owns readiness event. cppyy acquires GIL for callbacks; "
                    "blocking callers release it. Nonblocking callbacks must not wait. Errors propagate. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class CombinedPutAdapter : public tlm::tlm_put_if<T> { sc_core::sc_event ready; public: "
                    "void put(const T& value) override { put_value(value); } "
                    "bool nb_put(const T& value) override { return try_put_value(value); } "
                    "bool nb_can_put(tlm::tlm_tag<T>* = nullptr) const override { return can_put(); } "
                    "const sc_core::sc_event& ok_to_put(tlm::tlm_tag<T>* = nullptr) const override { return ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { ready.notify(delay); } "
                    "virtual void put_value(T) = 0; virtual bool try_put_value(T) = 0; "
                    "virtual bool can_put() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_put_if<").removesuffix(">")
            name = f"pysystemc_interfaces::CombinedPutAdapter<{argument}>"

        if name.startswith("tlm::tlm_get_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "CombinedGetAdapter"):
                cppyy.cppdef(
                    "/** @brief Blocking/nonblocking get bridge. @details Ports borrow the owner; "
                    "the adapter owns readiness storage. cppyy acquires GIL for callbacks; blocking "
                    "callers release it. Nonblocking callbacks must not wait. Exceptions propagate. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class CombinedGetAdapter : public tlm::tlm_get_if<T> { sc_core::sc_event ready; public: "
                    "using blocking_value_pair = std::pair<T, bool>; "
                    "T get(tlm::tlm_tag<T>* = nullptr) override { return get_value().first; } "
                    "void get(T& value) override { value = get_value().first; } "
                    "bool nb_get(T& value) override { auto result = try_get_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_can_get(tlm::tlm_tag<T>* = nullptr) const override { return can_get(); } "
                    "const sc_core::sc_event& ok_to_get(tlm::tlm_tag<T>* = nullptr) const override { return ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { ready.notify(delay); } "
                    "virtual blocking_value_pair get_value() = 0; virtual std::pair<bool, T> try_get_value() = 0; "
                    "virtual bool can_get() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_get_if<").removesuffix(">")
            name = f"pysystemc_interfaces::CombinedGetAdapter<{argument}>"

        if _copy_put and name.startswith("tlm::tlm_blocking_put_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "PutAdapter"):
                cppyy.cppdef(
                    "/** @brief Value-copy blocking put bridge. @details Ports borrow the owner; "
                    "cppyy acquires GIL for callbacks. Blocking callers release it; exceptions propagate. "
                    "The callback receives a copy, never borrowed reference storage. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class PutAdapter : public tlm::tlm_blocking_put_if<T> { public: "
                    "void put(const T& value) override { put_value(value); } "
                    "virtual void put_value(T) = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_blocking_put_if<").removesuffix(">")
            name = f"pysystemc_interfaces::PutAdapter<{argument}>"

        if name.startswith("tlm::tlm_nonblocking_get_peek_if<"):
            if not hasattr(
                getattr(cppyy.gbl, "pysystemc_interfaces", None),
                "NonblockingGetPeekAdapter",
            ):
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
                    "using transport_value_pair = std::pair<Rsp, bool>; "
                    "Rsp transport(const Req& req) override { return transport_value(req).first; } "
                    "void transport(const Req& req, Rsp& rsp) override { rsp = transport_value(req).first; } "
                    "virtual transport_value_pair transport_value(Req) = 0; }; }"
                )
            arguments = name.removeprefix("tlm::tlm_transport_if<").removesuffix(">")
            name = f"pysystemc_interfaces::TransportAdapter<{arguments}>"
            methods += f"using transport_value_pair = typename {name}::transport_value_pair; "

        if name.startswith("tlm::tlm_blocking_get_if<"):
            if not hasattr(getattr(cppyy.gbl, "pysystemc_interfaces", None), "GetAdapter"):
                cppyy.cppdef(
                    "/** @brief Value-copy TLM get bridge. @details Ports borrow the owner; "
                    "cppyy acquires GIL for callbacks. Blocking callers release GIL. Errors propagate. */ "
                    "namespace pysystemc_interfaces { template<class T> "
                    "class GetAdapter : public tlm::tlm_blocking_get_if<T> { public: "
                    "using blocking_value_pair = std::pair<T, bool>; "
                    "T get(tlm::tlm_tag<T>* = nullptr) override { return get_value().first; } "
                    "void get(T& value) override { value = get_value().first; } "
                    "virtual blocking_value_pair get_value() = 0; }; }"
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
                    "using blocking_value_pair = std::pair<T, bool>; "
                    "T peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_value().first; } "
                    "void peek(T& value) const override { value = peek_value().first; } "
                    "virtual blocking_value_pair peek_value() const = 0; }; }"
                )
            argument = name.removeprefix("tlm::tlm_blocking_peek_if<").removesuffix(">")
            name = f"pysystemc_interfaces::PeekAdapter<{argument}>"

        if fifo_family in (
            "tlm::tlm_analysis_if",
            "tlm::tlm_write_if",
            "tlm::tlm_delayed_write_if",
            "tlm::tlm_delayed_analysis_if",
        ):
            value_type = name.split("<", 1)[1].removesuffix(">")
            delayed = "delayed" in fifo_family
            argument = ", const sc_core::sc_time& delay" if delayed else ""
            call = ", delay" if delayed else ""
            methods = (
                f"void write(const {value_type}& value{argument}) override {{ write_value(value{call}); }} "
                f"virtual void write_value({value_type}{argument}) = 0; "
            )

        if name.startswith("tlm::tlm_transport_dbg_if<"):
            payload = name.removeprefix("tlm::tlm_transport_dbg_if<").removesuffix(">")
            methods = (
                f"unsigned int transport_dbg({payload}& trans) override {{ auto value = debug_transport(trans); "
                "if (value < 0 || static_cast<unsigned long>(value) > std::numeric_limits<unsigned int>::max()) "
                'throw std::out_of_range("transport_dbg result outside unsigned int range"); '
                "if (static_cast<unsigned long>(value) > trans.get_data_length()) "
                'throw std::out_of_range("transport_dbg result exceeds payload data length"); '
                "return static_cast<unsigned int>(value); } "
                f"virtual long debug_transport({payload}&) = 0; "
            )

        if name.startswith(("tlm::tlm_fw_transport_if<", "tlm::tlm_bw_transport_if<")):
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
                    "if (static_cast<unsigned long>(value) > trans.get_data_length()) "
                    'throw std::out_of_range("transport_dbg result exceeds payload data length"); '
                    "return static_cast<unsigned int>(value); } "
                    f"virtual long debug_transport({payload}&) = 0; "
                )
            else:
                methods += "virtual void invalidate_direct_mem_ptr(sc_dt::uint64, sc_dt::uint64) override = 0; "
        if fifo_family in (
            "tlm::tlm_get_if",
            "tlm::tlm_peek_if",
            "tlm::tlm_blocking_get_if",
            "tlm::tlm_blocking_peek_if",
            "tlm::tlm_blocking_get_peek_if",
            "tlm::tlm_get_peek_if",
        ):
            value = interface.__cpp_name__.split("<", 1)[1].removesuffix(">")
            methods += f"using blocking_value_pair = std::pair<{value}, bool>; "
        cppyy.cppdef(
            "/** @brief Combined Python channel interface. "
            "@details Ports borrow the owner through simulation. cppyy acquires the GIL for overrides; "
            "blocking native calls must release it. Callback exceptions propagate to the caller. "
            "Debug transport validates unsigned range and must not wait. Reference pointer callbacks "
            "borrow storage retained through shutdown; null pointers throw and events preserve owner identity. */ "
            f"namespace pysystemc_interfaces {{ class {identifier} : public sc_core::ScModule, public {name} {{ "
            f"public: explicit {identifier}(sc_core::sc_module_name name) : sc_core::ScModule(name) {{}} "
            f"~{identifier}() override = default; {methods} }}; }}"
        )
        base = getattr(cppyy.gbl.pysystemc_interfaces, identifier)
        for method in ("transport", "get", "peek", "put", "b_transport", "read", "write", "lock", "wait"):
            overload = getattr(base, method, None)
            if overload is not None and hasattr(overload, "__release_gil__"):
                overload.__release_gil__ = True
        _channel_interfaces[cache_name] = base
    return _channel_interfaces[cache_name]


def declare_interface(name: str, methods: dict[str, tuple[str, str]]) -> Any:
    """Declare a native interface for Python overrides and templated ports.

    Args:
        name: Unique C++ identifier for the interface.
        methods: Map method names to (C++ return type, C++ argument declarations).
            Reference arguments preserve native semantics; retain their storage.
            Lvalue reference results use a pointer trampoline; structural callbacks
            must return native objects, which the adapter retains through shutdown.

    Returns:
        Native class usable as a Python base and sc_port template argument.

    Raises:
        ValueError: Names are invalid or a declaration conflicts with an earlier one.
        RuntimeError: Cling cannot compile the supplied native signatures.

    Signatures are trusted C++ declarations. Ports borrow implementations; retain
    Python owners through simulation. cppyy acquires the GIL for overrides.
    Native method entry releases the GIL, including arbitrarily named methods.
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
    signatures = []
    reference_callbacks = {}
    for method, (result, arguments) in declaration:
        if result.rstrip().endswith("&") and not result.rstrip().endswith("&&"):
            pointee = result.rstrip()[:-1].strip()
            callback = f"python_reference_{method}"
            if "<" in arguments or "=" in arguments:
                raise ValueError("reference-return arguments with templates or defaults require a native adapter")
            parameters = []
            for argument in arguments.split(",") if arguments.strip() else []:
                match = re.search(r"([A-Za-z_][A-Za-z_0-9]*)\s*$", argument)
                if match is None:
                    raise ValueError("reference-return methods require named arguments without defaults")
                parameters.append(match[1])
            signatures.append(
                "/** @brief Borrow retained Python callback storage as a native reference. "
                "@details cppyy acquires the GIL for the pointer callback; the structural adapter "
                "retains returned storage through shutdown. Null results throw; callback exceptions "
                "propagate. Blocking callbacks require guarded waits and a GIL-releasing caller. */ "
                f"virtual {pointee}* {callback}({arguments}) = 0; "
                f"virtual {result} {method}({arguments}) {{ auto* value = {callback}({', '.join(parameters)}); "
                'if (!value) throw std::runtime_error("reference callback returned null"); return *value; }'
            )
            reference_callbacks[method] = callback
        else:
            signatures.append(f"virtual {result} {method}({arguments}) = 0;")
    signatures = "\n".join(signatures)
    cppyy.cppdef(
        f"namespace pysystemc_interfaces {{ class {name} : public virtual sc_core::PythonInterface {{ "
        f"public: {name}() = default; ~{name}() override = default; {signatures} }}; }}"
    )
    interface = getattr(cppyy.gbl.pysystemc_interfaces, name)
    # Declared names are arbitrary; any override may schedule a guarded wait.
    # Native entry releases GIL, while cppyy reacquires it for Python overrides.
    for method in methods:
        getattr(interface, method).__release_gil__ = True
    _reference_callbacks[interface.__cpp_name__] = reference_callbacks
    _interface_declarations[name] = (declaration, interface)
    return interface
