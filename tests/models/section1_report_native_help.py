"""Exercise curated sc_report help against real cppyy proxies in a child process."""

import builtins
from io import StringIO
import pydoc

import amal.eda.systemc as systemc

# Package initialization must precede direct cppyy access.
import cppyy

from amal.eda.systemc import sc_core


def check_exact_proxy_registry():
    """Check the exported class's exact-key registry and unchanged proxy surfaces."""
    report_type = sc_core.sc_report
    native_report_type = cppyy.gbl.sc_core.sc_report
    assert report_type is native_report_type
    assert report_type.__cpp_name__ == "sc_core::sc_report"

    original_identity = id(report_type)
    original_dir = dir(report_type)
    original_doc = report_type.__doc__
    original_help = pydoc.render_doc(report_type, renderer=pydoc.plaintext)
    original_builtin_help = builtins.help

    rendered = systemc.format_native_help(report_type)
    expected_names = (
        "get_file_name",
        "get_id",
        "get_line_number",
        "get_msg",
        "get_msg_type",
        "get_process_name",
        "get_severity",
        "get_time",
        "get_verbosity",
        "swap",
        "valid",
        "what",
    )
    assert "Curated public declarations from submodules/systemc/src/sysc/utils/sc_report.h" in rendered
    signature_lines = [line for line in rendered.splitlines() if "sc_core::sc_report::" in line]
    assert len(signature_lines) == 12, rendered
    for method_name in expected_names:
        assert any(f"sc_core::sc_report::{method_name}(" in line for line in signature_lines), rendered
        assert not hasattr(report_type, method_name)
        assert method_name not in dir(report_type)
    assert "get_time() returns a borrowed const sc_time&" in rendered
    assert "timestamp is captured at report construction" in rendered
    assert "Copy construction gives the copy its own timestamp value" in rendered
    assert "Assignment uses copy-and-swap" in rendered
    assert "does not add class attributes or change class dir()" in rendered

    output = StringIO()
    assert systemc.native_help(report_type, file=output) is None
    assert output.getvalue() == rendered + "\n"
    assert id(report_type) == original_identity
    assert report_type is native_report_type
    assert report_type.__doc__ == original_doc
    assert dir(report_type) == original_dir
    assert pydoc.render_doc(report_type, renderer=pydoc.plaintext) == original_help
    assert builtins.help is original_builtin_help
    assert all(f"sc_report::{method_name}(" not in original_help for method_name in expected_names)

    class WrongNativeClass:
        """Class proxy-shaped object with a nonmatching C++ name."""

        __cpp_name__ = "sc_core::sc_report_extra"

    class ExactMetadataOnlyClass:
        """Metadata-only class proxy shape carrying the exact registry key."""

        __cpp_name__ = "sc_core::sc_report"

    class ExactNamedObject:
        """Instance-shaped object must not trigger a class-only registry entry."""

        __cpp_name__ = "sc_core::sc_report"

    wrong_rendered = systemc.format_native_help(WrongNativeClass)
    exact_rendered = systemc.format_native_help(ExactMetadataOnlyClass)
    instance_rendered = systemc.format_native_help(ExactNamedObject())
    assert "Curated public declarations" not in wrong_rendered
    assert "Curated public declarations" in exact_rendered
    assert "Curated public declarations" not in instance_rendered

    class CallGuardMeta(type):
        """Reject constructor and member metadata reads on the registry fast path."""

        def __getattribute__(cls, attribute):
            """Allow only the exact C++ name needed for registry lookup."""
            forbidden = {
                "__doc__",
                "__init__",
                "get_file_name",
                "get_id",
                "get_line_number",
                "get_msg",
                "get_msg_type",
                "get_process_name",
                "get_severity",
                "get_time",
                "get_verbosity",
                "swap",
                "valid",
                "what",
            }
            if attribute in forbidden:
                raise AssertionError(f"formatter inspected forbidden metadata: {attribute}")
            return super().__getattribute__(attribute)

    class CallGuardClass(metaclass=CallGuardMeta):
        """Class-shaped proxy proving formatting does not inspect members."""

        __cpp_name__ = "sc_core::sc_report"

    guarded_rendered = systemc.format_native_help(CallGuardClass)
    assert "Curated public declarations" in guarded_rendered


def check_existing_overload_path():
    """Confirm a native bound overload keeps all original candidate docs."""
    event = sc_core.sc_event("section1_report_help_event")
    notify = event.notify
    before_doc = notify.__doc__
    rendered = systemc.format_native_help(notify)
    assert "sc_event::notify()" in rendered
    assert "sc_event::notify(const sc_core::sc_time&)" in rendered
    assert "sc_event::notify(double, sc_core::sc_time_unit)" in rendered
    assert notify.__doc__ == before_doc
    assert event.notify is not None


def check_existing_report_member_help():
    """Retain metadata-only help for all bound methods on a live report instance."""
    observations = {}
    member_names = (
        "get_file_name",
        "get_id",
        "get_line_number",
        "get_msg",
        "get_msg_type",
        "get_process_name",
        "get_severity",
        "get_time",
        "get_verbosity",
        "swap",
        "valid",
        "what",
    )

    def capture(report, actions):
        """Format only the existing report member metadata during dispatch."""
        generic_help = systemc.format_native_help(report)
        members = {}
        for member_name in member_names:
            bound_member = getattr(report, member_name)
            original_doc = bound_member.__doc__
            rendered = systemc.format_native_help(bound_member)
            stream = StringIO()
            assert systemc.native_help(bound_member, file=stream) is None
            signature_lines = [line for line in rendered.splitlines() if f"sc_core::sc_report::{member_name}(" in line]
            assert signature_lines, rendered
            assert stream.getvalue() == rendered + "\n"
            assert bound_member.__doc__ == original_doc
            members[member_name] = signature_lines
        observations["generic_type"] = type(report).__name__
        observations["generic_help"] = generic_help
        observations["members"] = members

    sc_core.set_report_handler(capture)
    try:
        sc_core.sc_report_handler.report(
            sc_core.SC_INFO,
            "section1/report-native-help",
            "metadata-only bound-member probe",
            "section1_report_native_help.py",
            1,
        )
    finally:
        sc_core.set_report_handler()

    assert observations["generic_type"] == "CPPExcInstance"
    assert "Curated public declarations" not in observations["generic_help"]
    assert set(observations["members"]) == set(member_names)
    assert all(observations["members"].values())


def main():
    """Run metadata-only report help checks before returning an acceptance marker."""
    check_exact_proxy_registry()
    check_existing_overload_path()
    check_existing_report_member_help()
    print("SECTION1_REPORT_NATIVE_HELP_OK")


main()
