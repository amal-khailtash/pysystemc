"""Exercise defensive metadata handling through the public native-help APIs."""

from __future__ import annotations

import pytest

from .test_native_help_formatting import _formatter


@pytest.mark.parametrize("failure", [AttributeError, TypeError, ValueError])
def test_metadata_getter_failures_fall_back_without_masking_public_help(failure):
    """Known proxy metadata lookup failures produce useful fallback help."""

    class UnavailableMetadata:
        """Object whose cppyy-style metadata getters are unavailable."""

        def __getattribute__(self, attribute):
            """Raise a selected metadata failure while retaining normal methods."""
            if attribute in {"__cpp_name__", "__doc__", "__name__"}:
                raise failure("metadata unavailable")
            return object.__getattribute__(self, attribute)

    rendered = _formatter.format_native_help(UnavailableMetadata())

    assert rendered == "Native help: UnavailableMetadata\n\nNo native documentation metadata is available."


def test_public_help_handles_missing_and_non_text_docs_and_name_fallbacks():
    """Empty or non-text docs stay hidden while explicit names remain validated."""
    no_doc = type("NoDoc", (), {"__cpp_name__": "sc_core::sc_time", "__doc__": None})()
    blank_doc = type("BlankDoc", (), {"__cpp_name__": "sc_core::sc_time", "__doc__": " \n "})()
    non_text_doc = type("NonTextDoc", (), {"__cpp_name__": "sc_core::sc_time", "__doc__": 19})()

    for member in (no_doc, blank_doc, non_text_doc):
        rendered = _formatter.format_native_help(member)
        assert "No native documentation metadata is available (C++ name: sc_core::sc_time)." in rendered
        assert "Native signature metadata is unavailable." in rendered

    assert _formatter.format_native_help(no_doc, name="").startswith("Native help: sc_core::sc_time")
    with pytest.raises(TypeError, match="name must be a string or None"):
        _formatter.native_help(no_doc, name=object())


def test_unexpected_metadata_errors_keep_their_original_exception():
    """The formatter absorbs known reflection failures and propagates others."""

    class BrokenMetadata:
        """Object that models an unexpected proxy metadata failure."""

        def __getattribute__(self, attribute):
            """Raise the original runtime failure when native name metadata is read."""
            if attribute == "__cpp_name__":
                raise RuntimeError("metadata backend failed")
            return object.__getattribute__(self, attribute)

    with pytest.raises(RuntimeError, match="metadata backend failed"):
        _formatter.format_native_help(BrokenMetadata())


def test_enum_help_omits_unavailable_and_nonnumeric_values_but_keeps_metadata():
    """Public enum formatting retains convertible members and omits bad values."""
    enum = type(
        "MetadataEnum",
        (),
        {
            "__module__": "metadata_fixture",
            "__cpp_name__": "sc_core::sc_fixture_enum",
            "__doc__": "enum sc_core::sc_fixture_enum",
            "__underlying": int,
            "VALID": 4,
            "OVERFLOW": float("inf"),
            "WRONG_TYPE": object(),
            "TEXT": "not an integer",
        },
    )
    original_name = enum.__cpp_name__
    original_doc = enum.__doc__

    rendered = _formatter.format_native_help(enum)

    assert "enum sc_core::sc_fixture_enum" in rendered
    assert "VALID = 4" in rendered
    assert "OVERFLOW =" not in rendered
    assert "WRONG_TYPE =" not in rendered
    assert "TEXT =" not in rendered
    assert enum.__cpp_name__ == original_name
    assert enum.__doc__ == original_doc


def test_enum_without_docs_or_members_gets_explicit_no_metadata_text():
    """An enum with no available documentation or values reports that state."""
    enum = type("EmptyEnum", (), {"__cpp_name__": "sc_dt::sc_empty_enum", "__underlying": int})

    rendered = _formatter.format_native_help(enum)

    assert rendered.startswith("Native help: sc_dt::sc_empty_enum")
    assert "No native documentation metadata is available (C++ name: sc_dt::sc_empty_enum)." in rendered
    assert "Native signature metadata is unavailable." not in rendered


def test_enum_with_unavailable_own_dictionary_uses_no_docs_fallback():
    """A metadata proxy whose own dictionary cannot be read stays printable."""

    class UnavailableDictionary(type):
        """Metaclass modeling cppyy enum dictionary access failure."""

        def __getattribute__(cls, attribute):
            """Reject own-dictionary lookup while leaving other metadata readable."""
            if attribute == "__dict__":
                raise TypeError("enum members are unavailable")
            return super().__getattribute__(attribute)

    enum = UnavailableDictionary(
        "UnavailableEnum",
        (),
        {"__cpp_name__": "sc_core::sc_unavailable_enum", "__underlying": int},
    )

    rendered = _formatter.format_native_help(enum)

    assert rendered.startswith("Native help: sc_core::sc_unavailable_enum")
    assert "No native documentation metadata is available (C++ name: sc_core::sc_unavailable_enum)." in rendered
    assert "Enum members:" not in rendered


def test_foreign_cpp_names_and_arbitrary_docs_survive_exact_bridge_filtering():
    """Only exact bridge identifiers are hidden from otherwise arbitrary docs."""
    member = type(
        "UserProxy",
        (),
        {
            "__cpp_name__": "user_ns::python_wait",
            "__doc__": (
                "user_ns::python_wait(user_ns::_payload)\n"
                "void sc_core::python_custom(const user_ns::_payload&)\n"
                "void sc_core::python_wait(const sc_core::sc_time&) SECRET"
            ),
        },
    )()
    original_doc = member.__doc__
    original_name = member.__cpp_name__

    rendered = _formatter.format_native_help(member)

    assert "user_ns::python_wait(user_ns::_payload)" in rendered
    assert "sc_core::python_custom" in rendered
    assert "sc_core::python_wait" not in rendered
    assert "SECRET" not in rendered
    assert "An exact nonpublic native bridge candidate was omitted." in rendered
    assert member.__doc__ == original_doc
    assert member.__cpp_name__ == original_name


def test_only_the_root_cpp_member_uses_the_exact_bridge_exclusion():
    """Nested user members named like a bridge stay visible under public roots."""
    nested_public_member = type(
        "NestedPublicMember",
        (),
        {
            "__cpp_name__": "sc_core::PublicContainer::python_wait",
            "__doc__": "void sc_core::PublicContainer::python_wait()",
        },
    )()
    nested_hidden_member = type(
        "NestedHiddenMember",
        (),
        {
            "__cpp_name__": "sc_core::python_wait::overload",
            "__doc__": "Hidden bridge member.",
        },
    )()

    public_rendered = _formatter.format_native_help(nested_public_member)
    hidden_rendered = _formatter.format_native_help(nested_hidden_member)

    assert public_rendered.startswith("Native help: sc_core::PublicContainer::python_wait")
    assert "void sc_core::PublicContainer::python_wait()" in public_rendered
    assert "nonpublic native bridge declaration" not in public_rendered
    assert hidden_rendered == (
        "Native help: sc_core::python_wait\nThis is a nonpublic native bridge declaration; documentation is omitted."
    )
