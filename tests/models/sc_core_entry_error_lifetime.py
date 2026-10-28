"""Verify native entry cleanup when Python report callbacks raise."""

import gc
from pathlib import Path
import sys
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy


scenario = sys.argv[1]
header = Path(__file__).parents[1] / "native" / "sc_core_entry_error_lifetime.h"
source = header.read_text(encoding="utf-8")
cppyy.cppdef("#define sc_main pysystemc_cpp_entry_error\n" + source + "\n#undef sc_main")
cppyy.cppdef("pysystemc::native_model_entry = &pysystemc_cpp_entry_error;")

calls = []


def report_callback(report, actions):
    """Record report dispatch and fail the selected Python callback boundary."""
    calls.append((report.get_msg_type(), report.get_msg(), int(actions)))
    if scenario == "deprecation" and len(calls) == 1:
        return
    raise ValueError(f"entry {scenario} callback failure")


callback_ref = weakref.ref(report_callback)
sc.set_report_handler(report_callback)
failure = None
try:
    try:
        sc.sc_elab_and_sim(2, ["entry-lifetime", scenario])
    except BaseException as error:
        failure = error
    else:
        failure = AssertionError("native entry failure did not reach the Python callback")
finally:
    cppyy.cppdef("pysystemc::native_model_entry = nullptr;")
    sc.set_report_handler()

expected_calls = 2 if scenario == "deprecation" else 1
assert len(calls) == expected_calls, (calls, type(failure), failure)
assert isinstance(failure, ValueError), (type(failure), failure)
assert f"entry {scenario} callback failure" in str(failure), (type(failure), failure)
expected_report = {
    "direct": "direct callback report",
    "report": "native entry report failure",
    "runtime": "native entry runtime failure",
    "deprecation": "entry deprecation counter marker",
}[scenario]
assert expected_report in calls[0][1], calls[0]
if scenario == "deprecation":
    assert calls[1][0] == sc.SC_ID_IEEE_1666_DEPRECATION_, calls[1]
    assert "turn off warnings" in calls[1][1], calls[1]
assert not bool(cppyy.gbl.sc_core.sc_in_action)
try:
    sc.sc_argv()
except RuntimeError:
    pass
else:
    raise AssertionError("native argv remained active after entry failure")

del report_callback
# The propagated exception traceback includes the callback frame; drop it before
# asserting the handler's retained callback owner has been released.
del failure
gc.collect()
assert callback_ref() is None
print(f"ENTRY_ERROR_LIFETIME_OK {scenario} calls={len(calls)} reset=1 released=1")
