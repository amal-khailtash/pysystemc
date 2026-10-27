"""Compare selected IEEE 1666 deprecation reports with native SystemC."""

from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef("namespace sc_core { SC_API void sc_cycle(const sc_time& duration); }")


scenario = sys.argv[1]
assert scenario in {"simcontext", "cthread", "module", "random_seed"}, scenario
records = []
current_label = ""


def normalized(message):
    """Normalize native report whitespace without changing its words."""
    return " ".join(message.split())


def basename(filename):
    """Return a portable source basename from a native report location."""
    return Path(str(filename)).name


def capture(report, actions):
    """Copy report fields and kernel state while the report remains borrowed.

    Args:
        report: Native report borrowed for this callback.
        actions: Native report action mask selected by SystemC.
    """
    message_type = report.get_msg_type()
    if message_type not in (sc.SC_ID_IEEE_1666_DEPRECATION_, sc.SC_ID_NOT_IMPLEMENTED_):
        return
    assert current_label, "deprecation report arrived outside a labeled native call"
    records.append(
        f"DEPRECATION_REPORT {current_label}|{report.get_id()}|{int(report.get_severity())}|{message_type}|"
        f"{normalized(report.get_msg())}|{basename(report.get_file_name())}|{report.get_line_number()}|"
        f"{report.get_time().value()}|{int(report.get_verbosity())}|"
        f"{sc.sc_report_handler.get_count(message_type)}|{sc.sc_delta_count()}|"
        f"{int(sc.sc_get_status())}|{int(actions)}"
    )


def label_call(label, operation):
    """Set the report label and invoke one deprecation branch.

    Args:
        label: Stable native/Python operation label.
        operation: Zero-argument callable for the native API.

    Returns:
        The native operation result.
    """
    global current_label
    current_label = label
    return operation()


def run_simcontext():
    """Exercise deprecated context methods and simulation lifecycle functions."""
    name = sc.sc_module_name("top")
    top = sc.sc_module(name)
    del name
    context = sc.sc_get_curr_simcontext()
    assert label_call("find_object", lambda: context.find_object("top"))
    assert label_call("get_child_objects", context.get_child_objects).size() > 0
    assert label_call("delta_count", context.delta_count) == sc.sc_delta_count()
    assert bool(label_call("is_running", context.is_running)) == bool(sc.sc_is_running())
    label_call("initialize", sc.sc_initialize)
    label_call("cycle", lambda: cppyy.gbl.sc_core.sc_cycle(sc.sc_time(1, sc.SC_NS)))
    simulation_time = label_call("simulation_time", sc.sc_simulation_time)
    assert simulation_time == sc.sc_time_stamp().to_default_time_units()
    assert sc.sc_time_stamp() == sc.sc_time(1, sc.SC_NS)
    assert sc.sc_get_status() == sc.SC_PAUSED
    del top


class CthreadModel(sc.sc_module):
    """Exercise deprecation reports from a clocked thread and its waits."""

    def __init__(self):
        """Register one CTHREAD and a driver that releases both event waits."""
        name = sc.sc_module_name("cthread_model")
        super().__init__(name)
        self.clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
        self.clock_input = sc.sc_in[bool]("clock_input")
        self.clock_input.bind(self.clock)
        self.first = sc.sc_event("first")
        self.second = sc.sc_event("second")
        self.finished = False
        self.sc_cthread("worker", self.worker, self.clock_input.pos())
        self.sc_thread("driver", self.driver)
        del name

    def worker(self):
        """Call the deprecated process handle, CTHREAD waits, and timeout API."""
        process = label_call("current_process_handle", sc.sc_get_curr_process_handle)
        assert process and process.name() == sc.sc_get_current_process_handle().name()
        label_call("cthread_general_wait", lambda: sc.wait(self.first))
        event_list = self.first | self.second
        label_call("cthread_event_list_wait", lambda: sc.wait(event_list))
        label_call("timed_out", sc.timed_out)
        self.finished = True

    def driver(self):
        """Notify each event between clock edges for deterministic CTHREAD waits."""
        sc.wait(1, sc.SC_NS)
        self.first.notify(sc.SC_ZERO_TIME)
        sc.wait(2, sc.SC_NS)
        self.second.notify(sc.SC_ZERO_TIME)


class BindingChild(sc.sc_module):
    """Provide two input ports to exercise each deprecated module binding path."""

    def __init__(self, name):
        """Create two input ports inside the child module's hierarchy scope."""
        super().__init__(name)
        self.first = sc.sc_in[bool]("first")
        self.second = sc.sc_in[bool]("second")


def run_cthread():
    """Run the clocked wait model and require all deprecated sites to complete."""
    model = CthreadModel()
    sc.sc_start(sc.sc_time(6, sc.SC_NS))
    assert model.finished
    assert sc.sc_time_stamp() == sc.sc_time(6, sc.SC_NS)
    assert sc.sc_get_status() == sc.SC_PAUSED


def run_module():
    """Trigger positional shift and repeated call-operator binding reports."""
    top_name = sc.sc_module_name("binding_top")
    top = sc.sc_module(top_name)
    del top_name
    signal = sc.sc_signal[bool]("signal", False)
    positional_name = sc.sc_module_name("positional")
    positional = BindingChild(positional_name)
    del positional_name
    repeated_name = sc.sc_module_name("repeated")
    repeated = BindingChild(repeated_name)
    del repeated_name
    global current_label
    current_label = "positional_shift"
    positional.__lshift__(signal)
    current_label = "repeat_first"
    repeated(signal)
    current_label = "repeat_second"
    repeated(signal)
    sc.sc_start(sc.SC_ZERO_TIME)
    assert sc.sc_get_status() == sc.SC_PAUSED and sc.sc_time_stamp() == sc.SC_ZERO_TIME
    del top, positional, repeated, signal


def run_random_seed():
    """Capture the warning from the safe native seed stub and recover normally."""
    label_call("random_seed", lambda: sc.sc_set_random_seed(123))
    assert sc.sc_report_handler.get_count(sc.SC_ID_NOT_IMPLEMENTED_) == 1


sc.set_report_handler(capture)
sc.sc_report_handler.set_actions(sc.SC_ID_IEEE_1666_DEPRECATION_, sc.SC_INFO, sc.SC_DISPLAY)
sc.sc_report_handler.set_actions(sc.SC_ID_NOT_IMPLEMENTED_, sc.SC_WARNING, sc.SC_DISPLAY)

expected_reports = {"simcontext": 7, "cthread": 4, "module": 2, "random_seed": 1}[scenario]
if scenario == "simcontext":
    run_simcontext()
elif scenario == "cthread":
    run_cthread()
elif scenario == "module":
    run_module()
else:
    run_random_seed()

message_type = sc.SC_ID_NOT_IMPLEMENTED_ if scenario == "random_seed" else sc.SC_ID_IEEE_1666_DEPRECATION_
count = sc.sc_report_handler.get_count(message_type)
assert count == expected_reports and len(records) == expected_reports, (count, len(records))
records.append(
    f"DEPRECATION_FINAL {scenario}|{count}|{sc.sc_time_stamp().value()}|{sc.sc_delta_count()}|{int(sc.sc_get_status())}"
)
sc.set_report_handler()
for record in records:
    print(record)
print(f"DEPRECATION_OK {scenario} reports={expected_reports}")
