"""Verify live public kernel flags and ordinary sc_core module assignments."""

from amal.eda.systemc import sc_core as sc
import cppyy

FLAG_NAMES = ("sc_enable_name_checking", "sc_allow_process_control_corners")
native_namespace = cppyy.gbl.sc_core
originals = {name: getattr(native_namespace, name) for name in FLAG_NAMES}
assert all(type(value) is bool for value in originals.values())
assert all(getattr(sc, name) is value for name, value in originals.items())
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME

wildcard_snapshot = {}
exec("from amal.eda.systemc.sc_core import *", wildcard_snapshot)
direct_snapshot = {}
exec(
    "from amal.eda.systemc.sc_core import sc_enable_name_checking, sc_allow_process_control_corners",
    direct_snapshot,
)
assert all(wildcard_snapshot[name] is value for name, value in originals.items())
assert all(direct_snapshot[name] is value for name, value in originals.items())

temporary_name = "_kernel_flag_module_edge_temporary_member"
temporary_value = object()
assert not hasattr(sc, temporary_name)
native_unchanged = True
try:
    setattr(sc, temporary_name, temporary_value)
    assert getattr(sc, temporary_name) is temporary_value
    assert sc.__dict__[temporary_name] is temporary_value
    native_unchanged = all(getattr(native_namespace, name) is value for name, value in originals.items())
    assert native_unchanged
    delattr(sc, temporary_name)
    assert temporary_name not in sc.__dict__
    try:
        getattr(sc, temporary_name)
    except AttributeError:
        pass
    else:
        raise AssertionError("deleted ordinary sc_core module member remained accessible")

    assignment_count = 0
    raw_assignment_count = 0
    invalid_assignment_count = 0
    for name, original in originals.items():
        changed = not original
        setattr(sc, name, changed)
        assignment_count += 1
        assert getattr(sc, name) is changed
        assert getattr(native_namespace, name) is changed
        assert wildcard_snapshot[name] is original
        assert direct_snapshot[name] is original

        setattr(native_namespace, name, original)
        raw_assignment_count += 1
        assert getattr(sc, name) is original
        setattr(native_namespace, name, changed)
        assert getattr(sc, name) is changed
        assert wildcard_snapshot[name] is original
        assert direct_snapshot[name] is original

        for invalid in (1, 0, None, "false"):
            try:
                setattr(sc, name, invalid)
            except TypeError:
                pass
            else:
                raise AssertionError(f"{name} accepted nonboolean value {invalid!r}")
            invalid_assignment_count += 1
            assert getattr(sc, name) is changed
            assert getattr(native_namespace, name) is changed

        setattr(sc, name, original)
        assignment_count += 1
        assert getattr(sc, name) is original
        assert getattr(native_namespace, name) is original
        assert wildcard_snapshot[name] is original
        assert direct_snapshot[name] is original
finally:
    for name, value in originals.items():
        setattr(native_namespace, name, value)
    if temporary_name in sc.__dict__:
        delattr(sc, temporary_name)

assert all(getattr(sc, name) is value for name, value in originals.items())
assert all(getattr(native_namespace, name) is value for name, value in originals.items())
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
print(
    "KERNEL_FLAG_MODULE_EDGE ordinary=identity,removed native_unchanged="
    f"{int(native_unchanged)} assignments={assignment_count} raw={raw_assignment_count} "
    f"invalid={invalid_assignment_count} snapshots=independent"
)
