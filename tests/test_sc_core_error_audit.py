"""Audit invalid guarded member-wait arguments and native process restrictions."""

import pytest

from .support import run_example


@pytest.mark.parametrize("owner", ("module", "channel"))
def test_member_wait_method_process_error(tmp_path, owner):
    """Assert each owner reports wait-not-allowed and permits callback recovery."""
    output = run_example(tmp_path, "tests/models/sc_core_error_audit.py", (owner, "method"))
    assert f"MEMBER_WAIT_METHOD_ERROR_OK {owner} report=519 attempts=1 completed=1" in output


@pytest.mark.parametrize("owner", ("module", "channel"))
def test_member_wait_host_type_error_does_not_start_kernel(tmp_path, owner):
    """Assert invalid host-side wait arguments leave the kernel in elaboration."""
    output = run_example(tmp_path, "tests/models/sc_core_error_audit.py", (owner, "type"))
    assert f"MEMBER_WAIT_TYPE_ERROR_OK {owner} status=" in output
