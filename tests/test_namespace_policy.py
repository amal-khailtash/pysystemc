"""Check namespace presentation policy without importing cppyy or SystemC."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path


def _load_namespace_module():
    """Load only the policy source so these tests stay native-free."""
    source = Path(__file__).resolve().parents[1] / "src/amal/eda/systemc/_namespace.py"
    spec = spec_from_file_location("_pysystemc_namespace_policy_test", source)
    assert spec is not None and spec.loader is not None
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class NativeNamespace:
    """Offer selected fake cppyy names and track seeded-name lookups."""

    def __init__(self, namespace, names=(), values=None):
        """Create a namespace view with explicit reflected and seeded values.

        Args:
            namespace: Exact qualified namespace key used by presentation policy.
            names: Names reported by ``dir`` on this fake native namespace.
            values: Resolvable namespace members, including optional seed entries.
        """
        self.__cpp_name__ = namespace
        self._names = set(names)
        self._values = {} if values is None else dict(values)
        self.lookups = []

    def __dir__(self):
        """Return only the test's reflected native spellings."""
        return sorted(self._names)

    def __getattr__(self, name):
        """Return a configured fake proxy or mimic an unresolved Cling seed."""
        self.lookups.append(name)
        try:
            return self._values[name]
        except KeyError as error:
            raise AttributeError(name) from error


def test_policy_excludes_exact_bridge_spellings_and_preserves_native_aliases():
    """Use the source registry without hiding unfamiliar prefix lookalikes."""
    policy = _load_namespace_module()

    expected_candidates = set(
        """
        PythonFifoChannel PythonInterface PythonPrimChannel PythonReadInterface PythonSignalChannel
        dispatch_python dispatch_report python_argv_at python_assign_event_expression python_blocking_result
        python_bound python_edge_wait python_entry_callback_error python_event_notify_delayed
        python_event_list_busy python_event_list_busy_access
        python_wait_expression_list_deletion_count python_next_trigger_expression_list_deletion_count
        python_event_notify_delayed_time python_event_notify_delayed_units python_export_access
        python_fifo_read python_fifo_read_output python_fifo_stream python_fifo_write
        python_guarded_halt_operation python_guarded_operation python_halt python_join_wait python_kill
        python_member_edge python_member_next_trigger python_member_scheduler python_member_timeout
        python_member_wait python_mutex_lock python_native_blocking python_native_blocking_traits
        python_next_trigger python_placeholder python_public_stream python_read_resolved
        python_report_handler python_reset python_result_functor python_scoped_lock python_semaphore_wait
        python_spawn_result python_start python_stream_access python_suspend python_throw_helper
        python_time_modulo python_timed_out python_unwind python_wait restore_report_handler
        set_python_report_handler spawn_callback trace_signal
        """.split()
    )
    registry = policy._BRIDGE_EXCLUSIONS["sc_core"]
    assert set(registry) == expected_candidates | {"add_trace_internal"}

    for name in expected_candidates | {"add_trace_internal"}:
        assert not policy.is_public_name("sc_core", name)
    assert policy.is_public_name("sc_core", "ScModule")
    assert policy.is_public_name("sc_core", "sc_module")
    assert policy.is_public_name("sc_core", "sc_placeholder")
    assert policy.is_public_name("sc_core", "CFT")
    assert policy.is_public_name("sc_core", "sc_plist_map_fn")
    assert policy.is_public_name("sc_core", "sc_report_handler_proc")
    for name in ("sc_argc", "sc_argv", "sc_elab_and_sim", "sc_in_action"):
        assert policy.is_public_name("sc_core", name)

    for name in ("python_user_extension", "PythonWidget", "sc_extension", "custom_vendor_type"):
        assert policy.is_public_name("sc_core", name)
    assert not policy.is_public_name("sc_core", "__cpp_name__")
    assert not policy.is_public_name("sc_core", "print")

    assert registry["python_wait"] == (
        "src/amal/eda/systemc/sc_core/sc_module.cpp:657",
        "process wait bridge helper",
    )
    assert registry["python_placeholder"][1] == "internal target of sc_placeholder alias"
    assert registry["add_trace_internal"][0].startswith("submodules/systemc/src/sysc/communication/sc_signal_ports.h:")


def test_public_names_keeps_custom_prefixes_and_skips_unresolved_seeds():
    """Resolve only available reflected/seed names while preserving native openness."""
    policy = _load_namespace_module()
    namespace = NativeNamespace(
        "tlm_utils",
        names=("python_user_type", "PythonWidget", "sc_custom_channel", "simple_target_socket"),
        values={
            "python_user_type": object(),
            "PythonWidget": object(),
            "sc_custom_channel": object(),
            "simple_target_socket": object(),
        },
    )

    names = policy.public_names(namespace)

    assert names == ["PythonWidget", "python_user_type", "sc_custom_channel", "simple_target_socket"]
    assert "tlm_quantumkeeper" in namespace.lookups
    assert "peq_with_cb_and_phase" in namespace.lookups
    assert "multi_passthrough_target_socket" in namespace.lookups
    assert "simple_target_socket" in namespace.lookups


def test_presented_names_includes_explicit_adapters_only_from_module_globals():
    """Union native names with registered adapter names, not imported globals."""
    policy = _load_namespace_module()
    native_value = object()
    namespace = NativeNamespace(
        "sc_dt",
        names=("sc_uint", "python_custom_native_name", "python_wait", "_ProxyMetadata", "print"),
        values={"sc_uint": native_value, "python_custom_native_name": native_value},
    )
    module_globals = {
        "concat": lambda: None,
        "sc_context": object(),
        "unrelated_module": object(),
        "__all__": ["concat", "sc_context", "python_wait"],
    }

    names = policy.presented_names(module_globals, namespace, adapters=("concat", "sc_context", "python_wait"))

    assert names == ["concat", "python_custom_native_name", "sc_context", "sc_uint"]
    documentation = policy.namespace_help(module_globals, namespace)
    assert "concat" in documentation and "sc_context" in documentation
    assert "unrelated_module" not in documentation
    assert "python_wait" not in documentation


def test_public_template_seeds_cover_reviewed_missing_namespaces():
    """Keep adapter aliases and native utility templates in the seed inventory."""
    policy = _load_namespace_module()
    sc_context = NativeNamespace("sc_dt", values={"sc_context": object()})
    analysis_fifo = NativeNamespace("tlm", values={"tlm_analysis_fifo": object()})
    assert policy.public_names(sc_context) == ["sc_context"]
    assert policy.public_names(analysis_fifo) == ["tlm_analysis_fifo"]
    for name in (
        "simple_initiator_socket",
        "simple_target_socket",
        "multi_passthrough_target_socket",
        "peq_with_get",
        "peq_with_cb_and_phase",
        "tlm_quantumkeeper",
    ):
        assert name in policy._PUBLIC_NAME_SEEDS["tlm_utils"]
