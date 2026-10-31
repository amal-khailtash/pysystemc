"""Native-free checks for cppyy metadata formatting and exact bridge filtering."""

from __future__ import annotations

import importlib.util
import io
import sys
import types
from pathlib import Path


def _load_formatter():
    """Load the formatter and real namespace policy without importing SystemC."""
    root = Path(__file__).parents[1] / "src" / "amal" / "eda" / "systemc"
    package_name = "_native_help_test_package"
    package = types.ModuleType(package_name)
    package.__path__ = [str(root)]
    sys.modules[package_name] = package

    for module_name in ("_namespace", "_native_help"):
        qualified_name = f"{package_name}.{module_name}"
        spec = importlib.util.spec_from_file_location(qualified_name, root / f"{module_name}.py")
        assert spec is not None and spec.loader is not None
        module = importlib.util.module_from_spec(spec)
        sys.modules[qualified_name] = module
        spec.loader.exec_module(module)
    return sys.modules[f"{package_name}._native_help"]


_formatter = _load_formatter()


class _NativeClass:
    """Simple proxy-shaped class for native class and constructor metadata."""

    __cpp_name__ = "sc_core::sc_time"
    __doc__ = "Native time value."

    def __init__(self):
        """sc_core::sc_time::sc_time(double, sc_core::sc_time_unit)"""


class _NativeOverload:
    """Simple cppyy-overload-shaped object with native candidate signatures."""

    __name__ = "notify"
    __cpp_name__ = "void* (*)(...)"
    __doc__ = "void sc_core::sc_event::notify()\nvoid sc_core::sc_event::notify(const sc_core::sc_time&)"


class _NativeEnum:
    """Simple enum proxy with its own native enumerator values."""

    SC_NS = 2
    SC_SEC = 6


_NativeEnum.__underlying = int


def test_class_help_preserves_constructor_candidate_and_cpp_types():
    """Class output includes its existing constructor signatures and argument types."""
    rendered = _formatter.format_native_help(_NativeClass)

    assert rendered.startswith("Native help: sc_core::sc_time")
    assert "Constructor overloads:" in rendered
    assert "sc_core::sc_time::sc_time(double, sc_core::sc_time_unit)" in rendered


def test_overload_help_preserves_every_existing_native_candidate():
    """Overloaded method output retains every cppyy signature line verbatim."""
    member = _NativeOverload()
    rendered = _formatter.format_native_help(member)

    assert rendered.startswith("Native help: notify")
    assert "void sc_core::sc_event::notify()" in rendered
    assert "void sc_core::sc_event::notify(const sc_core::sc_time&)" in rendered


def test_template_and_instantiation_keep_cpp_type_arguments():
    """Template help distinguishes its declaration from a width-bearing specialization."""
    template = type("Template", (), {"__cpp_name__": "sc_dt::sc_uint", "__doc__": "Native template."})()
    specialization = type(
        "sc_uint_meta",
        (),
        {"__cpp_name__": "sc_dt::sc_uint<73>", "__doc__": "Native arbitrary-width datatype."},
    )

    assert "uninstantiated C++ template" in _formatter.format_native_help(template)
    assert "sc_dt::sc_uint<73>" in _formatter.format_native_help(specialization, name="sc_uint")


def test_enum_help_lists_names_and_integer_values():
    """Enum output renders values available in the proxy's own dictionary."""
    rendered = _formatter.format_native_help(_NativeEnum)

    assert "SC_NS = 2" in rendered
    assert "SC_SEC = 6" in rendered


def test_python_adapter_fallback_doc_is_preserved():
    """Python-authored adapter documentation survives missing C++ metadata."""

    def adapter():
        """Start native simulation until completion or the selected duration."""

    rendered = _formatter.format_native_help(adapter, name="sc_start")
    assert rendered == "Native help: sc_start\n\nStart native simulation until completion or the selected duration."


def test_hidden_bridge_input_and_candidate_are_identified_without_leaking_docs():
    """Exact bridge declarations and candidate lines stay out of public help."""
    hidden = type(
        "bridge_proxy",
        (),
        {
            "__cpp_name__": "sc_core::python_wait",
            "__doc__": "void sc_core::python_wait(const sc_core::sc_time&) SECRET",
        },
    )()
    assert "nonpublic native bridge declaration" in _formatter.format_native_help(hidden)
    assert "SECRET" not in _formatter.format_native_help(hidden)

    public = type(
        "public_proxy",
        (),
        {
            "__cpp_name__": "sc_core::sc_time",
            "__doc__": ("sc_core::sc_time::sc_time()\nvoid sc_core::python_wait(const sc_core::sc_time&)"),
        },
    )()
    rendered = _formatter.format_native_help(public)
    assert "sc_core::sc_time::sc_time()" in rendered
    assert "python_wait" not in rendered
    assert "exact nonpublic native bridge candidate was omitted" in rendered

    specialized = type(
        "bridge_template",
        (),
        {"__cpp_name__": "sc_core::python_member_wait<sc_core::ScModule>", "__doc__": "Hidden."},
    )()
    assert "nonpublic native bridge declaration" in _formatter.format_native_help(specialized)


def test_exact_filter_preserves_user_prefixes_and_foreign_namespace_types():
    """Only declarations in the shared exact bridge inventory are removed."""
    public = type(
        "public_proxy",
        (),
        {
            "__cpp_name__": "sc_core::sc_time",
            "__doc__": (
                "void sc_core::python_custom(const user_ns::_type&)\n"
                "void std::__1::vector<user_ns::_type>::push_back(const user_ns::_type&)"
            ),
        },
    )()

    rendered = _formatter.format_native_help(public)

    assert "sc_core::python_custom" in rendered
    assert "std::__1::vector<user_ns::_type>" in rendered
    assert "user_ns::_type" in rendered


def test_missing_signature_metadata_is_explicit_and_name_validation_is_clear():
    """Prose-only native docs report unavailable signatures and reject bad names."""
    native_proxy = type("native_proxy", (), {"__cpp_name__": "sc_core::sc_time", "__doc__": "Duration."})()

    rendered = _formatter.format_native_help(native_proxy)

    assert "Duration." in rendered
    assert "Native signature metadata is unavailable." in rendered
    try:
        _formatter.format_native_help(native_proxy, name=73)
    except TypeError as error:
        assert "name must be a string or None" in str(error)
    else:
        raise AssertionError("a non-string display name must fail")


def test_native_help_requires_a_writable_text_stream():
    """The print helper rejects objects that cannot receive text output."""
    try:
        _formatter.native_help(_NativeClass, file=object())
    except TypeError as error:
        assert "text stream" in str(error)
    else:
        raise AssertionError("a stream without write() must fail")


def test_non_signature_overload_proxy_text_is_not_presented_as_a_candidate():
    """Aggregate metadata without native candidates does not masquerade as a signature."""

    def initialize(self):
        """Adapter constructor prose."""

    selector = types.SimpleNamespace(__self__=types.SimpleNamespace(__doc__="The type of the None singleton."))
    initialize.__overload__ = selector
    native_class = type(
        "WrappedNative",
        (),
        {"__cpp_name__": "sc_core::ScModule", "__doc__": "Native class.", "__init__": initialize},
    )

    rendered = _formatter.format_native_help(native_class)

    assert "Constructor documentation:" in rendered
    assert "Adapter constructor prose." in rendered
    assert "The type of the None singleton." not in rendered
    assert "Constructor overloads:" not in rendered


def test_sc_module_display_alias_is_text_only_and_native_help_prints():
    """ScModule displays through its lowercase public spelling without proxy changes."""
    module_class = type(
        "ScModule",
        (),
        {
            "__cpp_name__": "sc_core::ScModule",
            "__doc__": "sc_core::ScModule::ScModule(sc_core::sc_module_name)",
        },
    )
    output = io.StringIO()

    _formatter.native_help(module_class, file=output)

    assert "Native help: sc_core::sc_module" in output.getvalue()
    assert "sc_core::sc_module::ScModule(sc_core::sc_module_name)" in output.getvalue()
    assert "sc_core::ScModule" not in output.getvalue()
    assert module_class.__name__ == "ScModule"
    assert module_class.__cpp_name__ == "sc_core::ScModule"

    user_class = type("ScModuleExtra", (), {"__cpp_name__": "sc_core::ScModuleExtra", "__doc__": "User type."})
    assert _formatter.format_native_help(user_class).startswith("Native help: sc_core::ScModuleExtra")
