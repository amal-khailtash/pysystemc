"""Exercise native assertion reports and abort only in isolated processes."""

import resource
import sys

from amal.eda.systemc import sc_core as sc

resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
mode = sys.argv[1]
if mode == "throw":
    sc.sc_report_handler.set_actions(sc.SC_FATAL, sc.SC_THROW)
    try:
        sc.sc_assertion_failed("sample assertion", "portable_probe.cpp", 17)
    except sc.sc_report as report:
        assert report.get_severity() == sc.SC_FATAL
        assert report.get_msg() == "sample assertion" and report.get_line_number() == 17
        print(
            "FATAL_THROW",
            report.get_id(),
            report.get_msg_type(),
            report.get_file_name(),
            report.get_line_number(),
            sep="|",
        )
    else:
        raise AssertionError("assertion failure returned normally")
else:
    print("FATAL_ENTER", mode, flush=True)
    if mode == "abort":
        sc.sc_abort()
    else:
        sc.sc_assertion_failed("sample assertion", "portable_probe.cpp", 17)
    raise AssertionError("native abort returned normally")
