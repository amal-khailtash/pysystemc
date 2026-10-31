"""Verify consistent Python namespace presentation without running simulation."""

from amal.eda.systemc import sc_core, sc_dt, tlm, tlm_utils
from amal.eda.systemc._namespace import _BRIDGE_EXCLUSIONS
import cppyy


def documented_inventory_names(module):
    """Return exact names listed under the generated namespace categories."""
    discovery = module.__doc__.split("Native API discovery", maxsplit=1)[1]
    category_lines = (
        "Classes:",
        "Templates:",
        "Enums:",
        "Functions:",
        "Constants:",
    )
    names = set()
    for line in discovery.splitlines():
        for heading in category_lines:
            if line.startswith(heading):
                names.update(name.strip() for name in line[len(heading) :].split(",") if name.strip())
                break
    return names


def expect_hidden_names(module, names):
    """Require every exact bridge candidate to be absent from Python presentation."""
    documented = documented_inventory_names(module)
    for name in names:
        assert name not in module.__all__, (module.__name__, name, "__all__")
        assert name not in dir(module), (module.__name__, name, "dir")
        assert name not in documented, (module.__name__, name, "module inventory")
        try:
            getattr(module, name)
        except AttributeError:
            pass
        else:
            raise AssertionError(f"{module.__name__}.{name} resolved through Python")


def expect_raw_native_names(native, names):
    """Require representative bridge helpers to remain accessible through cppyy."""
    for name in names:
        assert getattr(native, name) is not None, (native.__cpp_name__, name, "raw cppyy")


def check_wildcard_identity(module, names):
    """Verify wildcard imports publish the same Python adapter/native objects."""
    imported = {}
    exec(f"from {module.__name__} import *", imported)
    for name in names:
        assert name in module.__all__, (module.__name__, name, "missing __all__ export")
        assert name in dir(module), (module.__name__, name, "missing dir entry")
        assert name in module.__doc__, (module.__name__, name, "missing module help")
        assert imported[name] is getattr(module, name), (module.__name__, name, "wildcard identity")


def main():
    """Check bridge filtering, preserved public aliases and unchanged kernel state."""
    native_core = cppyy.gbl.sc_core
    time_before = native_core.sc_time_stamp()
    status_before = native_core.sc_get_status()
    assert time_before == native_core.SC_ZERO_TIME

    hidden_sc_core = tuple(_BRIDGE_EXCLUSIONS["sc_core"])
    expect_hidden_names(sc_core, hidden_sc_core)
    expect_raw_native_names(
        cppyy.gbl.sc_core,
        (
            "PythonInterface",
            "PythonSignalChannel",
            "PythonReadInterface",
            "python_wait",
            "python_start",
            "python_member_wait",
            "python_bound",
            "dispatch_python",
            "python_placeholder",
        ),
    )

    # Scope filtering by exact provenance must retain native-looking API names
    # and the deliberately public aliases to first-party bridge implementations.
    assert sc_core.sc_interface is cppyy.gbl.sc_core.PythonInterface
    assert sc_core.sc_prim_channel is cppyy.gbl.sc_core.PythonPrimChannel
    assert sc_core.signal_channel is cppyy.gbl.sc_core.PythonSignalChannel
    assert sc_core.sc_placeholder is cppyy.gbl.sc_core.python_placeholder
    assert sc_core.ScModule is sc_core.sc_module
    assert "add_sc_core_members" not in dir(sc_core)
    assert "add_sc_core_members" not in sc_core.__all__

    check_wildcard_identity(
        sc_core,
        (
            "ScModule",
            "sc_module",
            "CFT",
            "sc_placeholder",
            "sc_time",
            "sc_time_unit",
            "SC_NS",
        ),
    )
    check_wildcard_identity(
        sc_dt,
        ("sc_context", "sc_uint", "sc_biguint", "SC_TRN", "concat", "to_string"),
    )
    check_wildcard_identity(
        tlm,
        ("tlm_analysis_fifo", "tlm_generic_payload", "TLM_OK_RESPONSE", "tlm_to_hostendian_generic"),
    )
    check_wildcard_identity(
        tlm_utils,
        (
            "simple_initiator_socket",
            "simple_target_socket",
            "multi_passthrough_target_socket",
            "peq_with_get",
            "tlm_quantumkeeper",
            "callback_target_socket",
            "callback_peq",
            "callback_quantumkeeper",
        ),
    )

    # Template arguments stay native and unrestricted at widths outside small
    # hand-instantiated test ranges.
    uint73 = sc_dt.sc_uint[73]
    biguint73 = sc_dt.sc_biguint[73]
    assert uint73.__cpp_name__ == "sc_dt::sc_uint<73>"
    assert biguint73.__cpp_name__ == "sc_dt::sc_biguint<73>"

    assert native_core.sc_time_stamp() == time_before
    assert native_core.sc_get_status() == status_before
    print("NAMESPACE_PRESENTATION_OK")


main()
