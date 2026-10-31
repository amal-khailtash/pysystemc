"""Verify native report text survives help formatting and adapter failures."""

from io import StringIO
from pathlib import Path

from amal.eda import systemc
from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt as dt

# Import SystemC before cppyy declarations so package initialization owns setup.
import cppyy


MESSAGE = (
    "native diagnostic | sc_core::python_member_wait | pysystemc_vectors::member_0 | "
    "pysystemc_datatypes::fixed_number_bits"
)
FILE = "presentation-report-preservation.cpp"
LINE = 317
REPORTS = []


def hex_text(value):
    """Encode UTF-8 text as lowercase hex for byte-exact native comparison.

    Args:
        value: Text copied from a native report field.

    Returns:
        Lowercase hexadecimal text containing the original UTF-8 bytes.
    """
    return value.encode("utf-8").hex()


def capture(report, actions):
    """Copy scalar report fields while the native callback borrow is valid.

    Args:
        report: Native report borrowed until this synchronous callback returns.
        actions: Configured native action mask passed to the handler.

    The callback retains only Python strings and integers, never the borrowed
    report object or any native pointer.
    """
    REPORTS.append(
        (
            report.get_msg_type(),
            report.get_msg(),
            report.get_file_name(),
            int(report.get_severity()),
            int(report.get_id()),
            int(report.get_line_number()),
            int(actions),
        )
    )


def report_line(label, record, count):
    """Render one copied callback record in the native reference format.

    Args:
        label: Stable boundary name shared with the C++ reference.
        record: Copied report fields and configured action from ``capture``.
        count: Post-dispatch count for this native message type.

    Returns:
        A tab-delimited row whose text fields contain their exact UTF-8 bytes.
    """
    message_type, message, file_name, severity, report_id, line, actions = record
    return (
        f"PRES_REPORT\t{label}\t{severity}\t{report_id}\t{hex_text(message_type)}\t"
        f"{hex_text(message)}\t{hex_text(file_name)}\t{line}\t{actions}\t{count}"
    )


def expect_adapter_error(label, operation, expected_word, *, cause_type=None):
    """Require a known public argument-shape failure before the next report.

    Args:
        label: Public operation name used in the failure assertion.
        operation: Callable that exercises the invalid public argument form.
        expected_word: Word required in the adapter's diagnostic.
        cause_type: Optional native/Python cause class required for translation.

    Returns:
        The caught adapter exception for identity and traceback checks.

    Raises:
        AssertionError: The operation succeeds or raises an unrelated diagnostic.
    """
    try:
        operation()
    except TypeError as error:
        assert expected_word in str(error).lower(), str(error)
        if cause_type is not None:
            assert isinstance(error.__cause__, cause_type), error.__cause__
            assert "missing" in str(error.__cause__).lower(), str(error.__cause__)
        return error
    raise AssertionError(f"{label} unexpectedly accepted an invalid public form")


def emit_native_report(handler, message_type, label):
    """Invoke a native warning and verify its copied fields and action.

    Args:
        handler: Native SystemC report handler.
        message_type: Unique native report type for this boundary.
        label: Stable label shared with the C++ reference.

    Returns:
        One tab-delimited record matching the independent C++ model.

    Raises:
        AssertionError: The native report changes message, location, action or count.
    """
    previous_actions = handler.set_actions(message_type, sc.SC_WARNING, sc.SC_DO_NOTHING)
    try:
        handler.report(sc.SC_WARNING, message_type, MESSAGE, FILE, LINE)
        record = REPORTS[-1]
        assert record[:3] == (message_type, MESSAGE, FILE)
        assert record[3] == int(sc.SC_WARNING)
        assert record[5:] == (LINE, int(sc.SC_DO_NOTHING))
        count = int(handler.get_count(message_type))
        assert count == 1
        return report_line(label, record, count)
    finally:
        handler.set_actions(message_type, sc.SC_WARNING, previous_actions)


def main():
    """Assert native report preservation across presentation and adapter calls."""
    handler = sc.sc_report_handler
    saved_handler = handler.get_handler()
    assert saved_handler
    old_status = sc.sc_get_status()
    old_time = sc.sc_time_stamp().value()
    sc.set_report_handler(capture)

    types = (
        "presentation/native_before",
        "presentation/native_after_help",
        "presentation/native_after_vector_error",
        "presentation/native_after_fixed_error",
    )
    output = []
    try:
        output.append(emit_native_report(handler, types[0], "before"))

        formatted = systemc.format_native_help(dt.sc_uint[73])
        assert "sc_dt::sc_uint<73>" in formatted
        help_output = StringIO()
        assert systemc.native_help(dt.sc_uint[73], file=help_output) is None
        assert help_output.getvalue() == formatted + "\n"
        output.append(emit_native_report(handler, types[1], "after_help"))

        cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_core_vector_access_probe.h"))
        element = cppyy.gbl.sc_core_vector_access_probe.Element
        elements = sc.sc_vector[element]("report_preservation_elements", 1)
        vector_error = expect_adapter_error(
            "sc_vector.assemble",
            lambda: elements.assemble("missing"),
            "member",
            cause_type=AttributeError,
        )
        assert vector_error.__traceback__ is not None
        output.append(emit_native_report(handler, types[2], "after_vector_error"))

        parameters = dt.sc_fxtype_params(17, 9, dt.SC_TRN, dt.SC_WRAP, 0)
        fixed_error = expect_adapter_error(
            "sc_fix constructor", lambda: dt.sc_fix(parameters, dt.SC_TC_), "constructor"
        )
        assert fixed_error.__cause__ is None
        assert fixed_error.__traceback__ is not None
        output.append(emit_native_report(handler, types[3], "after_fixed_error"))
    finally:
        sc.set_report_handler()
        assert handler.set_handler(saved_handler)

    throw_type = "presentation/native_throw"
    throw_message = "native thrown diagnostic | sc_dt::python_fixed_constructor"
    old_actions = handler.set_actions(throw_type, sc.SC_ERROR, sc.SC_THROW)
    try:
        handler.report(sc.SC_ERROR, throw_type, throw_message, FILE, LINE + 1)
    except sc.sc_report as report:
        assert report.get_severity() == sc.SC_ERROR
        assert report.get_id() == -1
        assert report.get_msg_type() == throw_type
        assert report.get_msg() == throw_message
        assert report.get_file_name() == FILE
        assert report.get_line_number() == LINE + 1
        assert handler.get_count(throw_type) == 1
        output.append(
            f"PRES_THROW\t{int(report.get_severity())}\t{int(report.get_id())}\t"
            f"{hex_text(report.get_msg_type())}\t{hex_text(report.get_msg())}\t"
            f"{hex_text(report.get_file_name())}\t{int(report.get_line_number())}\t"
            f"{int(sc.SC_THROW)}\t{int(handler.get_count(throw_type))}"
        )
    else:
        raise AssertionError("configured SC_THROW action returned without a native sc_report")
    finally:
        handler.set_actions(throw_type, sc.SC_ERROR, old_actions)

    assert sc.sc_get_status() == old_status
    assert sc.sc_time_stamp().value() == old_time
    for line in output:
        print(line)
    print("PRESENTATION_REPORT_PRESERVATION_OK")


main()
