"""Compare interleaved module member triggers with independent native methods."""

import gc
import json
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy


cppyy.cppdef(
    r"""
    #include <systemc>
    #include <vector>

    namespace sc_core_member_trigger_lifetime {
    /**
     * @brief Independently schedule two native methods with private event sets.
     * @details The module retains every event and method through simulation.
     * Its native callbacks establish the timing reference without entering
     * Python or acquiring the GIL; drive() emits a separate matching event
     * schedule for this reference model.
     */
    struct Reference : sc_core::sc_module {
      SC_HAS_PROCESS(Reference);
      sc_core::sc_event a_first{"ref_a_first"};
      sc_core::sc_event a_second{"ref_a_second"};
      sc_core::sc_event a_tail{"ref_a_tail"};
      sc_core::sc_event b_first{"ref_b_first"};
      sc_core::sc_event b_second{"ref_b_second"};
      sc_core::sc_event b_tail{"ref_b_tail"};
      std::vector<int> a_steps;
      std::vector<unsigned long long> a_ticks;
      std::vector<int> b_steps;
      std::vector<unsigned long long> b_ticks;

      /** Register both immediate SC_METHOD references and their native driver. */
      explicit Reference(sc_core::sc_module_name name) : sc_core::sc_module(name) {
        SC_METHOD(trigger_a);
        SC_METHOD(trigger_b);
        SC_THREAD(drive);
      }

      /** Record and schedule the native A method's time/event/list sequence. */
      void trigger_a() {
        const int step = static_cast<int>(a_steps.size());
        a_steps.push_back(step);
        a_ticks.push_back(sc_core::sc_time_stamp().value());
        if (step == 0)
          next_trigger(sc_core::sc_time(1, sc_core::SC_NS));
        else if (step == 1)
          next_trigger(a_first);
        else if (step == 2)
          next_trigger(a_first | a_second);
        else if (step == 3)
          next_trigger(a_first & a_second);
        else if (step == 4)
          next_trigger(sc_core::sc_time(2, sc_core::SC_NS), a_tail);
      }

      /** Record and schedule the native B method with different current values. */
      void trigger_b() {
        const int step = static_cast<int>(b_steps.size());
        b_steps.push_back(step);
        b_ticks.push_back(sc_core::sc_time_stamp().value());
        if (step == 0)
          next_trigger(sc_core::sc_time(2, sc_core::SC_NS));
        else if (step == 1)
          next_trigger(b_first);
        else if (step == 2)
          next_trigger(b_first | b_second);
        else if (step == 3)
          next_trigger(b_first & b_second);
        else if (step == 4)
          next_trigger(sc_core::sc_time(4, sc_core::SC_NS), b_tail);
      }

      /** Drive independent native event targets at the documented nanoseconds. */
      void drive() {
        using sc_core::SC_NS;
        using sc_core::SC_ZERO_TIME;
        wait(1, SC_NS);
        wait(1, SC_NS);
        wait(1, SC_NS);
        a_first.notify(SC_ZERO_TIME);
        wait(1, SC_NS);
        a_second.notify(SC_ZERO_TIME);
        b_first.notify(SC_ZERO_TIME);
        wait(1, SC_NS);
        a_first.notify(SC_ZERO_TIME);
        b_second.notify(SC_ZERO_TIME);
        wait(1, SC_NS);
        a_second.notify(SC_ZERO_TIME);
        b_first.notify(SC_ZERO_TIME);
        wait(1, SC_NS);
        b_second.notify(SC_ZERO_TIME);
        a_tail.notify(SC_ZERO_TIME);
        wait(4, SC_NS);
      }

      /** Return the number of recorded native A method callbacks. */
      unsigned int a_count() const {
        return static_cast<unsigned int>(a_steps.size());
      }
      /** Return one native A callback's sequence index. */
      int a_step(unsigned int index) const { return a_steps.at(index); }
      /** Return one native A callback's timestamp in SystemC ticks. */
      unsigned long long a_tick(unsigned int index) const {
        return a_ticks.at(index);
      }
      /** Return the number of recorded native B method callbacks. */
      unsigned int b_count() const {
        return static_cast<unsigned int>(b_steps.size());
      }
      /** Return one native B callback's sequence index. */
      int b_step(unsigned int index) const { return b_steps.at(index); }
      /** Return one native B callback's timestamp in SystemC ticks. */
      unsigned long long b_tick(unsigned int index) const {
        return b_ticks.at(index);
      }
    };
    } // namespace sc_core_member_trigger_lifetime
    """
)


class TriggerOwner(sc.sc_module):
    """Register a module-owned method that advances through reusable signatures."""

    def __init__(self, name, label, weakref_records, unsupported_weakrefs):
        """Retain owner-specific events and register the method callback."""
        super().__init__(name)
        self.label = label
        self.event_first = sc.sc_event(f"{label}_first")
        self.event_second = sc.sc_event(f"{label}_second")
        self.event_tail = sc.sc_event(f"{label}_tail")
        self.rows = []
        self.weakref_records = weakref_records
        self.unsupported_weakrefs = unsupported_weakrefs
        self.sc_method("trigger_sequence", self.trigger_sequence)
        self.method_handle = sc.sc_get_last_created_process_handle()

    def _register_expression(self, form, first, second):
        """Pass a new OR/AND proxy, then check its Python proxy lifetime."""
        expression = first | second if form == "or" else first & second
        label = f"{self.label}:{form}:{len(self.rows)}"
        reference = None
        try:
            reference = weakref.ref(expression)
        except TypeError:
            # cppyy expression proxies may omit weak-reference support; retain
            # the explicit unsupported record instead of treating it as passed.
            self.unsupported_weakrefs.append(label)
        else:
            self.weakref_records.append((label, reference))
        self.next_trigger(expression)
        del expression
        gc.collect()
        if reference is not None and reference() is not None:
            raise AssertionError(f"member trigger retained Python expression proxy {label}")

    def trigger_sequence(self):
        """Record one callback and install this owner's next dynamic sensitivity."""
        step = len(self.rows)
        ticks = int(sc.sc_time_stamp().value())
        forms = ("time", "event", "or", "and", "time_event")
        if step == 5:
            self.rows.append({"owner": self.label, "step": step, "form": "terminal", "ticks": ticks})
            return
        form = forms[step]
        self.rows.append({"owner": self.label, "step": step, "form": form, "ticks": ticks})
        if step == 0:
            delay = 1 if self.label == "A" else 2
            self.next_trigger(sc.sc_time(delay, sc.SC_NS))
        elif step == 1:
            self.next_trigger(self.event_first)
        elif step == 2:
            self._register_expression("or", self.event_first, self.event_second)
        elif step == 3:
            self._register_expression("and", self.event_first, self.event_second)
        else:
            delay = 2 if self.label == "A" else 4
            self.next_trigger(sc.sc_time(delay, sc.SC_NS), self.event_tail)


class TriggerLifetimeModel(sc.sc_module):
    """Own two Python modules, a native reference, and the matching stimuli."""

    def __init__(self, name):
        """Build both owners and retain the native SC_METHOD timing reference."""
        super().__init__(name)
        self.weakref_records = []
        self.unsupported_weakrefs = []
        reference_name = sc.sc_module_name("native_reference")
        self.reference = cppyy.gbl.sc_core_member_trigger_lifetime.Reference(reference_name)
        del reference_name
        first_name = sc.sc_module_name("owner_a")
        self.owner_a = TriggerOwner(first_name, "A", self.weakref_records, self.unsupported_weakrefs)
        del first_name
        second_name = sc.sc_module_name("owner_b")
        self.owner_b = TriggerOwner(second_name, "B", self.weakref_records, self.unsupported_weakrefs)
        del second_name
        self.sc_thread("drive_python_events", self.drive_python_events)
        self.driver_handle = sc.sc_get_last_created_process_handle()

    def drive_python_events(self):
        """Pulse each Python owner's private events at exact native reference times."""
        for tick in range(1, 13):
            sc.wait(sc.sc_time(1, sc.SC_NS))
            if tick == 3:
                self.owner_a.event_first.notify(sc.SC_ZERO_TIME)
            elif tick == 4:
                self.owner_a.event_second.notify(sc.SC_ZERO_TIME)
                self.owner_b.event_first.notify(sc.SC_ZERO_TIME)
            elif tick == 5:
                self.owner_a.event_first.notify(sc.SC_ZERO_TIME)
                self.owner_b.event_second.notify(sc.SC_ZERO_TIME)
            elif tick == 6:
                self.owner_a.event_second.notify(sc.SC_ZERO_TIME)
                self.owner_b.event_first.notify(sc.SC_ZERO_TIME)
            elif tick == 7:
                self.owner_b.event_second.notify(sc.SC_ZERO_TIME)
                self.owner_a.event_tail.notify(sc.SC_ZERO_TIME)


def _native_rows(reference, label):
    """Copy one native reference method's step and timestamp rows to Python."""
    count = int(getattr(reference, f"{label.lower()}_count")())
    step = getattr(reference, f"{label.lower()}_step")
    tick = getattr(reference, f"{label.lower()}_tick")
    return [(int(step(index)), int(tick(index))) for index in range(count)]


def main():
    """Assert owner-local cold/warm sensitivities and native row equivalence."""
    name = sc.sc_module_name("trigger_lifetime_top")
    model = TriggerLifetimeModel(name)
    del name
    assert type(model.owner_a).next_trigger is type(model.owner_b).next_trigger
    sc.sc_start()

    expected = {
        "A": (0, 1, 3, 4, 6, 7),
        "B": (0, 2, 4, 5, 7, 11),
    }
    expected_forms = ("time", "event", "or", "and", "time_event", "terminal")
    reference_rows = {}
    for label, owner in (("A", model.owner_a), ("B", model.owner_b)):
        expected_ticks = tuple(int(sc.sc_time(value, sc.SC_NS).value()) for value in expected[label])
        assert tuple(row["ticks"] for row in owner.rows) == expected_ticks, owner.rows
        assert tuple(row["form"] for row in owner.rows) == expected_forms
        assert tuple(row["owner"] for row in owner.rows) == (label,) * len(expected_forms)
        native = _native_rows(model.reference, label)
        python = [(int(row["step"]), int(row["ticks"])) for row in owner.rows]
        assert native == python, (label, native, python)
        reference_rows[label] = native
        assert owner.method_handle.valid()
        assert not owner.method_handle.terminated()
        assert owner.name() == f"trigger_lifetime_top.owner_{label.lower()}"

    assert model.driver_handle.valid() and model.driver_handle.terminated()
    gc.collect()
    retained_expression_proxies = [label for label, reference in model.weakref_records if reference() is not None]
    assert not retained_expression_proxies, retained_expression_proxies
    assert len(model.weakref_records) + len(model.unsupported_weakrefs) == 4
    assert model.unsupported_weakrefs == list(dict.fromkeys(model.unsupported_weakrefs))
    assert int(sc.sc_time_stamp().value()) == int(sc.sc_time(12, sc.SC_NS).value())

    print(
        "MEMBER_TRIGGER_LIFETIME_OK "
        + json.dumps(
            {
                "python_rows": {"A": model.owner_a.rows, "B": model.owner_b.rows},
                "native_rows": reference_rows,
                "expression_weakrefs_collected": len(model.weakref_records),
                "expression_weakref_unsupported": model.unsupported_weakrefs,
                "owner_methods_valid_after_simulation": True,
                "driver_terminated": True,
            },
            sort_keys=True,
        )
    )


if __name__ == "__main__":
    main()
