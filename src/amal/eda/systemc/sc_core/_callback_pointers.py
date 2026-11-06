"""Expose native callback pointer-value adapters."""

import re

import cppyy

_FUNCTION_NAME = re.compile(r"(?:::)?[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*\Z", re.ASCII)


def _qualified_function(function):
    """Reject expressions before selecting a borrowed qualified native function."""
    if not isinstance(function, str) or not _FUNCTION_NAME.fullmatch(function) or "::" not in function:
        raise TypeError("function must be a qualified native function name without '&'")
    return "&" + function


def _decorate(pointer_type, kind):
    """Limit construction to null/copies and exact compile-time native selection."""
    pointer_type.__doc__ = """Store an exact native callback pointer in a pointer-value adapter.

    Args:
        value: Same adapter type to copy, or None for a null native pointer.
    Raises:
        TypeError: Construction receives another value or a Python callable.
    Use from_function with a qualified global or namespace function name to
    select a native overload with the exact callback signature. Copies preserve
    pointer identity. This adapter stores a native pointer value, rather than
    representing a reflected function-pointer type. Native code remains borrowed
    and must stay loaded through all consumers; no Python callable is retained.
    Native reflected constructors do not implicitly accept this holder; use an
    explicitly adapted first-party consumer. Construction and native selection
    hold the GIL and do not invoke callbacks.
    """
    native_init = pointer_type.__init__

    def initialize(self, value=None):
        """Create null or copy an exact native pointer value.

        Args:
            value: Same adapter type, or None for a null pointer.
        Raises:
            TypeError: Value is neither a matching holder nor None.
        Code remains borrowed. Use from_function for native function selection;
        Python callbacks use the existing native consumer APIs and retention.
        """
        if value is None:
            native_init(self)
        elif isinstance(value, pointer_type):
            native_init(self, value)
        else:
            raise TypeError("callback pointer construction requires a matching holder or None")

    def from_function(function):
        """Store a qualified native function with the exact callback signature.

        Args:
            function: Qualified global ("::function") or namespace C++ function
                name without '&'; native overload resolution uses the exact typedef.
        Returns:
            Native pointer-value adapter retaining exact pointer identity.
        Raises:
            TypeError: Name is malformed or the function signature differs.
        Native code must remain loaded through all consumers. No Python callable
        is retained or invoked; GIL remains held during template selection.
        """
        name = _qualified_function(function)
        if kind == 3:
            return cppyy.gbl.pysystemc.make_report_callback_pointer[name]()
        return cppyy.gbl.pysystemc.make_callback_pointer[kind, name]()

    pointer_type.__init__ = initialize
    pointer_type.from_function = staticmethod(from_function)
    if kind == 0:

        def create_queue(self, queue_type, size):
            """Allocate a Python-owned native queue using this exact comparator.

            Args:
                queue_type: sc_ppq_base or a native sc_ppq specialization.
                size: Initial native queue capacity.
            Returns:
                Python-owned queue; stored pointees and comparator code are borrowed.
            Raises:
                TypeError: Queue type or size is incompatible.
                ValueError: Comparator is null.
            Keep pointees and native code alive through queue operations. GIL
            remains held and native allocation/report exceptions propagate.
            """
            if not self.valid():
                raise ValueError("cannot use a null native callback pointer")
            queue = cppyy.gbl.pysystemc.make_callback_queue[queue_type](size, self)
            queue.__python_owns__ = True
            return queue

        pointer_type.create_queue = create_queue
    return pointer_type


def _wrap_consumer(native, name=""):
    """Preserve existing callable consumers while accepting exact holder values."""
    spelling = getattr(native, "__cpp_name__", "")
    if spelling.startswith("sc_core::sc_pvector<") and not native.__dict__.get("_callback_sort_wrapped", False):
        native_sort = native.sort
        pointer_type = cppyy.gbl.pysystemc.callback_pointer[0]

        def sort(self, comparator):
            """Sort borrowed pointer slots using a native holder or existing callable.

            Native comparator receives slot addresses, not pointees. Keep code
            and pointees alive; the GIL stays held. Empty vectors are unchanged
            after comparator validation. Null holders raise ValueError and
            non-callable Python comparators raise TypeError. Native qsort
            callbacks must not throw across the C callback boundary.
            """
            if isinstance(comparator, pointer_type):
                if not comparator.valid():
                    raise ValueError("cannot use a null native callback pointer")
                if self.size() == 0:
                    return None
                return cppyy.gbl.pysystemc.sort_callback_pointer[native](self, comparator)
            if not callable(comparator):
                raise TypeError("sort comparator must be callable or a native callback pointer")
            if self.size() == 0:
                return None
            return native_sort(self, comparator)

        if hasattr(native_sort, "__overload__"):
            sort.__overload__ = native_sort.__overload__
        native.sort = sort
        native._callback_sort_wrapped = True
    if spelling == "sc_core::sc_plist_base" and not native.__dict__.get("_callback_map_wrapped", False):
        native_map = native.mapcar
        pointer_type = cppyy.gbl.pysystemc.callback_pointer[1]

        def mapcar(self, mapper, context):
            """Map borrowed list payloads/context using a holder or existing callable.

            Keep pointees/context and native code alive. List owns nodes only;
            do not remove the current traversal node. GIL remains held. Null
            holders raise ValueError; native/callback exceptions propagate.
            """
            if isinstance(mapper, pointer_type):
                if not mapper.valid():
                    raise ValueError("cannot use a null native callback pointer")
                return cppyy.gbl.pysystemc.map_callback_pointer(self, mapper, context)
            return native_map(self, mapper, context)

        if hasattr(native_map, "__overload__"):
            mapcar.__overload__ = native_map.__overload__
        native.mapcar = mapcar
        native._callback_map_wrapped = True


def _wrap_hash_consumer(native, name=""):
    """Adapt native hash setters while preserving Python callback retention."""
    if name != "sc_phash_base" and not name.startswith(("sc_phash<", "sc_pdhash<", "sc_strhash<")):
        return
    if native.__dict__.get("_hash_pointer_setters_wrapped", False):
        return

    native_hash_setter = native.set_hash_fn
    native_compare_setter = native.set_cmpr_fn
    hash_pointer = cppyy.gbl.pysystemc.callback_pointer[2]
    compare_pointer = cppyy.gbl.pysystemc.callback_pointer[0]

    def set_hash_fn(self, callback):
        """Set a borrowed native hash pointer or preserve the existing callback path.

        Args:
            callback: Exact hash pointer holder or a native/Python callback with
                the native ``unsigned(const void*)`` signature.
        Returns:
            None, matching the native setter.
        Raises:
            TypeError: Callback cannot convert to the exact native hash signature.
            ValueError: The native pointer holder is null.

        Native pointer holders retain no code or Python references; keep their
        native function loaded through table use. Existing Python callback
        retention remains on the original setter path. Calls hold the GIL and
        callback execution remains synchronous on the caller thread.
        """
        if isinstance(callback, hash_pointer):
            if not callback.valid():
                raise ValueError("cannot use a null native callback pointer")
            return cppyy.gbl.pysystemc.set_phash_hash_callback(self, callback)
        return native_hash_setter(self, callback)

    def set_cmpr_fn(self, callback):
        """Set a borrowed native comparison pointer or preserve callback handling.

        Args:
            callback: Exact CFT pointer holder or a native/Python callback with
                the native ``int(const void*, const void*)`` signature.
        Returns:
            None, matching the native setter.
        Raises:
            TypeError: Callback cannot convert to the exact native comparison signature.
            ValueError: The native pointer holder is null.

        Native pointer holders retain no code or Python references; keep their
        native function loaded through table use. Existing Python callback
        retention remains on the original setter path. Calls hold the GIL and
        callback execution remains synchronous on the caller thread.
        """
        if isinstance(callback, compare_pointer):
            if not callback.valid():
                raise ValueError("cannot use a null native callback pointer")
            return cppyy.gbl.pysystemc.set_phash_compare_callback(self, callback)
        return native_compare_setter(self, callback)

    if hasattr(native_hash_setter, "__overload__"):
        set_hash_fn.__overload__ = native_hash_setter.__overload__
    if hasattr(native_compare_setter, "__overload__"):
        set_cmpr_fn.__overload__ = native_compare_setter.__overload__
    native.set_hash_fn = set_hash_fn
    native.set_cmpr_fn = set_cmpr_fn
    native._hash_pointer_setters_wrapped = True


def install():
    """Return public pointer-value adapters and install synchronous consumers."""
    pointers = tuple(_decorate(cppyy.gbl.pysystemc.callback_pointer[kind], kind) for kind in range(3))
    report_type = _decorate(cppyy.gbl.pysystemc.report_handler_pointer, 3)
    namespace = cppyy.gbl.sc_core
    namespace.sc_ppq_base.compare_fn_t = pointers[0]
    hash_base = namespace.sc_phash_base
    hash_base.hash_fn_t = pointers[2]
    hash_base.cmpr_fn_t = pointers[0]
    _wrap_hash_consumer(hash_base, "sc_phash_base")
    _wrap_consumer(namespace.sc_plist_base)
    for candidate in tuple(vars(namespace).values()):
        if isinstance(candidate, type):
            _wrap_consumer(candidate)
            cpp_name = getattr(candidate, "__cpp_name__", "")
            simple_name = cpp_name.removeprefix("sc_core::")
            _wrap_hash_consumer(candidate, simple_name)
    cppyy.py.add_pythonization(_wrap_consumer, "sc_core")
    cppyy.py.add_pythonization(_wrap_hash_consumer, "sc_core")
    return pointers[0], pointers[1], report_type
