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
        if name.startswith("tlm::tlm_fifo_debug_if<"):
            value = name.removeprefix("tlm::tlm_fifo_debug_if<").removesuffix(">")
            methods = (
                "virtual int used() const override = 0; virtual int size() const override = 0; "
                "virtual void debug() const override = 0; "
                f"bool nb_peek({value}& value, int n) const override {{ auto result = debug_peek(n); "
                "if (!result.first) return false; value = result.second; return true; } "
                f"bool nb_poke(const {value}& value, int n = 0) override {{ return debug_poke(value, n); }} "
                f"virtual std::pair<bool, {value}> debug_peek(int) const = 0; "
                f"virtual bool debug_poke({value}, int) = 0; "
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
                    f"void put(const {put}& value) override {{ put_value(value); }} "
                    f"{get} get(tlm::tlm_tag<{get}>* = nullptr) override {{ return get_value(); }} "
                    f"void get({get}& value) override {{ value = get_value(); }} "
                    f"{get} peek(tlm::tlm_tag<{get}>* = nullptr) const override {{ return peek_value(); }} "
                    f"void peek({get}& value) const override {{ value = peek_value(); }} "
                    f"virtual void put_value({put}) = 0; virtual {get} get_value() = 0; "
                    f"virtual {get} peek_value() const = 0; "
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
                    "T get(tlm::tlm_tag<T>* = nullptr) override { return get_value(); } "
                    "void get(T& value) override { value = get_value(); } "
                    "T peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_value(); } "
                    "void peek(T& value) const override { value = peek_value(); } "
                    "virtual T get_value() = 0; virtual T peek_value() const = 0; }; }"
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
                    "T get(tlm::tlm_tag<T>* = nullptr) override { return get_value(); } "
                    "void get(T& value) override { value = get_value(); } "
                    "T peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_value(); } "
                    "void peek(T& value) const override { value = peek_value(); } "
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
                    "virtual T get_value() = 0; virtual T peek_value() const = 0; "
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
                    "T peek(tlm::tlm_tag<T>* = nullptr) const override { return peek_value(); } "
                    "void peek(T& value) const override { value = peek_value(); } "
                    "bool nb_peek(T& value) const override { auto result = try_peek_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_can_peek(tlm::tlm_tag<T>* = nullptr) const override { return can_peek(); } "
                    "const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<T>* = nullptr) const override { return ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { ready.notify(delay); } "
                    "virtual T peek_value() const = 0; virtual std::pair<bool, T> try_peek_value() const = 0; "
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
                    "T get(tlm::tlm_tag<T>* = nullptr) override { return get_value(); } "
                    "void get(T& value) override { value = get_value(); } "
                    "bool nb_get(T& value) override { auto result = try_get_value(); "
                    "if (!result.first) return false; value = result.second; return true; } "
                    "bool nb_can_get(tlm::tlm_tag<T>* = nullptr) const override { return can_get(); } "
                    "const sc_core::sc_event& ok_to_get(tlm::tlm_tag<T>* = nullptr) const override { return ready; } "
                    "void notify_ready(const sc_core::sc_time& delay = sc_core::SC_ZERO_TIME) { ready.notify(delay); } "
                    "virtual T get_value() = 0; virtual std::pair<bool, T> try_get_value() = 0; "
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

        if name.startswith("tlm::tlm_transport_dbg_if<"):
            payload = name.removeprefix("tlm::tlm_transport_dbg_if<").removesuffix(">")
            methods = (
                f"unsigned int transport_dbg({payload}& trans) override {{ auto value = debug_transport(trans); "
                "if (value < 0 || static_cast<unsigned long>(value) > std::numeric_limits<unsigned int>::max()) "
                'throw std::out_of_range("transport_dbg result outside unsigned int range"); '
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
