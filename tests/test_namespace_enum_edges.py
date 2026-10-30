"""Check namespace enum documentation precedence without importing cppyy."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
from unittest.mock import patch

import pytest


def _load_namespace_module():
    """Load the namespace helper directly, keeping native initialization out."""
    source = Path(__file__).resolve().parents[1] / "src/amal/eda/systemc/_namespace.py"
    spec = spec_from_file_location("_pysystemc_namespace_enum_test", source)
    assert spec is not None and spec.loader is not None
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_enum_details_use_present_module_adapter_without_native_relookup():
    """Render the adapter's members and avoid a second native enum lookup."""
    namespace_module = _load_namespace_module()
    enum_adapter = type(
        "PublicEnum",
        (),
        {
            "__underlying": int,
            "ADAPTER_VALUE": 7,
            "__cpp_name__": "test_namespace::PublicEnum",
            "__doc__": "Public enum adapter documentation.",
        },
    )
    lookups = []

    class NativeNamespace:
        """Reject native reflection for the name supplied by module globals."""

        __cpp_name__ = "test_namespace"

        def __getattr__(self, name):
            """Record and reject every attempted lookup."""
            lookups.append(name)
            raise AssertionError(f"unexpected native lookup: {name}")

    values = {"PublicEnum": enum_adapter, "__doc__": "Adapter namespace."}
    with patch.object(namespace_module, "public_names", return_value=["PublicEnum"]):
        documentation = namespace_module.namespace_help(values, NativeNamespace())

    assert lookups == []
    assert enum_adapter.__doc__ == "Public enum adapter documentation."
    assert namespace_module.document_member(enum_adapter) is enum_adapter
    assert "Enums: PublicEnum" in documentation
    assert "PublicEnum: ADAPTER_VALUE=7" in documentation


def test_missing_enum_adapter_uses_native_enum_detail_fallback():
    """Render raw enum details when module globals omit its name."""
    namespace_module = _load_namespace_module()
    raw_enum = type(
        "RawEnum",
        (),
        {
            "__underlying": int,
            "RAW_VALUE": 1,
            "__cpp_name__": "test_namespace::RawEnum",
            "__doc__": "Raw enum metadata.",
        },
    )
    lookups = []

    class NativeNamespace:
        """Provide the raw enum for both declaration and details lookup."""

        __cpp_name__ = "test_namespace"

        def __getattr__(self, name):
            """Return the enum only for its requested public name."""
            lookups.append(name)
            if name == "RawEnum":
                return raw_enum
            raise AttributeError(name)

    with patch.object(namespace_module, "public_names", return_value=["RawEnum"]):
        documentation = namespace_module.namespace_help({}, NativeNamespace())

    assert lookups == ["RawEnum", "RawEnum"]
    assert "Enums: RawEnum" in documentation
    assert "RawEnum: RAW_VALUE=1" in documentation


@pytest.mark.parametrize("failure", [AttributeError, TypeError])
def test_document_member_preserves_classes_when_doc_assignment_fails(failure):
    """Return the same native-like class if its documentation is read-only."""
    namespace_module = _load_namespace_module()

    class RejectingMeta(type):
        """Reject the attempted documentation assignment with one exception."""

        def __setattr__(cls, name, value):
            """Raise the selected metadata error for the docstring only."""
            if name == "__doc__":
                raise failure("read-only documentation")
            super().__setattr__(name, value)

    native_class = RejectingMeta(
        "NativeClass",
        (),
        {"__cpp_name__": "test_namespace::NativeClass", "__doc__": None},
    )

    assert namespace_module.document_member(native_class) is native_class
