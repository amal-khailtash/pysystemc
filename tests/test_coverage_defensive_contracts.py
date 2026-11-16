"""Validate intentionally injected reflection and cppyy defensive contracts."""

from .support import run_example


def test_coverage_defensive_contracts_run_in_fresh_process(tmp_path):
    """Run all seven classified statements without claiming native support."""
    output = run_example(tmp_path, "tests/models/coverage_defensive_contracts.py")
    assert (
        "COVERAGE_DEFENSIVE_CONTRACTS_OK statements=7 "
        "enum=helper-robustness hash=reflected-fixture "
        "alias=alternate-metadata pointer=backend-fault-injection"
    ) in output
