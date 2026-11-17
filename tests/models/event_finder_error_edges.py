"""Exercise event finder validation, cache recovery, and inherited callbacks."""

from amal.eda.systemc import sc_core as sc
import cppyy


cppyy.cppdef(
    r"""
    namespace event_finder_error_edges {
    /** @brief Native interface exposing an inherited event finder target.
     *
     * The interface owns its event for the object's full lifetime. A finder
     * borrows that event through the port, so retain this interface and port
     * while the finder is used. These methods do not retain Python objects or
     * invoke Python callbacks. cppyy calls enter with the GIL held; notify_ready
     * must run on the kernel thread. The registered Python observer is retained
     * by its native process for the module lifetime, and callback entry acquires
     * the GIL through the binding.
     */
    class ReadyBase : public virtual sc_core::sc_interface {
    public:
      /** @brief Return the interface-owned event borrowed by event finders. */
      const sc_core::sc_event& ready() const { return ready_event_; }
      /** @brief Return a stable value observed by the Python process callback. */
      int status() const { return 73; }
      /** @brief Schedule the owned event; caller must remain on the kernel thread. */
      void notify_ready(int delay, sc_core::sc_time_unit unit) { ready_event_.notify(delay, unit); }
    private:
      sc_core::sc_event ready_event_;
    };
    /** @brief Derived interface used to exercise inherited member reflection. */
    class ReadyDerived : public ReadyBase {};
    }
    """
)


def require_type_error(action):
    """Require an invalid finder factory call to raise TypeError.

    Args:
        action: Zero-argument callable performing the invalid factory call.

    Returns:
        None after the adapter rejects the input.

    Raises:
        AssertionError: The action returns or raises another exception.
    """
    try:
        action()
    except TypeError:
        return
    raise AssertionError("invalid event finder input did not raise TypeError")


base_type = cppyy.gbl.event_finder_error_edges.ReadyBase
derived_type = cppyy.gbl.event_finder_error_edges.ReadyDerived
base_method = base_type.ready
scalar_method = base_type.status
other_method = sc.sc_signal_in_if[bool].value_changed_event
finder_constructor = sc.sc_event_finder_t[derived_type]
native_finder_type = cppyy.gbl.sc_core.sc_event_finder_t[derived_type]
constructor_spelling_match = sc.sc_event_finder_t(derived_type) is finder_constructor
metadata_match = (
    finder_constructor.__cpp_name__ == native_finder_type.__cpp_name__
    and sc.sc_event_finder_t.__cpp_name__ == cppyy.gbl.sc_core.sc_event_finder_t.__cpp_name__
)
assert constructor_spelling_match and metadata_match


class Model(sc.sc_module):
    """Own the inherited interface, port, finder cache, and event observer."""

    def __init__(self):
        """Build finders, exercise invalid methods, and register the observer."""
        name = sc.sc_module_name("event_finder_error_edges_model")
        super().__init__(name)
        self.source = derived_type()
        self.port = sc.sc_port[derived_type]("edge_port")
        self.port.bind(self.source)
        self.cache = sc.sc_event_finder_cache()
        self.observations = []

        direct_invalid_actions = (
            None,
            17,
            lambda: None,
            other_method,
            scalar_method,
        )
        self.direct_error_count = 0
        for event_method in direct_invalid_actions:
            require_type_error(lambda method=event_method: finder_constructor(self.port, method))
            self.direct_error_count += 1

        cached_create = sc.sc_event_finder.cached_create[derived_type]
        self.cache_error_count = 0
        for event_method in (None, other_method, scalar_method):
            require_type_error(lambda method=event_method: cached_create(self.cache, self.port, method))
            self.cache_error_count += 1
            assert cppyy.addressof(self.cache.pointer) == 0

        self.cached_finder = cached_create(self.cache, self.port, base_method)
        cached_pointer = cppyy.addressof(self.cache.pointer)
        cached_second = cached_create(self.cache, self.port, base_method)
        self.cache_same = cppyy.addressof(self.cached_finder) == cppyy.addressof(cached_second)
        self.cache_borrowed = not bool(self.cached_finder.__python_owns__)
        self.cache_port = cppyy.addressof(self.cached_finder.port()) == cppyy.addressof(self.port)
        self.cache_event = cppyy.addressof(self.cached_finder.find_event()) == cppyy.addressof(self.source.ready())
        assert self.cache_same and self.cache_borrowed and self.cache_port and self.cache_event

        require_type_error(lambda: cached_create(self.cache, self.port, scalar_method))
        self.cache_error_count += 1
        assert cppyy.addressof(self.cache.pointer) == cached_pointer
        assert cppyy.addressof(self.cached_finder.find_event()) == cppyy.addressof(self.source.ready())

        self.finder = finder_constructor(self.port, base_method)
        self.finder_owned = bool(self.finder.__python_owns__)
        self.finder_event = cppyy.addressof(self.finder.find_event()) == cppyy.addressof(self.source.ready())
        assert self.finder_owned and self.finder_event

        self.sc_method("observe", self.observe)
        self.sensitive(self.finder)
        self.dont_initialize()

    def observe(self):
        """Capture callback time and value from the inherited native event method."""
        nanoseconds = sc.sc_time_stamp().value() // sc.sc_time(1, sc.SC_NS).value()
        self.observations.append((nanoseconds, self.source.status()))


model = Model()
model.source.notify_ready(3, sc.SC_NS)
sc.sc_start(4, sc.SC_NS)
assert model.observations == [(3, 73)], model.observations
assert cppyy.addressof(model.cached_finder.find_event()) == cppyy.addressof(model.source.ready())

reference = (
    f"FINDER_EDGE_REFERENCE finder_lookup={int(model.finder_event)} cache_same={int(model.cache_same)} "
    f"cache_port={int(model.cache_port)} callbacks={len(model.observations)} "
    f"event_value={model.observations[0][1]} time_ns={model.observations[0][0]}"
)
print(reference)
print(
    "FINDER_EDGE_VALIDATION "
    f"constructor_errors={model.direct_error_count} cache_errors={model.cache_error_count} "
    f"empty_after_errors=1 usable_after_errors=1 call_spelling={int(constructor_spelling_match)} "
    f"metadata={int(metadata_match)} direct_owned={int(model.finder_owned)} "
    f"cache_borrowed={int(model.cache_borrowed)}"
)
