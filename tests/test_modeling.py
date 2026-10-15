"""Validate SystemC modeling in isolated subprocesses."""

import subprocess
import sys

from .support import ROOT, run_example


def test_arbitrary_width_datatypes(tmp_path):
    """Verify runtime template instantiation, wide signals and bit/range access."""
    assert "DATATYPES_OK" in run_example(tmp_path, "tests/models/datatypes.py")


def test_native_namespaces_fixed_point_and_tlm(tmp_path):
    """Check exports plus native quantization and transaction payload semantics."""
    output = run_example(tmp_path, "tests/models/namespaces.py")
    assert "NAMESPACES_OK" in output


def test_modeling_datatype_and_vcd_matrix(tmp_path):
    """Check native channel updates, ports, datatype assignment and trace transitions."""
    output = run_example(tmp_path, "tests/models/modeling_matrix.py")
    assert "MODELING_MATRIX_OK" in output
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/modeling_matrix.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert [line for line in output.splitlines() if line.startswith("MATRIX_")] == [
        line for line in native.stdout.splitlines() if line.startswith("MATRIX_")
    ]
    python_trace = (tmp_path / "matrix.vcd").read_text()
    native_trace = (tmp_path / "native_matrix.vcd").read_text()
    assert python_trace[python_trace.index("$timescale") :] == native_trace[native_trace.index("$timescale") :]


def test_required_port_binding_diagnostic(tmp_path):
    """Unbound required input reports its hierarchical name without native abort."""
    assert "BINDING_ERROR_OK" in run_example(tmp_path, "tests/models/binding_error.py")


def test_datatype_mutation_and_typed_ports(tmp_path):
    """Verify overflow, bit/range writes, conversion and arbitrary typed port copies."""
    assert "DATATYPE_OPERATIONS_OK" in run_example(tmp_path, "tests/models/datatype_operations.py")


def test_trace_setup_and_output_failure_cleanup(tmp_path):
    """Trace setup/output errors preserve diagnostics and permit a subsequent valid trace."""
    assert "TRACE_FAILURE_OK" in run_example(tmp_path, "tests/models/trace_failure.py")


def test_hierarchy_children_and_status_helpers(tmp_path):
    """Native hierarchy paths and helper output preserve child module ownership."""
    assert "HIERARCHY_CHECKS_OK" in run_example(tmp_path, "tests/models/hierarchy_checks.py")


def test_version_time_registry_and_introspection_apis(tmp_path):
    """Check post-2.3.1 time, registry, RTTI, never-event and initialized-signal behavior."""
    assert "VERSION_APIS_OK" in run_example(tmp_path, "tests/models/version_apis.py")


def test_event_queue_and_resolved_signal_writers(tmp_path):
    """Queued duplicate notifications and conflicting resolved writers preserve native semantics."""
    assert "CHANNEL_EXTENSIONS_OK" in run_example(tmp_path, "tests/models/channel_extensions.py")


def test_native_vector_and_attribute_ownership(tmp_path):
    """Native vector names/values and typed attributes follow object ownership."""
    assert "VECTOR_ATTRIBUTES_OK" in run_example(tmp_path, "tests/models/vector_attributes.py")


def test_extended_native_trace_formats_and_values(tmp_path):
    """Inspect WIF and VCD output for resolved, fixed, time and event values."""
    assert "TRACE_EXTENSIONS_OK" in run_example(tmp_path, "tests/models/trace_extensions.py")


def test_fifo_resolved_ports_and_writer_policy_templates(tmp_path):
    """Check bounded FIFO port traffic, resolved port values and policy instantiation."""
    assert "PORT_EXTENSIONS_OK" in run_example(tmp_path, "tests/models/port_extensions.py")


def test_native_writer_contention_reports(tmp_path):
    """One-writer rejects both conflicts; many-writer permits later-delta writes."""
    assert "WRITER_POLICIES_OK" in run_example(tmp_path, "tests/models/writer_policies.py")


def test_concat_dynamic_widths_and_fixed_fast_arithmetic(tmp_path):
    """Assert writable concat, arbitrary dynamic widths and fixed arithmetic."""
    assert "DATATYPE_EXTENSIONS_OK" in run_example(tmp_path, "tests/models/datatype_extensions.py")


def test_datatype_extensions_native_reference(tmp_path):
    """Run native assertions for concatenation and dynamic/fixed arithmetic."""
    result = subprocess.run(
        [sys.executable, "-m", "amal.eda.systemc._runner", str(ROOT / "tests/native/datatype_extensions.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "DATATYPE_EXTENSIONS_OK" in result.stdout


def test_channel_extensions_native_reference(tmp_path):
    """Confirm queue duplicates/cancellation and resolved contention natively."""
    result = subprocess.run(
        [sys.executable, "-m", "amal.eda.systemc._runner", str(ROOT / "tests/native/channel_extensions.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "CHANNEL_EXTENSIONS_OK" in result.stdout


def test_mutex_and_semaphore_contention(tmp_path):
    """Check blocked lock wakeup times, ordering and semaphore count."""
    assert "LOCK_CONTENTION_OK" in run_example(tmp_path, "tests/models/lock_contention.py")
