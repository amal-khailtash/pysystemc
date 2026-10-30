"""Exercise public sc_report assignment and time-varying resolved reads."""

from amal.eda.systemc import sc_core as sc, sc_dt as dt


def hex_text(value):
    """Encode text as lowercase UTF-8 hex for native/Python output parity.

    Args:
        value: Text to encode.

    Returns:
        Lowercase hexadecimal representation of its UTF-8 bytes.
    """
    return value.encode("utf-8").hex()


reports = []


def capture_report(report, _actions):
    """Keep an owning copy of the report borrowed during native dispatch.

    Args:
        report: Borrowed native report valid for the callback duration.
        _actions: Native dispatch actions, unused by this test.
    """
    reports.append(sc.sc_report(report))


sc.set_report_handler(capture_report)
sc.sc_report_handler.report(sc.SC_INFO, "assignment/destination", "old", "dest.cpp", 11)
sc.sc_report_handler.report(sc.SC_WARNING, "assignment/source", "copied", "source.cpp", 73)
sc.sc_report_handler.report(sc.SC_ERROR, "assignment/alias-target", "alias", "alias.cpp", 97)
sc.set_report_handler()
assert len(reports) == 3

destination, source, alias_target = reports
source_snapshot = (
    source.get_id(),
    source.get_severity(),
    source.get_msg_type(),
    source.get_msg(),
    source.get_file_name(),
    source.get_line_number(),
    source.get_time().value(),
    source.get_verbosity(),
    source.what(),
)
returned = destination.__assign__(source)
assert destination.get_id() == source.get_id() == -1
assert destination.get_severity() == source.get_severity() == sc.SC_WARNING
assert destination.get_msg_type() == source.get_msg_type() == "assignment/source"
assert destination.get_msg() == source.get_msg() == "copied"
assert destination.get_file_name() == source.get_file_name() == "source.cpp"
assert destination.get_line_number() == source.get_line_number() == 73
assert destination.get_verbosity() == source.get_verbosity()
assert destination.get_time().value() == source.get_time().value() == 0
assert destination.what() == source.what()
# Mutating through the returned reference must mutate the original destination.
returned.__assign__(alias_target)
assert destination.get_msg_type() == alias_target.get_msg_type()
assert destination.get_msg() == alias_target.get_msg()
assert destination.get_file_name() == alias_target.get_file_name()
assert destination.get_line_number() == alias_target.get_line_number()
assert source_snapshot == (
    source.get_id(),
    source.get_severity(),
    source.get_msg_type(),
    source.get_msg(),
    source.get_file_name(),
    source.get_line_number(),
    source.get_time().value(),
    source.get_verbosity(),
    source.what(),
)
destination.__assign__(source)
print(
    "REPORT_ASSIGN",
    1,
    destination.get_id(),
    int(destination.get_severity()),
    destination.get_msg_type(),
    destination.get_msg(),
    destination.get_file_name(),
    destination.get_line_number(),
    destination.get_time().value(),
    destination.get_verbosity(),
    hex_text(destination.what()),
)

before_self_assignment = destination.what()
returned_self = destination.__assign__(destination)
assert destination.what() == before_self_assignment
returned_self.__assign__(alias_target)
assert destination.what() == alias_target.what()
destination.__assign__(source)
print("REPORT_SELF", 1, int(destination.what() == before_self_assignment))


class ResolvedModel(sc.sc_module):
    """Own two independent resolved writers and a transition observer."""

    def __init__(self, name):
        """Register finite writer threads and a noninitializing observer.

        Args:
            name: Native module name.
        """
        super().__init__(sc.sc_module_name(name))
        self.signal = sc.sc_signal_resolved("resolved")
        self.samples = []
        self.sc_thread("writer_zero", self.writer_zero)
        self.sc_thread("writer_one", self.writer_one)
        self.sc_method("observe", self.observe)
        self.sensitive(self.signal)
        self.dont_initialize()

    def writer_zero(self):
        """Drive zero at 1 ns, then release that driver at 3 ns."""
        sc.wait(1, sc.SC_NS)
        self.signal.write(dt.sc_logic("0"))
        sc.wait(2, sc.SC_NS)
        self.signal.write(dt.sc_logic("Z"))

    def writer_one(self):
        """Drive Z, one, then zero at 1 ns, 2 ns, and 4 ns."""
        sc.wait(1, sc.SC_NS)
        self.signal.write(dt.sc_logic("Z"))
        sc.wait(1, sc.SC_NS)
        self.signal.write(dt.sc_logic("1"))
        sc.wait(2, sc.SC_NS)
        self.signal.write(dt.sc_logic("0"))

    def observe(self):
        """Read the resolved value and native timestamp after each update."""
        value = self.signal.read()
        sample = (str(sc.sc_time_stamp()), value.to_char())
        self.samples.append(sample)
        print("RESOLVED_READ", *sample)


model = ResolvedModel("resolved_model")
sc.sc_start(5, sc.SC_NS)
assert model.samples == [
    ("1 ns", "0"),
    ("2 ns", "X"),
    ("3 ns", "1"),
    ("4 ns", "0"),
], model.samples
final_value = model.signal.read()
assert final_value.to_char() == "0"
print("RESOLVED_FINAL", sc.sc_time_stamp(), final_value.to_char())
sc.sc_stop()
