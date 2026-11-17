"""Check single-call enum classification while preserving native proxy identity."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
from unittest.mock import patch


def test_namespace_help_classifies_native_type_once():
    """Reuse enum detection for documentation and grouping without stale state."""
    source = Path(__file__).resolve().parents[1] / "src/amal/eda/systemc/_namespace.py"
    spec = spec_from_file_location("_namespace_enum_classification_test", source)
    helper = module_from_spec(spec)
    spec.loader.exec_module(helper)
    lookups = []

    class NativeMeta(type):
        """Record native-like metadata resolution for one type proxy."""

        def __getattr__(cls, name):
            """Reject missing metadata while retaining lookup evidence."""
            lookups.append(name)
            raise AttributeError(name)

    class NativeClass(metaclass=NativeMeta):
        """Existing native documentation must remain unchanged."""

    with patch.object(helper, "public_names", return_value=["NativeClass"]):
        doc = helper.namespace_help({"NativeClass": NativeClass, "__doc__": "Namespace introduction."}, object())
        assert "Classes: NativeClass" in doc
        assert lookups == ["__underlying"]
        lookups.clear()
        assert (
            helper.namespace_help({"NativeClass": NativeClass, "__doc__": "Namespace introduction."}, object()) == doc
        )
        assert lookups == ["__underlying"]
    assert NativeClass.__doc__ == "Existing native documentation must remain unchanged."
