"""Check native adapter metadata and error edges without kernel simulations."""

from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import subprocess
import sys
from typing import Any

from .support import ROOT


def _load_native_method_class() -> type[Any]:
    """Load the pure Python descriptor without initializing the SystemC package.

    Returns:
        The ``NativeMethod`` descriptor class from its source module.
    """
    source = ROOT / "src/amal/eda/systemc/sc_dt/_native_methods.py"
    spec = importlib.util.spec_from_file_location(
        "pre_example_adapter_edges_native_methods",
        source,
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.NativeMethod


def _run_native_probe(tmp_path: Path, source: str) -> str:
    """Run one isolated native adapter probe and return its asserted output.

    Args:
        tmp_path: Working directory for the child process.
        source: Python source to execute in the fresh interpreter.

    Returns:
        Captured standard output from the successful child process.

    Raises:
        AssertionError: The child exits unsuccessfully.
        subprocess.TimeoutExpired: Native initialization exceeds one minute.
    """
    script = tmp_path / "adapter_edges_probe.py"
    script.write_text(source)
    environment = dict(
        os.environ,
        SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")),
    )
    result = subprocess.run(
        [sys.executable, str(script)],
        cwd=tmp_path,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def test_native_method_class_access_calls_adapter_with_explicit_owner() -> None:
    """Class access preserves the descriptor and passes the explicit owner once."""
    native_method = _load_native_method_class()
    calls: list[tuple[object, tuple[object, ...], dict[str, object]]] = []

    def adapter(owner: object, *args: object, **kwargs: object) -> tuple[object, tuple[object, ...], dict[str, object]]:
        """Record the explicit owner and arguments received by the adapter."""
        record = (owner, args, kwargs)
        calls.append(record)
        return record

    descriptor = native_method(adapter, None)

    class Model:
        """Expose the descriptor on a class for unbound access."""

        method = descriptor

    owner = Model()
    assert Model.method is descriptor
    assert Model.method(owner, 7, label="class-access") == (owner, (7,), {"label": "class-access"})
    assert calls == [(owner, (7,), {"label": "class-access"})]


def test_native_member_token_metadata_and_public_error_edges(tmp_path: Path) -> None:
    """Check token metadata, invalid input, and cause-preserving member rejection."""
    output = _run_native_probe(
        tmp_path,
        '''
"""Probe real cppyy member-token metadata and public named-member errors."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef("""
/** @brief Native fields and a method for member-token adapter edge checks. */
struct PreExampleAdapterElement
{
  int value = 0;
  int method() const
  {
    return value;
  }
};
""")

Element = cppyy.gbl.PreExampleAdapterElement
policy_type = sc.sc_member_access[Element, int]
token_type = policy_type.member_type

# This installed native specialization has no member_type alias. The public
# token still exposes its synthesized C++ pointer spelling and rejects unknown
# alias-only metadata through the fallback branch.
native_policy_type = cppyy.gbl.sc_core.sc_member_access[Element, int]
assert not hasattr(native_policy_type, "member_type")
assert token_type.native_cpp_name == f"int {Element.__cpp_name__}::*"
try:
    token_type.unknown_native_metadata
except AttributeError as error:
    assert "unknown_native_metadata" in str(error)
else:
    raise AssertionError("missing native alias metadata was accepted")

try:
    token_type.from_member(object())
except TypeError as error:
    assert "public C++ data-member identifier" in str(error)
else:
    raise AssertionError("from_member accepted a non-string value")

# A native method exists on the class but is not a data-member pointer. The
# first request records the native reflection failure; the second exercises
# the cached failure path. Both public errors retain the lower-level cause.
for attempt in range(2):
    try:
        policy_type("method")
    except TypeError as error:
        assert "cannot use member 'method'" in str(error)
        assert isinstance(error.__cause__, TypeError)
        assert "not an accessible non-static data member" in str(error.__cause__)
    else:
        raise AssertionError("method name was accepted as a member field")

print("ADAPTER_EDGES native_alias_absent=1 metadata=1 nonstring=TypeError cached_member=2 cause=2")
''',
    )
    assert (
        "ADAPTER_EDGES native_alias_absent=1 metadata=1 nonstring=TypeError cached_member=2 cause=2"
        in output.splitlines()
    )
