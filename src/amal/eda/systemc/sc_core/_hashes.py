"""Adapt native hash constructors and retain Python callbacks for their lifetime."""

import re

import cppyy


_HASH_BASE_SIGNATURE_PATTERN = re.compile(
    r"\bvoid\s*\*\s*,\s*int\s*,\s*int\s*,\s*double\s*,\s*bool\s*,\s*"
    r"sc_core::sc_phash_base::hash_fn_t\s*,\s*sc_core::sc_phash_base::cmpr_fn_t\b"
)
_HASH_BASE_PARAMETER_NAMES = ("def", "size", "density", "grow", "reorder", "hash_fn", "cmpr_fn")
_NATIVE_HELPER_INSTALLED = False


def _install_native_type_helper():
    """Compile a native predicate used to validate opt-in adapter targets."""
    global _NATIVE_HELPER_INSTALLED
    if _NATIVE_HELPER_INSTALLED:
        return
    cppyy.cppdef(
        """
        #include <sysc/utils/sc_hash.h>

        #include <type_traits>

        namespace pysystemc_hashes
        {
        /**
         * @brief Check whether a native candidate derives from sc_phash_base.
         * @tparam Candidate Candidate C++ type being inspected.
         * @return True for sc_phash_base itself and its native derived types.
         * @details This query owns no objects or callbacks and throws no C++
         * exceptions. Python calls it synchronously while holding the GIL.
         */
        template <typename Candidate>
        bool inherits_sc_phash_base()
        {
          return std::is_base_of<sc_core::sc_phash_base, Candidate>::value;
        }
        }  // namespace pysystemc_hashes
        """
    )
    _NATIVE_HELPER_INSTALLED = True


def _is_native_sc_phash_base_subclass(table_type):
    """Return whether a reflected C++ class derives from, but is not, the hash base."""
    cpp_name = getattr(table_type, "__cpp_name__", None)
    if not cpp_name or cpp_name == "sc_core::sc_phash_base":
        return False
    _install_native_type_helper()
    return bool(cppyy.gbl.pysystemc_hashes.inherits_sc_phash_base[table_type]())


def _has_inherited_constructor_signature(table_type):
    """Check that cppyy reflects the exact seven-parameter hash-base constructor."""
    documentation = getattr(table_type.__init__, "__doc__", "") or ""
    return bool(_HASH_BASE_SIGNATURE_PATTERN.search(documentation))


def _is_default_arity_error(error):
    """Identify cppyy's lost-trailing-default diagnostic for this constructor."""
    return "takes at least 7 arguments" in str(error)


def _native_default_arguments():
    """Return native defaults in declaration order for sc_phash_base."""
    native = cppyy.gbl.sc_core
    return (
        cppyy.nullptr,
        native.PHASH_DEFAULT_INIT_TABLE_SIZE,
        native.PHASH_DEFAULT_MAX_DENSITY,
        native.PHASH_DEFAULT_GROW_FACTOR,
        native.PHASH_DEFAULT_REORDER_FLAG,
        native.default_ptr_hash_fn,
        cppyy.nullptr,
    )


def _completed_inherited_arguments(args, kwargs):
    """Complete positional or named inherited arguments without changing their types."""
    if len(args) > len(_HASH_BASE_PARAMETER_NAMES):
        return None
    if any(name not in _HASH_BASE_PARAMETER_NAMES for name in kwargs):
        return None
    if any(_HASH_BASE_PARAMETER_NAMES.index(name) < len(args) for name in kwargs):
        return None

    completed = list(_native_default_arguments())
    completed[: len(args)] = args
    for name, value in kwargs.items():
        completed[_HASH_BASE_PARAMETER_NAMES.index(name)] = value
    return tuple(completed)


def _call_inherited_constructor(constructor, self, args, kwargs, allow_default_retry):
    """Call a reflected constructor, retrying only its known lost-default shape."""
    try:
        constructor(self, *args, **kwargs)
    except TypeError as original_error:
        if not allow_default_retry or not _is_default_arity_error(original_error):
            raise
        completed_args = _completed_inherited_arguments(args, kwargs)
        if completed_args is None:
            raise
        try:
            constructor(self, *completed_args)
        except TypeError:
            # Keep cppyy's original arity diagnostic if the explicit native
            # retry also fails to select a constructor.
            raise original_error


def adapt_inherited_hash_constructor(table_type):
    """Restore inherited trailing defaults on a native ``sc_phash_base`` subclass.

    Args:
        table_type: Reflected native C++ class inheriting the exact
            ``sc_phash_base`` constructor.

    Returns:
        The same native class, with its familiar constructor call shape.

    Raises:
        TypeError: The class is not a native ``sc_phash_base`` subclass or its
            reflected constructor set does not contain the exact seven-argument
            inherited signature.

    This opt-in adapter is for user-defined C++ subclasses whose ``using``
    declaration inherited the native hash-base constructor. It preserves the
    default value, configuration, hash and comparison callback types from
    SystemC. Positional arguments and the native names ``def``, ``size``,
    ``density``, ``grow``, ``reorder``, ``hash_fn`` and ``cmpr_fn`` are accepted
    when cppyy reports the inherited constructor's lost trailing defaults.
    Python callbacks remain owned by each successfully constructed
    proxy until destruction. Calls hold the GIL; callback invocation remains
    synchronous on the native caller thread. Native conversion and constructor
    exceptions propagate, and other constructor overloads retain cppyy's
    original behavior because cppyy is always given the caller's arguments first.
    """
    if getattr(table_type, "_pysystemc_inherited_hash_constructor", False):
        return table_type
    if not _is_native_sc_phash_base_subclass(table_type):
        raise TypeError("adapter target must be a native subclass of sc_core::sc_phash_base")
    if not _has_inherited_constructor_signature(table_type):
        raise TypeError("adapter target does not expose the inherited sc_phash_base constructor signature")

    native = table_type.__init__

    def construct(self, *args, **kwargs):
        """Construct with native defaults and retain supplied callback owners."""
        _call_inherited_constructor(native, self, args, kwargs, allow_default_retry=True)
        retain(self, args, kwargs)

    def retain(self, args, kwargs):
        """Retain callback proxies after a successful native construction."""
        self._hash_callback_owners = {str(index): argument for index, argument in enumerate(args) if callable(argument)}
        self._hash_callback_owners.update({key: value for key, value in kwargs.items() if callable(value)})

    def select(signature):
        """Preserve explicit cppyy constructor selection and callback ownership."""
        selected = native.__overload__(signature)
        allow_default_retry = bool(_HASH_BASE_SIGNATURE_PATTERN.search(signature))

        def construct_selected(self, *args, **kwargs):
            """Construct a selected overload and retain its callback inputs."""
            _call_inherited_constructor(selected, self, args, kwargs, allow_default_retry)
            retain(self, args, kwargs)

        return construct_selected

    construct.__overload__ = select
    table_type.__init__ = construct
    table_type._pysystemc_inherited_hash_constructor = True
    return table_type


def _pythonize(table_type, name):
    """Attach callback owners to native hash classes and unrestricted templates."""
    if name != "sc_phash_base" and not name.startswith(("sc_phash<", "sc_pdhash<", "sc_strhash<")):
        return
    native = table_type.__init__

    def construct(self, *args, **kwargs):
        """Construct a native table and retain callbacks stored by its constructor.

        Args:
            *args: Native default value, table configuration and callback inputs.
            **kwargs: Native named constructor parameters.

        Raises:
            TypeError: Inputs cannot select a native constructor or callback type.

        Key/content ownership follows the native table specialization. Python
        callback owners survive until proxy destruction; native-created views
        require external lifetime management. Calls hold the GIL.
        """
        native(self, *args, **kwargs)
        retain(self, args, kwargs)

    def retain(self, args, kwargs):
        """Retain callback proxies after a successful native construction."""
        self._hash_callback_owners = {str(index): argument for index, argument in enumerate(args) if callable(argument)}
        self._hash_callback_owners.update({key: value for key, value in kwargs.items() if callable(value)})

    def select(signature):
        """Preserve explicit cppyy constructor selection with callback ownership."""
        selected = native.__overload__(signature)

        def construct_selected(self, *args, **kwargs):
            """Construct the selected native overload and retain its callback inputs."""
            selected(self, *args, **kwargs)
            retain(self, args, kwargs)

        return construct_selected

    construct.__overload__ = select
    table_type.__init__ = construct
    native_lookup = table_type.lookup
    native_contains = table_type.contains

    def lookup(self, key, output):
        """Look up a borrowed key with an optional native output pointer slot.

        Args:
            key: Native key satisfying this table's pointer/type contract.
            output: Native addressable contents slot, or None/cppyy.nullptr to
                omit the output as allowed by native lookup.

        Returns:
            Native integer presence flag (zero or one).

        Calls hold the GIL. Omitted outputs invoke native contains, which calls
        the identical native lookup with a null output pointer. Key hashing,
        comparison callbacks, table reorder effects and exceptions are preserved.
        """
        if output is None or output is cppyy.nullptr:
            return int(native_contains(self, key))
        return native_lookup(self, key, output)

    lookup.__overload__ = native_lookup.__overload__
    table_type.lookup = lookup

    for method_name in ("set_hash_fn", "set_cmpr_fn"):
        method = getattr(table_type, method_name)

        def set_callback(self, callback, _native=method, _name=method_name):
            """Set a native function pointer while retaining its Python callback.

            Args:
                callback: Native function pointer or Python callable with the
                    native hash/comparison signature.

            Native conversion errors propagate before ownership changes. Calls
            hold the GIL; callbacks run synchronously on the caller thread.
            Owners persist until the table proxy is released. Views of tables
            created in native code require an external owner for callbacks.
            """
            result = _native(self, callback)
            self.__dict__.setdefault("_hash_callback_owners", {})[_name] = callback
            return result

        setattr(table_type, method_name, set_callback)


def install():
    """Install lifetime adapters before reflecting hash specializations.

    Hash tables retain Python callback proxies until table destruction. Native
    tables still borrow key/content objects unless explicit duplication/free
    callbacks provide ownership. Iterator owners must outlive borrowed views.
    """
    cppyy.py.add_pythonization(_pythonize, "sc_core")
