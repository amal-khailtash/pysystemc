"""Construct generic native event finders from reflected interface methods."""

import re
import cppyy


def _event_member_selector(declaring_type, method_name):
    """Select the exact native const-event signature, including inherited overloads."""
    return f"&{declaring_type.__cpp_name__}::{method_name}"


class _FinderSpecialization:
    """Construct an owning native finder and delegate reflection to its native type."""

    def __init__(self, interface_type):
        """Retain the native interface specialization and its finder class."""
        self._interface_type = interface_type
        self._native = cppyy.gbl.sc_core.sc_event_finder_t[interface_type]

    def __call__(self, port, event_method):
        """Construct a native finder for a reflected const event method.

        Args:
            port: Native port borrowed for the complete finder lifetime.
            event_method: Reflected method on the selected native interface,
                such as ``IF.value_changed_event``. Arbitrary native interface
                types and method names are instantiated on demand.

        Returns:
            An owning native ``sc_event_finder_t[IF]`` proxy.

        Raises:
            TypeError: If the method is not a reflected native const event method.
            Native report/conversion exceptions: If construction fails.

        Keep the port, bound interface and event alive while the finder is used.
        Construction holds the GIL and belongs to the kernel thread.
        """
        method_name = getattr(event_method, "__name__", "")
        signature = getattr(event_method, "__cpp_name__", "") + getattr(event_method, "func_doc", "")
        if not re.fullmatch(r"[A-Za-z_]\w*", method_name) or not callable(event_method):
            raise TypeError("event_method must be a reflected native const event method")
        if "sc_core::sc_event" not in signature:
            raise TypeError("event_method must return a native const sc_event reference")
        declaring_class = getattr(event_method, "im_class", None)
        if declaring_class is None or not issubclass(self._interface_type, declaring_class):
            raise TypeError("event_method must belong to the selected native interface")
        # Template argument parsing checks the exact signature and interface owner.
        factory = cppyy.gbl.pysystemc_public.make_event_finder[
            self._interface_type, declaring_class, _event_member_selector(declaring_class, method_name)
        ]
        finder = factory(port)
        finder.__python_owns__ = True
        return finder

    def __getattr__(self, name):
        """Delegate native type reflection without modifying returned views."""
        return getattr(self._native, name)

    def __instancecheck__(self, instance):
        """Recognize the native finder proxy returned by construction."""
        return isinstance(instance, self._native)


class EventFinderTemplate:
    """Instantiate native event finders for arbitrary native interface types.

    Use ``sc_event_finder_t[IF](port, IF.event_method)``. The returned finder owns
    its native allocation and borrows its port/interface/event dependencies.
    """

    def __init__(self):
        """Cache specialized constructors while retaining native reflection."""
        self._specializations = {}

    def __getitem__(self, interface_type):
        """Return the constructor for a native interface type.

        Args:
            interface_type: Native type compatible with ``sc_interface`` or its
                fully qualified native type spelling.
        Returns:
            A constructor preserving the native finder signature and reflection.
        Raises:
            TypeError: If cppyy cannot instantiate the interface/finder type.
        """
        if isinstance(interface_type, str):
            interface_type = getattr(cppyy.gbl, interface_type)
        if interface_type not in self._specializations:
            self._specializations[interface_type] = _FinderSpecialization(interface_type)
        return self._specializations[interface_type]

    def __call__(self, interface_type):
        """Select a specialization using cppyy's alternate template-call spelling."""
        return self[interface_type]

    def __getattr__(self, name):
        """Expose native template metadata for introspection."""
        return getattr(cppyy.gbl.sc_core.sc_event_finder_t, name)


class CachedFinderCreator:
    """Forward native cached_create using an owning native pointer slot."""

    def __getitem__(self, interface_type):
        """Select the native interface specialization for cache construction."""
        if isinstance(interface_type, str):
            interface_type = getattr(cppyy.gbl, interface_type)

        def create(cache, port, event_method):
            """Return a borrowed cached finder, allocating only once.

            Args:
                cache: Native ``sc_event_finder_cache`` owner, initially empty.
                port: Borrowed port; all uses of this cache must use the same port.
                event_method: Reflected const event method of the selected IF.
            Returns:
                Native finder borrowed from cache, with cache retained by the proxy.
            Raises:
                TypeError: If the reflected method does not match the selected IF.
                Native exceptions: If allocation or conversion fails.

            Destroy dependent processes before the cache. Keep the port, bound
            interface and events alive. Calls hold the GIL on the kernel thread;
            native same-port assertions and first-method cache semantics apply.
            """
            selection = _FinderSpecialization(interface_type)
            method_name = getattr(event_method, "__name__", "")
            declaring_class = getattr(event_method, "im_class", None)
            if (
                declaring_class is None
                or not issubclass(interface_type, declaring_class)
                or not re.fullmatch(r"[A-Za-z_]\w*", method_name)
            ):
                raise TypeError("event_method must belong to the selected native interface")
            signature = getattr(event_method, "__cpp_name__", "") + getattr(event_method, "func_doc", "")
            if "sc_core::sc_event" not in signature:
                raise TypeError("event_method must return a native const sc_event reference")
            factory = cppyy.gbl.pysystemc_public.cached_event_finder[
                selection._interface_type, declaring_class, _event_member_selector(declaring_class, method_name)
            ]
            finder = factory(cache, port)
            finder.__python_owns__ = False
            finder._pysystemc_cache_owner = cache
            return finder

        return create
