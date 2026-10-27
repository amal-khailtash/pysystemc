"""Compare guarded free wait and dynamic next_trigger overloads to native runs."""

from amal.eda.systemc import sc_core as sc


wait_records = []
trigger_records = []
wait_cleanup_records = []
error_records = []


def record(records, label):
    """Save the callback label, exact kernel time and delta count.

    Args:
        records: Result list retained by the simulation model.
        label: Stable name for the overload and completion phase.
    """
    records.append((label, int(sc.sc_time_stamp().value()), int(sc.sc_delta_count())))


WAIT_FORMS = (
    "cycles",
    "time_object",
    "time_number",
    "event",
    "time_event",
    "number_event",
    "or_list",
    "time_or_list",
    "number_or_list",
    "and_list",
    "time_and_list",
    "number_and_list",
    "or_expression",
    "time_or_expression",
    "number_or_expression",
    "and_expression",
    "time_and_expression",
    "number_and_expression",
)


TRIGGER_BASE_FORMS = (
    "event",
    "or_list",
    "and_list",
    "time_object",
    "time_number",
    "time_event",
    "number_event",
    "time_or_list",
    "number_or_list",
    "time_and_list",
    "number_and_list",
    "or_expression",
    "and_expression",
    "time_or_expression",
    "number_or_expression",
    "time_and_expression",
    "number_and_expression",
)

TRIGGER_FORMS = (
    "default_context",
    "explicit_context",
    *(f"{form}_{context}" for form in TRIGGER_BASE_FORMS for context in ("default", "explicit")),
)


class Model(sc.sc_module):
    """Own all events, lists, clocks and retained process callbacks."""

    def __init__(self, name, clock):
        """Create independent waiters and dynamically sensitive methods.

        Args:
            name: Native module name.
            clock: Clock retained for cycle waits.
        """
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = clock
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port.bind(clock)
        self.event_a = sc.sc_event("event_a")
        self.event_b = sc.sc_event("event_b")
        self.static_event = sc.sc_event("static_event")
        self.cleanup_event = sc.sc_event("cleanup_event")
        self.or_list = sc.sc_event_or_list(self.event_a)
        self.or_list |= self.event_b
        self.and_list = sc.sc_event_and_list(self.event_a)
        self.and_list &= self.event_b

        self.sc_cthread("wait_no_arguments", self.wait_no_arguments, self.clock_port.pos())
        self.sc_cthread("wait_cycle_count", self.wait_cycle_count, self.clock_port.pos())
        self.sc_cthread("wait_no_arguments_explicit", self.wait_no_arguments_explicit, self.clock_port.pos())
        self.sc_cthread("wait_cycle_count_explicit", self.wait_cycle_count_explicit, self.clock_port.pos())
        for form in WAIT_FORMS[1:]:
            self.sc_thread(f"wait_{form}_default", self.make_waiter(form, False))
            self.sc_thread(f"wait_{form}_explicit", self.make_waiter(form, True))

        for form in TRIGGER_FORMS:
            self.sc_method(f"trigger_{form}", self.make_trigger(form))
            self.sensitive(self.static_event)

        self.sc_method("invalid_wait_method", self.invalid_wait_method)
        self.sc_thread("invalid_next_trigger_thread", self.invalid_next_trigger_thread)
        self.sc_thread("drive", self.drive)
        self.sc_thread("explicit_wait_cleanup", self.explicit_wait_cleanup)
        self.cleanup_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("control_cleanup", self.control_cleanup)
        self.sc_thread("stopper", self.stopper)

    def wait_no_arguments(self):
        """Wait for one native clock edge using the default process context."""
        sc.wait()
        record(wait_records, "wait_no_arguments")

    def wait_cycle_count(self):
        """Wait two native clock edges through the integer overload."""
        sc.wait(2)
        record(wait_records, "wait_cycles")

    def wait_no_arguments_explicit(self):
        """Wait one native clock edge with an explicit simulation context."""
        sc.wait(sc.sc_get_curr_simcontext())
        record(wait_records, "wait_no_arguments_explicit")

    def wait_cycle_count_explicit(self):
        """Wait two native clock edges with an explicit simulation context."""
        sc.wait(2, sc.sc_get_curr_simcontext())
        record(wait_records, "wait_cycles_explicit")

    def make_waiter(self, form, explicit_context):
        """Return a retained callback for one guarded wait overload.

        Args:
            form: Name of a time, event, list or expression overload.
            explicit_context: Whether to pass the active native simcontext.
        """

        def waiter():
            """Wait using the selected native overload and record resumption."""
            context = sc.sc_get_curr_simcontext()

            def wait(*args):
                """Dispatch one overload with its requested context form."""
                if explicit_context:
                    sc.wait(*args, context)
                else:
                    sc.wait(*args)

            if form == "time_object":
                wait(sc.sc_time(3, sc.SC_NS))
            elif form == "time_number":
                wait(4, sc.SC_NS)
            elif form == "event":
                wait(self.event_a)
            elif form == "time_event":
                wait(sc.sc_time(5, sc.SC_NS), self.event_a)
            elif form == "number_event":
                wait(5, sc.SC_NS, self.event_b)
            elif form == "or_list":
                wait(self.or_list)
            elif form == "time_or_list":
                wait(sc.sc_time(5, sc.SC_NS), self.or_list)
            elif form == "number_or_list":
                wait(5, sc.SC_NS, self.or_list)
            elif form == "and_list":
                wait(self.and_list)
            elif form == "time_and_list":
                wait(sc.sc_time(5, sc.SC_NS), self.and_list)
            elif form == "number_and_list":
                wait(5, sc.SC_NS, self.and_list)
            elif form == "or_expression":
                wait(self.event_a | self.event_b)
            elif form == "time_or_expression":
                wait(sc.sc_time(5, sc.SC_NS), self.event_a | self.event_b)
            elif form == "number_or_expression":
                wait(5, sc.SC_NS, self.event_a | self.event_b)
            elif form == "and_expression":
                wait(self.event_a & self.event_b)
            elif form == "time_and_expression":
                wait(sc.sc_time(5, sc.SC_NS), self.event_a & self.event_b)
            elif form == "number_and_expression":
                wait(5, sc.SC_NS, self.event_a & self.event_b)
            else:
                raise AssertionError(f"unhandled wait overload {form}")
            suffix = "explicit" if explicit_context else "default"
            record(wait_records, f"wait_{form}_{suffix}")

        return waiter

    def make_trigger(self, form):
        """Return a method callback that schedules then restores sensitivity.

        Args:
            form: Name of a native dynamic trigger overload.
        """
        invocation = 0
        no_arguments = form in ("default_context", "explicit_context")
        if no_arguments:
            base_form = ""
            explicit_context = form == "explicit_context"
        else:
            base_form, context_form = form.rsplit("_", 1)
            explicit_context = context_form == "explicit"

        def trigger():
            """Select a dynamic trigger, record completion and restore static sensitivity."""
            nonlocal invocation
            context = sc.sc_get_curr_simcontext()

            def schedule(*args):
                """Pass the requested context form to the public next_trigger adapter."""
                if explicit_context:
                    sc.next_trigger(*args, context)
                else:
                    sc.next_trigger(*args)

            if invocation == 0:
                invocation += 1
                if form == "default_context":
                    schedule()
                elif form == "explicit_context":
                    sc.next_trigger(context)
                elif base_form == "event":
                    schedule(self.event_a)
                elif base_form == "or_list":
                    schedule(self.or_list)
                elif base_form == "and_list":
                    schedule(self.and_list)
                elif base_form == "time_object":
                    schedule(sc.sc_time(4, sc.SC_NS))
                elif base_form == "time_number":
                    schedule(5, sc.SC_NS)
                elif base_form == "time_event":
                    schedule(sc.sc_time(5, sc.SC_NS), self.event_a)
                elif base_form == "number_event":
                    schedule(6, sc.SC_NS, self.event_b)
                elif base_form == "time_or_list":
                    schedule(sc.sc_time(6, sc.SC_NS), self.or_list)
                elif base_form == "number_or_list":
                    schedule(6, sc.SC_NS, self.or_list)
                elif base_form == "time_and_list":
                    schedule(sc.sc_time(6, sc.SC_NS), self.and_list)
                elif base_form == "number_and_list":
                    schedule(6, sc.SC_NS, self.and_list)
                elif base_form == "or_expression":
                    schedule(self.event_a | self.event_b)
                elif base_form == "and_expression":
                    schedule(self.event_a & self.event_b)
                elif base_form == "time_or_expression":
                    schedule(sc.sc_time(6, sc.SC_NS), self.event_a | self.event_b)
                elif base_form == "number_or_expression":
                    schedule(6, sc.SC_NS, self.event_a | self.event_b)
                elif base_form == "time_and_expression":
                    schedule(sc.sc_time(6, sc.SC_NS), self.event_a & self.event_b)
                elif base_form == "number_and_expression":
                    schedule(6, sc.SC_NS, self.event_a & self.event_b)
                else:
                    raise AssertionError(f"unhandled next_trigger overload {form}")
                return
            if invocation == 1:
                invocation += 1
                if not no_arguments:
                    record(trigger_records, f"trigger_{form}_dynamic")
                    schedule()
                else:
                    record(trigger_records, f"trigger_{form}_static")
                return
            if invocation == 2:
                invocation += 1
                record(trigger_records, f"trigger_{form}_restored")
                return
            raise AssertionError(f"unexpected extra method invocation for {form}")

        return trigger

    def drive(self):
        """Notify event waiters in order and later restore static methods."""
        sc.wait(2, sc.SC_NS)
        self.event_a.notify()
        sc.wait(1, sc.SC_NS)
        self.event_b.notify()
        sc.wait(17, sc.SC_NS)
        self.static_event.notify()

    def explicit_wait_cleanup(self):
        """Leave cleanup evidence when explicit-context wait is reset and killed."""
        try:
            sc.wait(self.cleanup_event, sc.sc_get_curr_simcontext())
        finally:
            wait_cleanup_records.append(
                (
                    len(wait_cleanup_records),
                    int(sc.sc_time_stamp().value()),
                    int(sc.sc_delta_count()),
                )
            )

    def control_cleanup(self):
        """Reset then kill the thread suspended in an explicit-context event wait."""
        sc.wait(1, sc.SC_NS)
        self.cleanup_handle.reset()
        sc.wait(1, sc.SC_NS)
        self.cleanup_handle.kill()

    def invalid_wait_method(self):
        """Capture the native error from calling wait in a method process."""
        try:
            sc.wait(1, sc.SC_NS)
        except sc.sc_report as error:
            error_records.append(
                (
                    "method_wait",
                    int(error.get_id()),
                    int(error.get_severity()),
                    error.get_msg_type(),
                    " ".join(error.get_msg().split()),
                )
            )
        else:
            raise AssertionError("wait from an SC_METHOD unexpectedly succeeded")

    def invalid_next_trigger_thread(self):
        """Capture the native error from calling next_trigger in a thread."""
        try:
            sc.next_trigger(sc.sc_get_curr_simcontext())
        except sc.sc_report as error:
            assert error.get_id() == 520
            assert error.get_msg_type() == sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_
            error_records.append(
                (
                    "thread_next_trigger",
                    error.get_id(),
                    int(error.get_severity()),
                    error.get_msg_type(),
                    " ".join(error.get_msg().split()),
                )
            )
        else:
            raise AssertionError("next_trigger from an SC_THREAD unexpectedly succeeded")

    def stopper(self):
        """Stop after all static-sensitivity restoration callbacks complete."""
        sc.wait(21, sc.SC_NS)
        sc.sc_stop()


clock = sc.sc_clock("wait_trigger_clock", sc.sc_time(2, sc.SC_NS))
sc.sc_report_handler.set_actions(sc.SC_ID_WAIT_NOT_ALLOWED_, sc.SC_ERROR, sc.SC_THROW)
sc.sc_report_handler.set_actions(sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, sc.SC_ERROR, sc.SC_THROW)
model = Model("wait_next_public", clock)
sc.sc_start()

wait_times = {label: time for label, time, _delta in wait_records}
expected_wait_times = {
    "wait_no_arguments": 2000,
    "wait_cycles": 4000,
    "wait_no_arguments_explicit": 2000,
    "wait_cycles_explicit": 4000,
}
base_wait_times = {
    "time_object": 3000,
    "time_number": 4000,
    "event": 2000,
    "time_event": 2000,
    "number_event": 3000,
    "or_list": 2000,
    "time_or_list": 2000,
    "number_or_list": 2000,
    "and_list": 3000,
    "time_and_list": 3000,
    "number_and_list": 3000,
    "or_expression": 2000,
    "time_or_expression": 2000,
    "number_or_expression": 2000,
    "and_expression": 3000,
    "time_and_expression": 3000,
    "number_and_expression": 3000,
}
for form, time in base_wait_times.items():
    expected_wait_times[f"wait_{form}_default"] = time
    expected_wait_times[f"wait_{form}_explicit"] = time
assert wait_times == expected_wait_times, wait_records
assert len(trigger_records) == 2 * len(TRIGGER_FORMS) - 2, trigger_records
assert [time for _index, time, _delta in wait_cleanup_records] == [1000, 2000], wait_cleanup_records
assert [(row[0], row[1], row[2]) for row in error_records] == [
    ("method_wait", 519, sc.SC_ERROR),
    ("thread_next_trigger", 520, sc.SC_ERROR),
], error_records

print("WAIT_NEXT_WAIT_REFERENCE", *(f"{label}@{time}/{delta}" for label, time, delta in wait_records))
print("WAIT_NEXT_TRIGGER_REFERENCE", *(f"{label}@{time}/{delta}" for label, time, delta in trigger_records))
print(
    "WAIT_NEXT_CLEANUP_REFERENCE",
    f"reset@{wait_cleanup_records[0][1]}/{wait_cleanup_records[0][2]}",
    f"kill@{wait_cleanup_records[1][1]}/{wait_cleanup_records[1][2]}",
)
for row in error_records:
    print("WAIT_NEXT_ERROR_REFERENCE", *row, sep="|")
