"""Exercise public-name completion in an isolated Python process."""

import builtins
import json
import sys


def _complete(completer, prefix: str) -> set[str]:
    """Return real ``rlcompleter`` results for one prompt prefix.

    Args:
        completer: The completer installed by :class:`Repl`.
        prefix: Bare or attribute expression immediately before the cursor.

    Returns:
        Complete candidate spellings from the live REPL namespace.
    """
    from prompt_toolkit.completion import CompleteEvent
    from prompt_toolkit.document import Document

    return {completion.text for completion in completer.get_completions(Document(prefix), CompleteEvent())}


def _has_candidate(completer, prefix: str, expected: str) -> bool:
    """Match one completion, including rlcompleter's call/index suffix.

    Args:
        completer: The completer installed by :class:`Repl`.
        prefix: Text entered before the cursor.
        expected: Public spelling that should be suggested.

    Returns:
        True when completion returns the spelling or its opening call/index.
    """
    return any(
        item == expected or item.startswith((expected + "(", expected + "[")) for item in _complete(completer, prefix)
    )


def main() -> None:
    """Assert public and private completion behavior without running a kernel."""
    assert sys.version_info[:2] == (3, 14), sys.version

    from amal.eda.systemc import sc_core, sc_dt, tlm
    import amal.eda.systemc.tlm_utils as tlm_utils
    from amal.eda.systemc.repl import Repl
    import cppyy

    native_core = cppyy.gbl.sc_core
    time_snapshot = native_core.sc_time(native_core.sc_time_stamp())
    status_snapshot = native_core.sc_get_status()
    assert time_snapshot == native_core.SC_ZERO_TIME

    # This user-defined native name shares the implementation-like prefix but
    # has no first-party bridge provenance and must stay discoverable. Define it
    # after package startup so cppyy reuses the loaded native headers.
    cppyy.cppdef(
        """
        namespace sc_core {
        /** @brief Test-native type with a user-owned implementation-like name. */
        struct python_user_probe_native {};
        }
        """
    )

    class InjectedSession:
        """Satisfy Repl's construction boundary without opening a terminal."""

        def __init__(self, **kwargs):
            """Retain the supplied prompt configuration for inspection."""
            self.kwargs = kwargs

    repl = Repl(no_history=True, prompt_session_factory=InjectedSession)
    namespace = repl.namespace
    completer = repl.completer

    # REPL bare-name seeding follows the public export list exactly, retaining
    # each exported object rather than creating presentation wrappers.
    for name in sc_core.__all__:
        if name not in vars(builtins):
            assert name in namespace, name
            assert namespace[name] is getattr(sc_core, name), name
    for name in ("CFT", "sc_plist_map_fn", "sc_report_handler_proc", "sc_placeholder"):
        assert name in sc_core.__all__ and name in namespace, name
        assert namespace[name] is getattr(sc_core, name), name
        assert _has_candidate(completer, name[: max(2, len(name) - 1)], name), name

    # Public native and convenience names complete through bare and module
    # attribute paths, including the public alias to the native placeholder.
    for name in ("sc_time", "sc_placeholder"):
        assert _has_candidate(completer, name[: max(2, len(name) - 1)], name), name
    public_paths = (
        ("sc_core", "sc_time"),
        ("sc_dt", "sc_context"),
        ("tlm", "tlm_analysis_fifo"),
        ("tlm_utils", "simple_target_socket"),
    )
    modules = {"sc_core": sc_core, "sc_dt": sc_dt, "tlm": tlm, "tlm_utils": tlm_utils}
    for namespace_name, name in public_paths:
        module = modules[namespace_name]
        assert name in module.__all__, f"{namespace_name}.{name} missing from __all__"
        assert name in dir(module), f"{namespace_name}.{name} missing from dir()"
        qualified = f"{namespace_name}.{name}"
        assert _has_candidate(
            completer,
            f"{namespace_name}.{name[: max(2, len(name) - 1)]}",
            qualified,
        ), qualified

    # A similar-prefix C++ declaration that is not in the exact bridge
    # inventory remains a public native object and retains native identity.
    raw_user_type = cppyy.gbl.sc_core.python_user_probe_native
    assert sc_core.python_user_probe_native is raw_user_type
    assert "python_user_probe_native" in dir(sc_core)
    assert _has_candidate(completer, "sc_core.python_user_probe_nativ", "sc_core.python_user_probe_native")

    # The known bridge declarations remain usable through raw cppyy for
    # binding/debugging, but are absent from Python REPL discovery.
    hidden_names = ("python_wait", "python_member_wait", "PythonInterface")
    raw_core = cppyy.gbl.sc_core
    for name in hidden_names:
        assert getattr(raw_core, name) is not None, name
        assert name not in namespace, f"{name} was seeded as a bare name"
        assert name not in _complete(completer, name[: max(2, len(name) - 1)]), name
        assert not any(
            item.startswith(f"sc_core.{name}")
            for item in _complete(completer, f"sc_core.{name[: max(2, len(name) - 1)]}")
        ), name

    # Public module fallback remains available for ordinary API attributes,
    # while the same exact bridge names do not complete through that module.
    assert sc_core.sc_time is cppyy.gbl.sc_core.sc_time
    assert sc_core.sc_placeholder is cppyy.gbl.sc_core.python_placeholder
    for name in hidden_names:
        assert not any(
            item.startswith(f"sc_core.{name}")
            for item in _complete(completer, f"sc_core.{name[: max(2, len(name) - 1)]}")
        ), name

    # Datatype specialization keeps its original arbitrary-width C++ proxy.
    uint73 = sc_dt.sc_uint[73]
    biguint73 = sc_dt.sc_biguint[73]
    assert uint73 is cppyy.gbl.sc_dt.sc_uint[73]
    assert biguint73 is cppyy.gbl.sc_dt.sc_biguint[73]
    assert uint73.__cpp_name__ == "sc_dt::sc_uint<73>"
    assert biguint73.__cpp_name__ == "sc_dt::sc_biguint<73>"

    # Local Python variables with a similar prefix remain completable too.
    namespace["python_user_note"] = object()
    assert "python_user_note" in _complete(completer, "python_user_no")

    # Namespace lookup and completion must not enter or mutate the singleton
    # kernel. The explicit native copy keeps this invariant independent of a
    # borrowed return proxy from sc_time_stamp().
    assert native_core.sc_time_stamp() == time_snapshot
    assert native_core.sc_get_status() == status_snapshot

    print(
        "REPL_PRESENTATION_OK",
        json.dumps(
            {
                "public_paths": [f"{ns}.{name}" for ns, name in public_paths],
                "preserved_callback_aliases": ["CFT", "sc_plist_map_fn", "sc_report_handler_proc"],
                "public_placeholder_alias": "sc_placeholder",
                "hidden_bridge_names": list(hidden_names),
                "unknown_native_prefix_name": "python_user_probe_native",
                "unknown_python_prefix_name": "python_user_note",
                "template_cpp_names": [uint73.__cpp_name__, biguint73.__cpp_name__],
                "seeded_exports": len(sc_core.__all__),
                "kernel_time": str(time_snapshot),
                "kernel_status": int(status_snapshot),
            },
            sort_keys=True,
        ),
    )


if __name__ == "__main__":
    main()
