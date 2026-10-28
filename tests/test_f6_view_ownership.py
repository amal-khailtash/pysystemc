"""Verify native-view ownership boundaries in independent Python processes."""

from .support import run_example


def run_scope(tmp_path, scope):
    """Run one ownership scope in its own interpreter and kernel process."""
    working = tmp_path / scope
    working.mkdir()
    return run_example(working, "tests/models/f6_view_ownership.py", (scope,))


def test_f6_16_sc_dt_proxy_owner_and_rollback(tmp_path):
    """Check copied and borrowed proxy directions plus fixed owner tokens."""
    output = run_scope(tmp_path, "dt")
    assert "F6_16_VIEW_OWNERSHIP_OK integers=4 vectors=2 fixed=2 concat=mutable/const rollback=1" in output


def test_f6_17_context_copy_and_last_owner_release(tmp_path):
    """Check all three context families and conversion-failure rollback."""
    output = run_scope(tmp_path, "contexts")
    assert "F6_17_CONTEXT_OWNERSHIP_OK forms=fxtype,length,cast copy=1 rollback=1" in output


def test_f6_19_vector_element_and_assembly_lifetimes(tmp_path):
    """Check returned elements, borrowed assembly, creator and rollback edges."""
    output = run_scope(tmp_path, "vectors")
    assert "F6_19_VECTOR_OWNERSHIP_OK creator=sync returned_proxies=2 assembly=retains rollback_destroyed=1" in output


def test_f6_22_trace_sources_and_registration_rollback(tmp_path):
    """Check scalar/vector/event declarations, transitions and trace cleanup."""
    output = run_scope(tmp_path, "trace")
    assert "F6_22_TRACE_OWNERSHIP_OK sources=scalar,vector,event failures=enum_rollback native_file=1" in output
    vcd = (tmp_path / "trace" / "f6_view_ownership.vcd").read_text()
    signal_id = next(line.split()[3] for line in vcd.splitlines() if line.startswith("$var") and "f6_signal" in line)
    vector_id = next(line.split()[3] for line in vcd.splitlines() if line.startswith("$var") and "f6_vector" in line)
    event_id = next(line.split()[3] for line in vcd.splitlines() if line.startswith("$var") and "f6_event" in line)
    assert f"1{signal_id}" in vcd, "VCD omitted the scalar signal transition"
    assert f"b10101010 {vector_id}" in vcd, "VCD omitted the vector signal transition"
    assert f"1{event_id}" in vcd, "VCD omitted the event transition"


def test_f6_24_event_finder_expression_and_sensitivity_lifetimes(tmp_path):
    """Check cache proxy retention, failed expression composition and dispatch."""
    output = run_scope(tmp_path, "events")
    assert "F6_24_EVENT_OWNERSHIP_OK cache=retained_until_finder direct_finder=gc handle=separate_copy" in output
    assert "events=borrowed_failed_composition=1 dispatch=1" in output
