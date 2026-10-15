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


def test_python_custom_interface_native_port_dispatch(tmp_path):
    """Verify package-declared interfaces dispatch through native templated ports."""
    assert "CUSTOM_INTERFACE_OK" in run_example(tmp_path, "tests/models/custom_interface.py")


def test_python_channel_interface_blocking_dispatch(tmp_path):
    """Assert multiple native bases and guarded waits through an interface port."""
    assert "INTERFACE_CHANNEL_OK" in run_example(tmp_path, "tests/models/interface_channel.py")


def test_native_mutex_interface_python_implementation(tmp_path):
    """Check source-native mutex virtual methods and blocking port dispatch."""
    assert "NATIVE_MUTEX_INTERFACE_OK" in run_example(tmp_path, "tests/models/native_mutex_interface.py")


def test_native_signal_interface_python_reference_returns(tmp_path):
    """Verify native signal interface values and reference-event sensitivity."""
    assert "NATIVE_SIGNAL_INTERFACE_OK" in run_example(tmp_path, "tests/models/native_signal_interface.py")


def test_native_semaphore_interface_python_implementation(tmp_path):
    """Check source-native semaphore methods, blocked wakeup and token accounting."""
    assert "NATIVE_SEMAPHORE_INTERFACE_OK" in run_example(tmp_path, "tests/models/native_semaphore_interface.py")


def test_callback_signal_interface_stable_native_references(tmp_path):
    """Check computed Python read values and explicit interface notifications."""
    assert "CALLBACK_READ_INTERFACE_OK" in run_example(tmp_path, "tests/models/callback_read_interface.py")


def test_callback_bool_logic_reference_and_edge_interfaces(tmp_path):
    """Assert native bool/logic edge methods and sampled update timestamps."""
    assert "CALLBACK_EDGE_INTERFACES_OK" in run_example(tmp_path, "tests/models/callback_edge_interfaces.py")


def test_native_fifo_interface_reference_overloads_and_events(tmp_path):
    """Check native FIFO interfaces through Python-owned storage and native ports."""
    assert "NATIVE_FIFO_INTERFACE_OK" in run_example(tmp_path, "tests/models/native_fifo_interface.py")


def test_native_event_queue_interface_python_overloads(tmp_path):
    """Check native-port notification overloads, duplicate deltas and cancellation."""
    assert "NATIVE_EVENT_QUEUE_INTERFACE_OK" in run_example(tmp_path, "tests/models/native_event_queue_interface.py")


def test_native_tlm_blocking_interface_python_reference_dispatch(tmp_path):
    """Verify native TLM payload/time references and blocking Python service calls."""
    assert "NATIVE_TLM_BLOCKING_INTERFACE_OK" in run_example(tmp_path, "tests/models/native_tlm_blocking_interface.py")


def test_native_tlm_combined_forward_backward_python_interfaces(tmp_path):
    """Check phase/delay references, sync enums, debug and DMI invalidation callbacks."""
    assert "NATIVE_TLM_COMBINED_INTERFACES_OK" in run_example(
        tmp_path, "tests/models/native_tlm_combined_interfaces.py"
    )


def test_structural_tlm2_combined_and_standalone_interfaces(tmp_path):
    """Check ordinary callbacks, native references, debug limits and blocking waits."""
    assert "STRUCTURAL_TLM2_INTERFACES_OK" in run_example(tmp_path, "tests/models/python_tlm2_interfaces.py")


def test_structural_tlm_master_slave_interfaces(tmp_path):
    """Verify all six compound families preserve distinct native input/output types."""
    assert "STRUCTURAL_TLM_MASTER_SLAVE_OK" in run_example(tmp_path, "tests/models/python_tlm_master_slave.py")


def test_structural_tlm_fifo_debug_and_configuration(tmp_path):
    """Verify indexed debug access, unchanged failure outputs and native defaults."""
    assert "STRUCTURAL_TLM_FIFO_DEBUG_OK" in run_example(tmp_path, "tests/models/python_tlm_fifo_debug.py")


def test_structural_tlm_fifo_compound_interfaces(tmp_path):
    """Verify compound FIFO indexed/ordinary peek and shared blocking queue state."""
    assert "STRUCTURAL_TLM_FIFO_COMPOUND_OK" in run_example(tmp_path, "tests/models/python_tlm_fifo_compound.py")


def test_native_tlm_quantumkeeper_and_payload_queue(tmp_path):
    """Verify quantum waits, PEQ due times, cancellation and payload identity."""
    assert "TLM_UTILITIES_OK" in run_example(tmp_path, "tests/models/tlm_utilities.py")


def test_tlm_utilities_match_native_reference(tmp_path):
    """Compare all quantum/queue timing records with independent native processes."""
    python_output = run_example(tmp_path, "tests/models/tlm_utilities.py")
    result = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/tlm_utilities.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    native_output = result.stdout
    assert [line for line in python_output.splitlines() if line.startswith("UTILITY ")] == [
        line for line in native_output.splitlines() if line.startswith("UTILITY ")
    ]


def test_tlm_quantumkeeper_reset_and_kill_cleanup(tmp_path):
    """Verify both quantum wait APIs restart and execute cleanup before termination."""
    assert "QUANTUM_RESET_OK" in run_example(tmp_path, "tests/models/tlm_quantum_reset.py")


def test_native_callback_payload_event_queue(tmp_path):
    """Verify retained callbacks, copied phases, payload mutation and timed delivery."""
    assert "CALLBACK_PEQ_OK" in run_example(tmp_path, "tests/models/tlm_callback_peq.py")


def test_callback_payload_queue_custom_protocol_and_errors(tmp_path):
    """Verify wide custom payload identity, primitive phases and callback exceptions."""
    assert "CUSTOM_CALLBACK_PEQ_OK" in run_example(tmp_path, "tests/models/tlm_callback_peq_custom.py")


def test_convenience_target_socket_python_callback(tmp_path):
    """Verify arbitrary width, retained blocking callback and borrowed transaction updates."""
    assert "CALLBACK_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_callback_socket.py")


def test_convenience_initiator_socket_python_callbacks(tmp_path):
    """Verify connected forward/backward convenience callbacks and 64-bit invalidation."""
    assert "CALLBACK_INITIATOR_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_callback_initiator.py")


def test_convenience_socket_custom_protocol(tmp_path):
    """Verify custom native payload identity, wide fields and forward/backward callbacks."""
    assert "CUSTOM_CALLBACK_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_callback_socket_custom.py")


def test_custom_socket_protocol_matches_native_reference(tmp_path):
    """Compare custom socket payload mutation and timing with upstream native sockets."""
    python_output = run_example(tmp_path, "tests/models/tlm_callback_socket_custom.py")
    result = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/tlm_socket_custom.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    expected = ["CUSTOM_SOCKET 21 23 0 2 ns"]
    assert [line for line in python_output.splitlines() if line.startswith("CUSTOM_SOCKET ")] == expected
    assert [line for line in result.stdout.splitlines() if line.startswith("CUSTOM_SOCKET ")] == expected


def test_multi_socket_custom_protocol_payloads(tmp_path):
    """Verify multi protocol specialization preserves native identity and 137-bit fields."""
    assert "CUSTOM_MULTI_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_multi_socket_custom.py")


def test_multi_socket_reset_and_kill_cleanup(tmp_path):
    """Verify both connections restart and clean up while indexed transport is suspended."""
    assert "MULTI_SOCKET_RESET_OK" in run_example(tmp_path, "tests/models/tlm_multi_socket_reset.py")


def test_multi_to_multi_socket_connection_mapping(tmp_path):
    """Verify direct multi binding preserves distinct target and initiator indices."""
    assert "MULTI_TO_MULTI_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_multi_to_multi.py")


def test_multi_initiator_socket_connection_callbacks(tmp_path):
    """Verify indexed forward waits and connection-indexed backward transport/invalidation."""
    assert "MULTI_INITIATOR_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_multi_initiator.py")


def test_multi_target_socket_connection_callbacks(tmp_path):
    """Verify multi target IDs, borrowed state, retained owners and independent waits."""
    assert "MULTI_TARGET_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_multi_target.py")


def test_convenience_socket_reset_and_kill_cleanup(tmp_path):
    """Verify automatic GIL release and callback cleanup through reset/kill."""
    assert "CALLBACK_SOCKET_RESET_OK" in run_example(tmp_path, "tests/models/tlm_callback_socket_reset.py")


def test_convenience_socket_indexed_forward_access(tmp_path):
    """Verify indexed native interface access configures blocking GIL release."""
    assert "CALLBACK_INDEXED_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_callback_indexed.py")


def test_convenience_socket_callback_exception_state(tmp_path):
    """Verify callback errors preserve borrowed mutations and stop at service time."""
    assert "CALLBACK_SOCKET_ERROR_OK" in run_example(tmp_path, "tests/models/tlm_callback_socket_error.py")


def test_tagged_target_socket_blocking_callback(tmp_path):
    """Verify tagged native registration supplies user ID and borrowed transport state."""
    assert "TAGGED_TARGET_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_tagged_target.py")


def test_tagged_initiator_socket_backward_callbacks(tmp_path):
    """Verify independent tagged response/invalidation IDs and native references."""
    assert "TAGGED_INITIATOR_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_tagged_initiator.py")


def test_hierarchical_convenience_socket_binding(tmp_path):
    """Verify parent port/export resolution and callback dispatch after elaboration."""
    assert "HIERARCHICAL_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_hierarchical_socket.py")


def test_hierarchical_target_socket_binding(tmp_path):
    """Verify target parent export and backward port resolve to Python callbacks."""
    assert "HIERARCHICAL_TARGET_SOCKET_OK" in run_example(tmp_path, "tests/models/tlm_hierarchical_target.py")


def test_native_tlm_dmi_pointer_access_and_lifetime(tmp_path):
    """Verify granted DMI reads/writes survive GC and preserve native memory identity."""
    assert "NATIVE_TLM_DMI_INTERFACE_OK" in run_example(tmp_path, "tests/models/native_tlm_dmi_interface.py")


def test_native_tlm_socket_forward_backward_binding(tmp_path):
    """Check native sockets bind Python interfaces and preserve DMI/phase dispatch."""
    assert "NATIVE_TLM_SOCKETS_OK" in run_example(tmp_path, "tests/models/native_tlm_sockets.py")


def test_native_tlm_memory_manager_reference_count_and_recycle(tmp_path):
    """Verify native reference counting invokes the retained Python manager at zero."""
    assert "NATIVE_TLM_MEMORY_MANAGER_OK" in run_example(tmp_path, "tests/models/native_tlm_memory_manager.py")


def test_native_tlm1_blocking_and_analysis_python_dispatch(tmp_path):
    """Verify TLM-1 blocking calls and analysis broadcast preserve values and time."""
    assert "NATIVE_TLM1_BLOCKING_INTERFACES_OK" in run_example(
        tmp_path, "tests/models/native_tlm1_blocking_interfaces.py"
    )


def test_native_tlm1_transport_and_peek_reference_overloads(tmp_path):
    """Verify TLM-1 request/response and peek preserve native output-reference semantics."""
    assert "NATIVE_TLM1_TRANSPORT_PEEK_OK" in run_example(tmp_path, "tests/models/native_tlm1_transport_peek.py")


def test_native_tlm1_nonblocking_put_readiness_event(tmp_path):
    """Verify nonblocking acceptance and stable native event wakeup through a port."""
    assert "NATIVE_TLM1_NONBLOCKING_PUT_OK" in run_example(tmp_path, "tests/models/native_tlm1_nonblocking_put.py")


def test_native_tlm1_nonblocking_get_output_and_readiness(tmp_path):
    """Verify failed get preserves output and successful get consumes one native value."""
    assert "NATIVE_TLM1_NONBLOCKING_GET_OK" in run_example(tmp_path, "tests/models/native_tlm1_nonblocking_get.py")


def test_native_tlm1_nonblocking_peek_output_and_readiness(tmp_path):
    """Verify failed peek preserves output and repeated peeks do not consume samples."""
    assert "NATIVE_TLM1_NONBLOCKING_PEEK_OK" in run_example(tmp_path, "tests/models/native_tlm1_nonblocking_peek.py")


def test_native_tlm1_combined_nonblocking_get_peek(tmp_path):
    """Verify combined and base-port dispatch, readiness and consumption agree."""
    assert "NATIVE_TLM1_NONBLOCKING_GET_PEEK_OK" in run_example(
        tmp_path, "tests/models/native_tlm1_nonblocking_get_peek.py"
    )


def test_structural_python_interface_binding(tmp_path):
    """Verify ordinary methods, automatic GIL handling and retained implementations."""
    assert "PYTHON_INTERFACE_BINDING_OK" in run_example(tmp_path, "tests/models/python_interface_binding.py")


def test_protocol_interface_annotation_inference(tmp_path):
    """Verify inherited signatures, native values and ambiguous annotation rejection."""
    assert "INTERFACE_ANNOTATIONS_OK" in run_example(tmp_path, "tests/models/interface_annotations.py")


def test_structural_nonblocking_interface_binding(tmp_path):
    """Verify ordinary readiness/value callbacks dispatch through native base ports."""
    assert "PYTHON_NONBLOCKING_BINDING_OK" in run_example(tmp_path, "tests/models/python_nonblocking_binding.py")


def test_structural_combined_get_interface(tmp_path):
    """Verify shared blocking/nonblocking queue access and native output overloads."""
    assert "PYTHON_COMBINED_GET_OK" in run_example(tmp_path, "tests/models/python_combined_get.py")


def test_structural_combined_put_interface(tmp_path):
    """Verify shared capacity, blocking dispatch and native readiness events."""
    assert "PYTHON_COMBINED_PUT_OK" in run_example(tmp_path, "tests/models/python_combined_put.py")


def test_structural_combined_peek_interface(tmp_path):
    """Verify blocking/native output overloads preserve nonconsuming access."""
    assert "PYTHON_COMBINED_PEEK_OK" in run_example(tmp_path, "tests/models/python_combined_peek.py")


def test_structural_full_get_peek_interface(tmp_path):
    """Verify consuming/nonconsuming dispatch through full and native base views."""
    assert "PYTHON_COMBINED_GET_PEEK_OK" in run_example(tmp_path, "tests/models/python_combined_get_peek.py")


def test_structural_blocking_get_peek_interface(tmp_path):
    """Verify direct blocking-only structural binding and both output overloads."""
    assert "PYTHON_BLOCKING_GET_PEEK_OK" in run_example(tmp_path, "tests/models/python_blocking_get_peek.py")


def test_structural_tlm_arbitrary_width_value_copies(tmp_path):
    """Verify high-bit preservation, independent copies and wide output assignments."""
    assert "PYTHON_TLM_WIDE_VALUES_OK" in run_example(tmp_path, "tests/models/python_tlm_wide_values.py")
