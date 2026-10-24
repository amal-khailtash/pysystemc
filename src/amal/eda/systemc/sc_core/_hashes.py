"""Retain Python function-pointer callbacks for native hash-table lifetimes."""

import cppyy


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
