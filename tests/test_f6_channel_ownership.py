"""Run bounded channel ownership audits in fresh SystemC subprocesses."""

import pytest

from .support import run_example


@pytest.mark.parametrize(
    ("scenario", "marker"),
    (
        ("structural", "F6_STRUCTURAL_OWNER_OK reset=2ns kill=3ns released=1"),
        ("fifo-frame", "F6_FIFO_FRAME_OK get=peek=put=value=output=tag=error=release"),
        ("quantum", "F6_QUANTUM_OWNER_OK reset=2ns kill=3ns callback=released"),
        ("peq", "F6_PEQ_OWNER_OK callback=retained payload=borrowed phase=copy delay=copy cancel=released"),
    ),
)
def test_f6_channel_ownership_lifetimes(tmp_path, scenario, marker):
    """Assert callback retention, safe release, and native unwind in isolation."""
    output = run_example(tmp_path, "tests/models/f6_channel_ownership.py", (scenario,))
    assert marker in output
