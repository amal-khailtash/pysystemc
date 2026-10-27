"""Run fresh-process ownership probes for selected F6 process contracts."""

import pytest

from .support import run_example


@pytest.mark.parametrize(
    ("scenario", "expected"),
    (
        ("processes", "F6_PROCESS_REGISTRATION cthread|method|thread released_after_module=3"),
        ("registration_failures", "F6_PROCESS_REGISTRATION_FAILURES method thread cthread unchanged=1"),
        ("unwind", "F6_PROCESS_RESET_KILL"),
        ("spawn", "F6_TYPED_SPAWN 41 source=97 ctypes=14 callback_retained=1"),
        ("binding", "F6_BINDING_OWNERS bound_calls=2 port_interface_live=1 hierarchy=1"),
        ("entry_success", "F6_SC_MAIN_BRIDGE success status=23 owned_refs=5 released=1"),
        ("entry_list_failure", "F6_SC_MAIN_BRIDGE list_failure status=-1 owned_refs=1 released=1"),
        ("entry_decode_failure", "F6_SC_MAIN_BRIDGE decode_failure status=-1 owned_refs=3 released=1"),
        ("entry_null_slot", "F6_SC_MAIN_BRIDGE null_slot status=-1 owned_refs=3 released=1"),
        ("entry_cpp_unwind", "F6_SC_MAIN_BRIDGE cpp_unwind status=-2 owned_refs=3 released=1"),
        ("entry_callback_exception", "F6_SC_MAIN_BRIDGE callback_exception status=-1 owned_refs=4 released=1"),
        ("entry_bad_result", "F6_SC_MAIN_BRIDGE bad_result status=-1 owned_refs=5 released=1"),
        ("carrier_held", "F6_EXCEPTION_CARRIER held copy_ctor=1 make_exception_ptr=1 decref=1 gil=verified"),
        ("carrier_worker", "F6_EXCEPTION_CARRIER worker copy_ctor=1 make_exception_ptr=1 decref=1 gil=verified"),
        ("carrier_propagation", "F6_EXCEPTION_PROPAGATION same_instance=1 released=1"),
        ("carrier_direct_report", "F6_EXCEPTION_CARRIER held copy_ctor=1 make_exception_ptr=1 decref=1 gil=verified"),
        ("carrier_caught_report", "F6_EXCEPTION_CARRIER held copy_ctor=1 make_exception_ptr=1 decref=1 gil=verified"),
        ("carrier_deprecation", "F6_EXCEPTION_CARRIER held copy_ctor=1 make_exception_ptr=1 decref=1 gil=verified"),
        ("identity_caught_report", "F6_EXCEPTION_IDENTITY caught_report same_instance=1 released=1 calls=1"),
        ("identity_deprecation", "F6_EXCEPTION_IDENTITY deprecation same_instance=1 released=1 calls=2"),
    ),
)
def test_f6_process_ownership(tmp_path, scenario, expected):
    """Verify one isolated ownership boundary and its observable evidence."""
    output = run_example(tmp_path, "tests/models/f6_process_ownership.py", (scenario,))
    assert expected in output


@pytest.mark.parametrize("kind", ("method", "thread", "cthread"))
def test_f6_process_callback_failure(tmp_path, kind):
    """Verify translated exceptions from each module-registered process kind."""
    output = run_example(tmp_path, "tests/models/f6_process_ownership.py", ("process_failure", kind))
    assert f"F6_PROCESS_CALLBACK_FAILURE {kind} entered=1 retained=1" in output
