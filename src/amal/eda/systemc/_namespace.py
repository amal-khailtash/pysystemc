"""Discover public native namespace members without constraining templates."""

_BRIDGE_EXCLUSIONS = {
    "sc_core": {
        # These exact declarations back first-party cppyy bindings. Their source
        # paths are first-party provenance, not a spelling-based prefix rule.
        "PythonFifoChannel": ("src/amal/eda/systemc/sc_core/sc_module.cpp:111", "bridge channel implementation"),
        "PythonInterface": ("src/amal/eda/systemc/sc_core/sc_module.cpp:33", "bridge interface implementation"),
        "PythonPrimChannel": ("src/amal/eda/systemc/sc_core/sc_module.cpp:45", "bridge channel implementation"),
        "PythonReadInterface": ("src/amal/eda/systemc/sc_core/sc_module.cpp:128", "bridge interface implementation"),
        "PythonSignalChannel": ("src/amal/eda/systemc/sc_core/sc_module.cpp:94", "bridge channel implementation"),
        "dispatch_python": ("src/amal/eda/systemc/sc_core/sc_module.cpp:632", "Python process dispatch bridge"),
        "dispatch_report": ("src/amal/eda/systemc/sc_core/sc_module.cpp:785", "Python report dispatch bridge"),
        "guarded_wait": ("src/amal/eda/systemc/sc_core/sc_module.cpp:507", "guarded blocking bridge helper"),
        "python_argv_at": ("src/amal/eda/systemc/sc_core/sc_module.cpp:173", "native entry bridge helper"),
        "python_assign_event_expression": (
            "src/amal/eda/systemc/sc_core/event_lists.cpp:36",
            "event-list bridge helper",
        ),
        "python_blocking_result": (
            "src/amal/eda/systemc/sc_core/native_blocking.h:42",
            "blocking bridge result carrier",
        ),
        "python_bound": ("src/amal/eda/systemc/sc_core/bind.cpp:42", "binding bridge helper"),
        "python_edge_wait": ("src/amal/eda/systemc/sc_core/sc_module.cpp:332", "process wait bridge helper"),
        "python_event_list_busy_access": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:207",
            "read-only native event-list ownership probe type",
        ),
        "python_event_list_busy": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:218",
            "read-only native event-list ownership bridge helper",
        ),
        "python_wait_expression_list_deletion_count": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:293",
            "read-only wait-expression ownership validation counter",
        ),
        "python_next_trigger_expression_list_deletion_count": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:299",
            "read-only trigger-expression ownership validation counter",
        ),
        "python_entry_callback_error": (
            "src/amal/eda/systemc/sc_core/native_entry.cpp:77",
            "native entry callback error carrier",
        ),
        "python_event_notify_delayed": (
            "src/amal/eda/systemc/sc_core/event_lists.cpp:8",
            "event notification bridge helper",
        ),
        "python_event_notify_delayed_time": (
            "src/amal/eda/systemc/sc_core/event_lists.cpp:15",
            "event notification bridge helper",
        ),
        "python_event_notify_delayed_units": (
            "src/amal/eda/systemc/sc_core/event_lists.cpp:22",
            "event notification bridge helper",
        ),
        "python_export_access": ("src/amal/eda/systemc/sc_core/sc_module.cpp:497", "export bridge helper"),
        "python_fifo_read": ("src/amal/eda/systemc/sc_core/sc_module.cpp:372", "FIFO bridge helper"),
        "python_fifo_read_output": ("src/amal/eda/systemc/sc_core/sc_module.cpp:391", "FIFO bridge helper"),
        "python_fifo_stream": ("src/amal/eda/systemc/sc_core/sc_module.cpp:450", "FIFO stream bridge helper"),
        "python_fifo_write": ("src/amal/eda/systemc/sc_core/sc_module.cpp:406", "FIFO bridge helper"),
        "python_guarded_halt_operation": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:213",
            "guarded kernel operation bridge",
        ),
        "python_guarded_operation": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:191",
            "guarded kernel operation bridge",
        ),
        "python_halt": ("src/amal/eda/systemc/sc_core/sc_module.cpp:230", "process control bridge helper"),
        "python_join_wait": ("src/amal/eda/systemc/sc_core/sc_module.cpp:314", "process wait bridge helper"),
        "python_kill": ("src/amal/eda/systemc/sc_core/sc_module.cpp:537", "process control bridge helper"),
        "python_member_edge": ("src/amal/eda/systemc/sc_core/sc_module.cpp:1042", "module member bridge helper"),
        "python_member_next_trigger": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:1079",
            "module member bridge helper",
        ),
        "python_member_scheduler": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:1026",
            "module member bridge helper",
        ),
        "python_member_timeout": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:1094",
            "module member bridge helper",
        ),
        "python_member_wait": ("src/amal/eda/systemc/sc_core/sc_module.cpp:1064", "module member bridge helper"),
        "python_member_wait_call": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:1229",
            "module member wait overload bridge helper",
        ),
        "python_member_next_trigger_call": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:1299",
            "module member trigger overload bridge helper",
        ),
        "python_mutex_lock": ("src/amal/eda/systemc/sc_core/sc_module.cpp:415", "synchronization bridge helper"),
        "python_native_blocking": (
            "src/amal/eda/systemc/sc_core/native_blocking.h:76",
            "blocking bridge implementation",
        ),
        "python_native_blocking_traits": (
            "src/amal/eda/systemc/sc_core/native_blocking.h:18",
            "blocking bridge traits",
        ),
        "python_next_trigger": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:571",
            "process trigger bridge helper",
        ),
        "python_placeholder": ("src/amal/eda/systemc/sc_core/bind.cpp:25", "internal target of sc_placeholder alias"),
        "python_public_stream": ("src/amal/eda/systemc/sc_core/sc_module.cpp:474", "FIFO stream bridge implementation"),
        "python_read_resolved": ("src/amal/eda/systemc/sc_core/sc_module.cpp:149", "signal bridge helper"),
        "python_report_handler": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:780",
            "Python report callback bridge",
        ),
        "python_reset": ("src/amal/eda/systemc/sc_core/sc_module.cpp:545", "process control bridge helper"),
        "python_result_functor": (
            "src/amal/eda/systemc/sc_core/spawn_result.cpp:20",
            "spawn result bridge carrier",
        ),
        "python_scoped_lock": ("src/amal/eda/systemc/sc_core/sc_module.cpp:514", "synchronization bridge helper"),
        "python_semaphore_wait": ("src/amal/eda/systemc/sc_core/sc_module.cpp:524", "synchronization bridge helper"),
        "python_spawn_result": ("src/amal/eda/systemc/sc_core/spawn_result.cpp:44", "spawn result bridge helper"),
        "python_start": ("src/amal/eda/systemc/sc_core/sc_module.cpp:251", "kernel start bridge helper"),
        "python_stream_access": ("src/amal/eda/systemc/sc_core/sc_module.cpp:426", "FIFO stream bridge helper"),
        "python_suspend": ("src/amal/eda/systemc/sc_core/sc_module.cpp:560", "process control bridge helper"),
        "python_throw_helper": (
            "src/amal/eda/systemc/sc_core/process_exceptions.cpp:17",
            "process exception bridge helper",
        ),
        "python_time_modulo": ("src/amal/eda/systemc/sc_core/sc_module.cpp:161", "time conversion bridge helper"),
        "python_timed_out": ("src/amal/eda/systemc/sc_core/sc_module.cpp:240", "process state bridge helper"),
        "python_unwind": ("src/amal/eda/systemc/sc_core/sc_module.cpp:185", "process unwind bridge helper"),
        "python_wait": ("src/amal/eda/systemc/sc_core/sc_module.cpp:657", "process wait bridge helper"),
        "restore_report_handler": ("src/amal/eda/systemc/sc_core/sc_module.cpp:802", "Python report callback bridge"),
        "set_python_report_handler": (
            "src/amal/eda/systemc/sc_core/sc_module.cpp:793",
            "Python report callback bridge",
        ),
        "spawn_callback": ("src/amal/eda/systemc/sc_core/sc_module.cpp:814", "Python process callback bridge"),
        "trace_signal": ("src/amal/eda/systemc/sc_core/sc_module.cpp:833", "trace callback bridge helper"),
        # Vendored implementation member: an exact upstream exclusion, not a
        # prefix or suffix convention for native API names.
        "add_trace_internal": (
            "submodules/systemc/src/sysc/communication/sc_signal_ports.h:246",
            "vendored internal trace callback member",
        ),
    }
}


_PUBLIC_NAME_SEEDS = {
    "sc_core": (
        "CFT",
        "ScModule",
        "sc_argc",
        "sc_argv",
        "sc_attribute",
        "sc_attr_base",
        "sc_attr_cltn",
        "sc_buffer",
        "sc_elab_and_sim",
        "sc_event_and_list",
        "sc_event_finder",
        "sc_event_or_list",
        "sc_event_queue",
        "sc_export",
        "sc_fifo",
        "sc_in_action",
        "sc_in",
        "sc_in_resolved",
        "sc_in_rv",
        "sc_inout",
        "sc_inout_resolved",
        "sc_inout_rv",
        "sc_interface",
        "sc_join",
        "sc_module",
        "sc_mutex",
        "sc_out",
        "sc_out_resolved",
        "sc_out_rv",
        "sc_plist_map_fn",
        "sc_port",
        "sc_prim_channel",
        "sc_process_handle",
        "sc_report_handler",
        "sc_report_handler_proc",
        "sc_report",
        "sc_semaphore",
        "sc_signal",
        "sc_signal_resolved",
        "sc_signal_rv",
        "sc_spawn_options",
        "sc_time",
        "sc_trace_file",
        "sc_vector",
    ),
    "sc_dt": (
        "sc_bigint",
        "sc_biguint",
        "sc_bv_base",
        "sc_bv",
        "sc_context",
        "sc_fix_fast",
        "sc_fix",
        "sc_fixed_fast",
        "sc_fixed",
        "sc_fxcast_switch",
        "sc_fxnum",
        "sc_fxtype_params",
        "sc_fxval_fast",
        "sc_fxval",
        "sc_int_base",
        "sc_int",
        "sc_logic",
        "sc_lv_base",
        "sc_lv",
        "sc_signed",
        "sc_ufix_fast",
        "sc_ufix",
        "sc_ufixed_fast",
        "sc_ufixed",
        "sc_uint_base",
        "sc_uint",
        "sc_unsigned",
    ),
    "tlm": (
        "tlm_analysis_fifo",
        "tlm_analysis_if",
        "tlm_analysis_port",
        "tlm_base_protocol_types",
        "tlm_bw_transport_if",
        "tlm_dmi",
        "tlm_extension_base",
        "tlm_extension",
        "tlm_fifo",
        "tlm_fw_transport_if",
        "tlm_generic_payload",
        "tlm_get_if",
        "tlm_initiator_socket",
        "tlm_mm_interface",
        "tlm_peek_if",
        "tlm_phase",
        "tlm_put_if",
        "tlm_target_socket",
        "tlm_transport_if",
    ),
    # These exact public template declarations are supplied by tlm_utils
    # headers included by tlm_utils/__init__.py. They may be absent from dir()
    # until instantiated, so lookup remains resolution-checked below.
    "tlm_utils": (
        "multi_passthrough_initiator_socket",
        "multi_passthrough_initiator_socket_optional",
        "multi_passthrough_target_socket",
        "multi_passthrough_target_socket_optional",
        "peq_with_cb_and_phase",
        "peq_with_get",
        "passthrough_target_socket",
        "passthrough_target_socket_optional",
        "passthrough_target_socket_tagged",
        "passthrough_target_socket_tagged_optional",
        "simple_initiator_socket",
        "simple_initiator_socket_optional",
        "simple_initiator_socket_tagged",
        "simple_initiator_socket_tagged_optional",
        "simple_target_socket",
        "simple_target_socket_optional",
        "simple_target_socket_tagged",
        "simple_target_socket_tagged_optional",
        "tlm_quantumkeeper",
    ),
}


def _namespace_key(namespace):
    """Return a cppyy namespace's stable qualified name or a string test key."""
    return namespace if isinstance(namespace, str) else getattr(namespace, "__cpp_name__", "")


def is_public_name(namespace, name):
    """Return whether a namespace spelling is eligible for public presentation.

    Args:
        namespace: Native namespace proxy or its exact namespace key string.
        name: C++ or Python-facing spelling to classify.
    Returns:
        True unless the exact spelling is known bridge metadata/implementation.
    """
    if not isinstance(name, str) or name.startswith("_") or name == "print":
        return False
    return name not in _BRIDGE_EXCLUSIONS.get(_namespace_key(namespace), {})


def public_names(namespace):
    """Return eligible names exposed by cppyy reflection and public template seeds.

    Parameters
    ----------
    namespace : object
        Native cppyy namespace proxy after its headers have been included.

    Returns
    -------
    list[str]
        Sorted, resolvable native names excluding exact bridge-only declarations.
    """
    names = []
    namespace_name = _namespace_key(namespace)
    # Cling omits uninstantiated templates from dir(). Seed public declarations,
    # never particular template widths/types, and confirm native resolution.
    for name in sorted(set(dir(namespace)) | set(_PUBLIC_NAME_SEEDS.get(namespace_name, ()))):
        if not is_public_name(namespace_name, name):
            continue
        try:
            getattr(namespace, name)
        except AttributeError, TypeError:
            # Cling may list declarations injected from class scopes that
            # cannot actually be resolved as namespace members.
            continue
        names.append(name)
    return sorted(names)


def presented_names(module_globals, namespace, adapters=()):
    """Return native public names plus explicitly registered Python adapters.

    Args:
        module_globals: Module dictionary containing Python adapter objects.
        namespace: Native cppyy namespace proxy after its headers are included.
        adapters: Explicit Python-facing names permitted from module_globals.
    Returns:
        Sorted union of resolvable native names and listed adapter names present
        in module_globals. Other imported globals are never inferred as exports.
    """
    names = set(public_names(namespace))
    names.update(name for name in adapters if is_public_name(namespace, name) and name in module_globals)
    return sorted(names)


def document_member(member, *, is_enum=None):
    """Supply missing native type documentation without replacing overload help.

    Parameters
    ----------
    member : object
        cppyy class, enum or template proxy. Existing documentation is preserved.
    is_enum : bool or None, keyword-only
        Enum classification from the current namespace discovery call. When
        omitted, inspect the proxy metadata directly.

    Returns
    -------
    object
        The same native proxy, with a docstring when the proxy permits assignment.
    """
    if is_enum is None:
        is_enum = isinstance(member, type) and hasattr(member, "__underlying")
    if vars(member).get("__doc__") if is_enum else getattr(member, "__doc__", None):
        return member

    cpp_name = getattr(member, "__cpp_name__", None)

    if cpp_name is None:
        cpp_name = getattr(member, "__name__", None)

    if not cpp_name:
        return member

    if is_enum:
        values = [
            (name, value) for name, value in vars(member).items() if not name.startswith("_") and not callable(value)
        ]
        doc = f"Native C++ enum {cpp_name}.\n\nMembers:\n" + "\n".join(
            f"    {name} = {int(value)}" for name, value in values
        )
    elif type(member).__name__ == "Template":
        doc = (
            f"Native C++ template {cpp_name}.\n\n"
            "Instantiate with Type[parameters]; parameters retain native C++ semantics.\n"
            "Use help(Type[parameters]) for methods and constructor overloads."
        )
    elif isinstance(member, type):
        doc = (
            f"Native C++ class {cpp_name}.\n\n"
            "Constructor and method help lists native C++ overload signatures.\n"
            "Keep referenced native objects alive for the duration of their use."
        )
    else:
        return member

    try:
        if not is_enum:
            member.__doc__ = doc
        # cppyy enums have read-only type metadata. Their member names and
        # values remain visible to pydoc; namespace help adds a readable list.
    except AttributeError, TypeError:
        pass

    return member


def namespace_help(module_globals, namespace):
    """Build grouped namespace documentation and annotate native type proxies.

    Parameters
    ----------
    module_globals : dict
        Python namespace dictionary, including adapters and its introduction.
    namespace : object
        Included native cppyy namespace.
    Returns
    -------
    str
        Documentation suitable for module.__doc__ and Python help().
    """
    groups = {"Classes": [], "Templates": [], "Enums": [], "Functions": [], "Constants": []}
    for name in presented_names(module_globals, namespace, adapters=module_globals.get("__all__", ())):
        if name in module_globals:
            member = module_globals[name]
        else:
            member = getattr(namespace, name)
        is_enum = isinstance(member, type) and hasattr(member, "__underlying")
        member = document_member(member, is_enum=is_enum)
        if is_enum:
            category = "Enums"
        elif type(member).__name__ == "Template":
            category = "Templates"
        elif isinstance(member, type):
            category = "Classes"
        elif callable(member):
            category = "Functions"
        else:
            category = "Constants"
        groups[category].append(name)

    intro = module_globals.get("__doc__") or f"Native {namespace.__cpp_name__} SystemC namespace."
    lines = [intro.rstrip(), "\nNative API discovery", "--------------------"]
    for title, names in groups.items():
        if names:
            lines.append(f"{title}: " + ", ".join(names))

    for name in groups["Enums"]:
        if name in module_globals:
            enum = module_globals[name]
        else:
            enum = getattr(namespace, name)
        values = [(key, value) for key, value in vars(enum).items() if not key.startswith("_") and not callable(value)]
        lines.append(name + ": " + ", ".join(f"{key}={int(value)}" for key, value in values))
    lines.append(
        "\nUse help(namespace.Type), help(namespace.Type[parameters]), or\n"
        "help(namespace.Type.method) for native declarations and overloads.\n"
        "dir(namespace) lists currently discoverable names. Templates remain unrestricted."
    )
    return "\n\n".join(lines)
