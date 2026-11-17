"""Verify native help output while preserving cppyy objects and kernel state."""

import builtins
from contextlib import redirect_stdout
from io import StringIO
import pydoc

import amal.eda.systemc as systemc

# The package import owns native initialization and must precede cppyy access.
import cppyy

from amal.eda.systemc import sc_core, sc_dt
from amal.eda.systemc.repl import _seed_namespace


def invalid_time_error():
    """Return the native overload diagnostic for an invalid sc_time call."""
    try:
        sc_core.sc_time("not a time")
    except TypeError as error:
        return str(error)
    raise AssertionError("invalid sc_time construction unexpectedly succeeded")


def main():
    """Check signatures, aliases, output streams, and unchanged native state."""
    native = cppyy.gbl.sc_core
    native_time = sc_core.sc_time
    native_time_doc = native_time.__doc__
    native_constructor_doc = native_time.__init__.__doc__
    pydoc_before = pydoc.render_doc(native_time, renderer=pydoc.plaintext)
    help_builtin_before = builtins.help
    now_before = sc_core.sc_time(native.sc_time_stamp())
    status_before = native.sc_get_status()
    zero_proxy = sc_core.SC_ZERO_TIME
    zero_value_before = zero_proxy.value()
    zero_ownership_before = zero_proxy.__python_owns__
    error_before = invalid_time_error()

    # Constructor candidates and native method return spelling come from
    # cppyy metadata. The formatter adds presentation text without wrapping
    # or replacing any native proxy.
    time_text = systemc.format_native_help(native_time)
    assert "sc_time" in time_text
    assert "sc_time::sc_time(double, sc_core::sc_time_unit)" in time_text
    assert "sc_time::sc_time(double, const char* unit)" in time_text
    method_text = systemc.format_native_help(sc_core.sc_time(3, sc_core.SC_NS).to_seconds)
    method_heading = method_text.splitlines()[0]
    assert "to_seconds" in method_heading
    assert "(*)(...)" not in method_heading
    assert "double sc_core::sc_time::to_seconds()" in method_text

    template_text = systemc.format_native_help(sc_dt.sc_uint)
    specialization = sc_dt.sc_uint[73]
    specialization_text = systemc.format_native_help(specialization)
    assert "sc_dt::sc_uint" in template_text
    assert "Type[parameters]" in template_text or "template" in template_text.lower()
    assert "sc_dt::sc_uint<73>" in specialization_text

    enum_text = systemc.format_native_help(sc_core.sc_time_unit)
    assert "sc_time_unit" in enum_text
    assert "SC_NS" in enum_text
    assert int(sc_core.SC_NS) == 2
    assert "SC_NS = 2" in enum_text

    adapter_text = systemc.format_native_help(sc_core.sc_start)
    assert "lifetime" in adapter_text.lower()
    assert "gil" in adapter_text.lower()

    # ScModule is a text-only public spelling. Both module exports still
    # resolve to the same native type with its original C++ name.
    assert sc_core.ScModule is sc_core.sc_module
    assert sc_core.ScModule is cppyy.gbl.sc_core.ScModule
    assert sc_core.ScModule.__cpp_name__ == "sc_core::ScModule"
    alias_text = systemc.format_native_help(sc_core.ScModule)
    assert "sc_core::sc_module" in alias_text
    assert "sc_core::ScModule" not in alias_text
    assert "The type of the None singleton." not in alias_text

    # Passing an exact hidden bridge proxy directly must identify it as
    # nonpublic and suppress its implementation overload documentation.
    bridge = cppyy.gbl.sc_core.python_wait
    bridge_text = systemc.format_native_help(bridge)
    assert "nonpublic native bridge declaration" in bridge_text.lower()
    assert "bool sc_core::python_wait(" not in bridge_text

    # The package API and the initialized REPL locals expose the same callable
    # objects. Exercise both the supplied file and redirected stdout paths.
    assert callable(systemc.format_native_help)
    assert callable(systemc.native_help)
    namespace, owners = _seed_namespace()
    assert owners
    assert namespace["format_native_help"] is systemc.format_native_help
    assert namespace["native_help"] is systemc.native_help
    assert namespace["native_help"](sc_dt.sc_uint[73], file=StringIO()) is None

    file_output = StringIO()
    assert systemc.native_help(sc_core.sc_time, file=file_output) is None
    assert file_output.getvalue().strip() == time_text.strip()
    stdout_output = StringIO()
    with redirect_stdout(stdout_output):
        assert systemc.native_help(sc_core.sc_time) is None
    assert stdout_output.getvalue().strip() == time_text.strip()

    # Generic help/pydoc and native docs remain untouched, and native invalid
    # construction continues to show its overload candidates.
    assert builtins.help is help_builtin_before
    assert sc_core.sc_time is native_time is native.sc_time
    assert native_time.__doc__ == native_time_doc
    assert native_time.__init__.__doc__ == native_constructor_doc
    assert pydoc.render_doc(native_time, renderer=pydoc.plaintext) == pydoc_before
    error_after = invalid_time_error()
    assert error_after == error_before
    assert "none of the 8 overloaded methods succeeded" in error_after
    assert "sc_time::sc_time(double, sc_core::sc_time_unit)" in error_after
    assert "sc_time::sc_time(double, const char* unit)" in error_after

    # Help is observational: no simulation or reset occurred and the exported
    # zero-time value retains its native value and cppyy ownership state.
    assert native.sc_time_stamp() == now_before
    assert native.sc_get_status() == status_before
    assert sc_core.SC_ZERO_TIME is zero_proxy
    assert zero_proxy.value() == zero_value_before == 0
    assert zero_proxy.__python_owns__ == zero_ownership_before
    print("NATIVE_HELP_ACCEPTANCE_OK")


main()
