"""Exercise the pure Python native-method descriptor without loading SystemC."""

from __future__ import annotations

import gc
import importlib.util
import weakref
from pathlib import Path
from typing import Any


def _load_native_method_class() -> type[Any]:
    """Load ``NativeMethod`` directly from its file without importing its package."""
    source = Path(__file__).parents[1] / "src" / "amal" / "eda" / "systemc" / "sc_dt" / "_native_methods.py"
    spec = importlib.util.spec_from_file_location(
        "pysystemc_native_methods_descriptor_test_source",
        source,
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.NativeMethod


NativeMethod = _load_native_method_class()


class _OverloadMethod:
    """Represent an original native callable with exact-overload selection."""

    def __init__(self) -> None:
        """Initialize call and overload records for assertions."""
        self.calls: list[tuple[object, tuple[Any, ...], dict[str, Any]]] = []
        self.selections: list[str] = []

    def __call__(self, owner: object, *args: Any, **kwargs: Any) -> tuple[object, tuple[Any, ...], dict[str, Any]]:
        """Record a regular original-method invocation."""
        record = (owner, args, kwargs)
        self.calls.append(record)
        return record

    def __overload__(self, signature: str) -> Any:
        """Return an exact-signature function that records its invocation."""
        self.selections.append(signature)

        def invoke(owner: object, *args: Any, **kwargs: Any) -> tuple[str, object, tuple[Any, ...], dict[str, Any]]:
            """Record one call through a selected overload."""
            return signature, owner, args, kwargs

        return invoke


def test_class_access_and_unbound_calls_forward_owner_and_arguments() -> None:
    """Keep descriptor access and direct calls usable with an explicit owner."""
    calls: list[tuple[object, tuple[Any, ...], dict[str, Any]]] = []

    def wrapper(owner: object, *args: Any, **kwargs: Any) -> tuple[object, tuple[Any, ...], dict[str, Any]]:
        """Record adapter arguments for this test."""
        record = (owner, args, kwargs)
        calls.append(record)
        return record

    original = _OverloadMethod()
    descriptor = NativeMethod(wrapper, original)

    class Model:
        """Provide class access to the descriptor under test."""

        method = descriptor

    owner = Model()
    assert Model.method is descriptor
    assert Model.method(owner, 4, named="value") == (owner, (4,), {"named": "value"})
    assert Model.method.__overload__("int")(owner, 7) == ("int", owner, (7,), {})
    assert calls == [(owner, (4,), {"named": "value"})]
    assert original.selections == ["int"]


def test_bound_callable_preserves_doc_and_forwards_exact_overload_arguments() -> None:
    """Bind owner once for regular and exact-overload calls and preserve docs."""

    def wrapper(owner: object, *args: Any, **kwargs: Any) -> tuple[object, tuple[Any, ...], dict[str, Any]]:
        """Adapter documentation visible through the descriptor."""
        return owner, args, kwargs

    original = _OverloadMethod()
    descriptor = NativeMethod(wrapper, original)

    class Model:
        """Provide instance access to the descriptor under test."""

        method = descriptor

    owner = Model()
    bound = owner.method
    assert bound.__doc__ == wrapper.__doc__
    assert bound(3, named="regular") == (owner, (3,), {"named": "regular"})
    assert bound.__overload__("unsigned int")(5, named="exact") == (
        "unsigned int",
        owner,
        (5,),
        {"named": "exact"},
    )
    assert original.selections == ["unsigned int"]


def test_original_without_overloads_returns_bound_adapter_only() -> None:
    """Allow ordinary original callables and omit a bound overload selector."""

    def original(owner: object, value: int) -> tuple[object, int]:
        """Stand in for a native method without overload support."""
        return owner, value

    def wrapper(owner: object, value: int) -> tuple[object, int]:
        """Forward one value through the adapter."""
        return owner, value

    descriptor = NativeMethod(wrapper, original)

    class Model:
        """Provide instance access to the descriptor under test."""

        method = descriptor

    owner = Model()
    bound = owner.method
    assert bound(11) == (owner, 11)
    assert not hasattr(bound, "__overload__")


def test_bound_callable_retains_owner_until_bound_callable_is_deleted() -> None:
    """Match bound-method lifetime by retaining and then releasing the owner."""

    def wrapper(owner: object) -> object:
        """Return the owner passed through the adapter."""
        return owner

    descriptor = NativeMethod(wrapper, None)

    class Model:
        """Provide instance access to the descriptor under test."""

        method = descriptor

    owner = Model()
    owner_ref = weakref.ref(owner)
    bound = owner.method
    del owner
    gc.collect()
    assert owner_ref() is not None
    assert bound() is owner_ref()

    del bound
    gc.collect()
    assert owner_ref() is None


def test_adapter_exception_propagates_unchanged() -> None:
    """Propagate the adapter's original exception object without wrapping."""
    failure = RuntimeError("adapter failed")

    def wrapper(owner: object) -> None:
        """Raise the sentinel exception from the adapter."""
        raise failure

    descriptor = NativeMethod(wrapper, None)

    class Model:
        """Provide instance access to the descriptor under test."""

        method = descriptor

    try:
        Model().method()
    except RuntimeError as exc:
        assert exc is failure
    else:
        raise AssertionError("adapter exception was swallowed")


def test_original_overload_exception_propagates_unchanged() -> None:
    """Propagate exceptions raised while invoking an exact original overload."""
    failure = LookupError("native overload failed")

    class FailingOverload:
        """Provide an original callable whose selected overload fails."""

        def __overload__(self, signature: str) -> Any:
            """Return a selected overload that raises the sentinel exception."""
            assert signature == "int"

            def invoke(owner: object) -> None:
                """Raise the sentinel from the selected overload invocation."""
                raise failure

            return invoke

    def wrapper(owner: object) -> object:
        """Return the bound owner for regular calls."""
        return owner

    descriptor = NativeMethod(wrapper, FailingOverload())

    class Model:
        """Provide instance access to the descriptor under test."""

        method = descriptor

    try:
        Model().method.__overload__("int")()
    except LookupError as exc:
        assert exc is failure
    else:
        raise AssertionError("original overload exception was swallowed")
