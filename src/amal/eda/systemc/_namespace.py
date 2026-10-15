"""Discover public native namespace members without constraining templates."""


def public_names(namespace):
    """Return public C++ names exposed by cppyy's namespace reflection.

    Parameters
    ----------
    namespace : object
        Native cppyy namespace proxy after its headers have been included.

    Returns
    -------
    list[str]
        Sorted names excluding Python proxy metadata and C++ implementation names.
    """
    names = []
    # Cling omits some uninstantiated templates from dir(). Seed public
    # class/template names, never template parameter values, then validate native resolution.
    seeds = {
        "sc_core": (
            "sc_attr_base",
            "sc_attr_cltn",
            "sc_attribute",
            "sc_buffer",
            "sc_event_and_list",
            "sc_event_finder",
            "sc_event_or_list",
            "sc_event_queue",
            "sc_export",
            "sc_fifo",
            "sc_in_resolved",
            "sc_in_rv",
            "sc_in",
            "sc_inout_resolved",
            "sc_inout_rv",
            "sc_inout",
            "sc_interface",
            "sc_join",
            "sc_mutex",
            "sc_out_resolved",
            "sc_out_rv",
            "sc_out",
            "sc_port",
            "sc_prim_channel",
            "sc_process_handle",
            "sc_report_handler",
            "sc_report",
            "sc_semaphore",
            "sc_signal_resolved",
            "sc_signal_rv",
            "sc_signal",
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
    }
    for name in sorted(set(dir(namespace)) | set(seeds.get(namespace.__cpp_name__, ()))):
        if name.startswith("_") or name == "print":
            continue
        try:
            getattr(namespace, name)
        except AttributeError, TypeError:
            # Cling may list declarations injected from class scopes that
            # cannot actually be resolved as namespace members.
            continue
        names.append(name)
    return sorted(names)


def document_member(member):
    """Supply missing native type documentation without replacing overload help.

    Parameters
    ----------
    member : object
        cppyy class, enum or template proxy. Existing documentation is preserved.

    Returns
    -------
    object
        The same native proxy, with a docstring when the proxy permits assignment.
    """
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
    for name in public_names(namespace):
        member = document_member(module_globals.get(name, getattr(namespace, name)))
        if isinstance(member, type) and hasattr(member, "__underlying"):
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
        enum = getattr(namespace, name)
        values = [(key, value) for key, value in vars(enum).items() if not key.startswith("_") and not callable(value)]
        lines.append(name + ": " + ", ".join(f"{key}={int(value)}" for key, value in values))
    lines.append(
        "\nUse help(namespace.Type), help(namespace.Type[parameters]), or\n"
        "help(namespace.Type.method) for native declarations and overloads.\n"
        "dir(namespace) lists currently discoverable names. Templates remain unrestricted."
    )
    return "\n\n".join(lines)
