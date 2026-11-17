"""Regress the exact public blocking TLM peek output overload."""

import pytest

from .support import run_example


@pytest.mark.parametrize("scenario", ("kill", "reset", "normal"))
def test_exact_tlm_peek_unwind(tmp_path, scenario):
    """Check peek output copying and native wait unwind in isolated kernels."""
    working = tmp_path / scenario
    working.mkdir()
    output = run_example(working, "tests/models/exact_tlm_peek_unwind.py", (scenario,))
    assert f"EXACT_TLM_PEEK_UNWIND_OK scenario={scenario}" in output
