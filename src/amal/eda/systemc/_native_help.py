"""Format cppyy documentation for native SystemC API objects without pydoc traversal."""

from __future__ import annotations

import re
import sys
from typing import Any, TextIO

from ._namespace import _BRIDGE_EXCLUSIONS


_QUALIFIED_IDENTIFIER = re.compile(r"\b([A-Za-z_]\w*)::([A-Za-z_]\w*)")
_SIGNATURE_DECLARATION = re.compile(r"^\s*.*?\b([A-Za-z_]\w*)::([A-Za-z_]\w*)(?:::|\s*[<(])")
_SCMODULE_NAMES = {"ScModule", "sc_module", "sc_core::ScModule", "sc_core::sc_module", "sc_core.ScModule"}
_NATIVE_NAMESPACES = {"sc_core", "sc_dt", "tlm", "tlm_utils"}


def _is_exact_bridge_exclusion(namespace: str, name: str) -> bool:
    """Match only declarations recorded as bridge or implementation details."""
    member_match = re.match(r"([A-Za-z_]\w*)", name)
    member_name = member_match.group(1) if member_match else name
    return member_name in _BRIDGE_EXCLUSIONS.get(namespace, {})


def _metadata(member: object, attribute: str) -> Any:
    """Read one documented cppyy metadata attribute without invoking callables."""
    try:
        return getattr(member, attribute, None)
    except AttributeError, TypeError, ValueError:
        return None


def _qualified_cpp_name(member: object) -> str | None:
    """Return cppyy's native type spelling when it is available."""
    value = _metadata(member, "__cpp_name__")
    return value if isinstance(value, str) and value else None


def _hidden_declaration(cpp_name: str | None) -> str | None:
    """Return an exact nonpublic namespace member name, if the proxy names one."""
    if not cpp_name or "::" not in cpp_name:
        return None
    namespace = cpp_name.split("::", maxsplit=1)[0]
    if namespace not in _NATIVE_NAMESPACES:
        return None
    declaration_text = cpp_name[len(namespace) + 2 :].split("<", maxsplit=1)[0]
    root_declaration = re.match(r"([A-Za-z_]\w*)", declaration_text)
    if root_declaration:
        member_name = root_declaration.group(1)
        if _is_exact_bridge_exclusion(namespace, member_name):
            return f"{namespace}::{member_name}"
    return None


def _hidden_doc_declarations(value: object) -> list[str]:
    """Find exact hidden declarations named by native signature lines."""
    if not isinstance(value, str):
        return []
    hidden = []
    for line in value.splitlines():
        match = _SIGNATURE_DECLARATION.match(line)
        if not match:
            continue
        namespace, member_name = match.groups()
        if namespace in _NATIVE_NAMESPACES and _is_exact_bridge_exclusion(namespace, member_name):
            declaration = f"{namespace}::{member_name}"
            if declaration not in hidden:
                hidden.append(declaration)
    return hidden


def _display_name(member: object, name: str | None, cpp_name: str | None) -> str:
    """Choose a heading spelling while keeping native proxy identity untouched."""
    native_name = cpp_name if cpp_name and "::" in cpp_name and "(*)(...)" not in cpp_name else None
    original = name or native_name or _metadata(member, "__name__")
    if not isinstance(original, str) or not original:
        original = type(member).__name__
    if cpp_name == "sc_core::ScModule" and original in _SCMODULE_NAMES:
        return "sc_core::sc_module"
    original = re.sub(r"\bsc_core::ScModule\b", "sc_core::sc_module", original)
    return re.sub(r"\bsc_core\.ScModule\b", "sc_core::sc_module", original)


def _contains_hidden_candidate(line: str) -> bool:
    """Check qualified identifiers against the shared exact namespace policy."""
    for match in _QUALIFIED_IDENTIFIER.finditer(line):
        namespace, member_name = match.groups()
        if namespace in _NATIVE_NAMESPACES and _is_exact_bridge_exclusion(namespace, member_name):
            return True
    return False


def _visible_doc(value: object) -> tuple[str | None, bool]:
    """Return existing documentation with exact hidden bridge candidate lines removed."""
    if not isinstance(value, str) or not value.strip():
        return None, False
    lines = value.strip().splitlines()
    visible_lines = [line for line in lines if not _contains_hidden_candidate(line)]
    omitted = len(visible_lines) != len(lines)
    rendered = "\n".join(visible_lines).strip() or None
    if rendered:
        rendered = re.sub(r"\bsc_core::ScModule\b", "sc_core::sc_module", rendered)
        rendered = re.sub(r"\bsc_core\.ScModule\b", "sc_core::sc_module", rendered)
    return rendered, omitted


def _is_enum(member: object) -> bool:
    """Recognize cppyy enum proxies without walking their inherited attributes."""
    if not isinstance(member, type):
        return False
    try:
        return "__underlying" in vars(member)
    except TypeError:
        return False


def _enum_members(member: object) -> list[str]:
    """Render available enum names and values from the proxy's own dictionary."""
    try:
        own_members = vars(member).items()
    except TypeError:
        return []
    values = []
    for enum_name, value in own_members:
        if enum_name.startswith("_") or callable(value):
            continue
        try:
            enum_value = int(value)
        except OverflowError, TypeError, ValueError:
            continue
        values.append(f"    {enum_name} = {enum_value}")
    return values


def _is_cpp_template(member: object) -> bool:
    """Identify cppyy's uninstantiated template proxy without instantiating it."""
    return type(member).__name__ == "Template"


def _overload_documentation(member: object) -> str | None:
    """Read aggregate CPPOverload documentation attached to an adapter wrapper."""
    selector = _metadata(member, "__overload__")
    overload_proxy = _metadata(selector, "__self__")
    return _metadata(overload_proxy, "__doc__")


def format_native_help(member: object, *, name: str | None = None) -> str:
    """Format existing cppyy metadata for one native class, overload, enum or template.

    The formatter reads existing ``__doc__``, ``__cpp_name__`` and ``__name__``
    metadata, including aggregate constructor docs on an adapter's
    ``__overload__.__self__`` proxy. It does not call the represented native
    member, instantiate a template, change the proxy, or traverse inherited
    pydoc data.

    Args:
        member: A cppyy native proxy or a Python adapter with useful documentation.
        name: Optional public display name. This changes rendered text only.

    Returns:
        The heading and available native documentation as plain text.

    Raises:
        TypeError: If ``name`` is not a string or ``None``.

    A native class's constructor documentation is read from its existing
    ``__init__.__doc__`` metadata and any retained aggregate overload proxy. If
    cppyy exposes no documentation, the formatter states that explicitly and
    retains any available C++ name.
    """
    if name is not None and not isinstance(name, str):
        raise TypeError("name must be a string or None")

    cpp_name = _qualified_cpp_name(member)
    member_doc = _metadata(member, "__doc__")
    hidden_name = _hidden_declaration(cpp_name)
    hidden_doc_names = _hidden_doc_declarations(member_doc)
    if not hidden_name and hidden_doc_names and len(hidden_doc_names) == 1:
        # Native free-function proxies may expose candidate docs while leaving
        # __cpp_name__ as the generic function-pointer spelling.
        if not cpp_name or "(*)(...)" in cpp_name:
            hidden_name = hidden_doc_names[0]
    if hidden_name:
        return f"Native help: {hidden_name}\nThis is a nonpublic native bridge declaration; documentation is omitted."

    display_name = _display_name(member, name, cpp_name)
    sections = [f"Native help: {display_name}"]
    docs: list[str] = []
    omitted_hidden = False

    doc, omitted = _visible_doc(member_doc)
    if doc:
        docs.append(doc)
    omitted_hidden |= omitted

    is_enum = _is_enum(member)
    is_template = _is_cpp_template(member)
    constructor = None
    overload_doc = None
    if isinstance(member, type) and not is_enum:
        constructor = _metadata(member, "__init__")
        if constructor is not object.__init__:
            overload_doc, overload_omitted = _visible_doc(_overload_documentation(constructor))
            constructor_doc, omitted = _visible_doc(_metadata(constructor, "__doc__"))
            overload_has_candidates = bool(
                overload_doc and any(_SIGNATURE_DECLARATION.match(line) for line in overload_doc.splitlines())
            )
            if overload_has_candidates and overload_doc != constructor_doc:
                docs.append("Constructor overloads:\n" + overload_doc)
            if constructor_doc:
                label = (
                    "Constructor overloads:\n"
                    if any(_SIGNATURE_DECLARATION.match(line) for line in constructor_doc.splitlines())
                    else "Constructor documentation:\n"
                )
                docs.append(label + constructor_doc)
            omitted_hidden |= omitted or overload_omitted

    if docs:
        sections.extend(docs)
    if is_enum:
        enum_members = _enum_members(member)
        if enum_members:
            sections.append("Enum members:\n" + "\n".join(enum_members))

    if not docs and not (is_enum and len(sections) > 1):
        details = f" (C++ name: {cpp_name})" if cpp_name else ""
        sections.append(f"No native documentation metadata is available{details}.")

    if cpp_name and not is_template and not is_enum:
        has_signature = any(_SIGNATURE_DECLARATION.match(line) for text in docs for line in text.splitlines())
        has_signature |= bool(
            overload_doc and any(_SIGNATURE_DECLARATION.match(line) for line in overload_doc.splitlines())
        )
        if not has_signature:
            sections.append("Native signature metadata is unavailable.")

    if is_template:
        sections.append("This is an uninstantiated C++ template; no specialization was created.")
    elif cpp_name and cpp_name not in display_name and "<" in cpp_name:
        sections.append(f"Native C++ type: {cpp_name}")
    if omitted_hidden:
        sections.append("An exact nonpublic native bridge candidate was omitted.")

    rendered = "\n\n".join(sections)
    rendered, _ = _visible_doc(rendered)
    return rendered or f"Native help: {display_name}"


def native_help(member: object, *, name: str | None = None, file: TextIO | None = None) -> None:
    """Print local native help to standard output or a supplied text stream.

    Args:
        member: A cppyy native proxy or documented Python adapter.
        name: Optional public display name, affecting rendered text only.
        file: Text stream to receive help; ``None`` selects current ``sys.stdout``.

    Returns:
        ``None`` after writing the formatted help and a trailing newline.

    Raises:
        TypeError: If ``name`` is not a string or the selected stream does not
            provide a compatible ``write`` method.
    """
    stream = sys.stdout if file is None else file
    writer = getattr(stream, "write", None)
    if not callable(writer):
        raise TypeError("file must be a text stream with a write method")
    writer(format_native_help(member, name=name) + "\n")
