"""Native-free regression checks for dynamic SystemC namespace documentation."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
from unittest.mock import patch


def _load_namespace_module():
    """Load the helper source directly without initializing the native package."""
    source = Path(__file__).resolve().parents[1] / "src/amal/eda/systemc/_namespace.py"
    spec = spec_from_file_location("_pysystemc_namespace_test", source)
    assert spec is not None and spec.loader is not None
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_namespace_help_prefers_present_module_values_and_falls_back_for_missing_names():
    """Use present falsy adapters directly and reflect only missing exports."""
    namespace_module = _load_namespace_module()
    lookups = []

    class NativeNamespace:
        """Small namespace proxy recording simulated native name lookups."""

        __cpp_name__ = "test_namespace"

        def __getattr__(self, name):
            """Provide one native fallback and reject adapter re-reflection."""
            lookups.append(name)
            if name == "native_fallback":
                return native_function
            raise AssertionError(f"unexpected native lookup: {name}")

    def native_function():
        """Stand in for a native namespace function."""

    values = {
        "false_adapter": False,
        "none_adapter": None,
        "zero_adapter": 0,
    }
    names = ["false_adapter", "native_fallback", "none_adapter", "zero_adapter"]
    with patch.object(namespace_module, "public_names", return_value=names):
        documentation = namespace_module.namespace_help(values, NativeNamespace())

    assert lookups == ["native_fallback"]
    assert "Functions: native_fallback" in documentation
    assert "Constants: false_adapter, none_adapter, zero_adapter" in documentation
