"""Exercise native diagnostic bytes, environment controls and write failures."""

import json
import os
import sys

import amal.eda.systemc as systemc

cppyy = systemc.cppyy
cppyy.cppdef("""
#include <system_error>
namespace native_logging_probe {
/** Emit copied bytes including embedded NUL; no Python callback or retained state. */
void bytes() { pysystemc::log(true, std::string("left\\0right", 10)); }
/** Return a write failure while clearing the borrowed stderr error indicator. */
std::string write_failure() {
  try { pysystemc::log(true, "write failure"); }
  catch (const std::system_error& error) { clearerr(stderr); return error.what(); }
  return "NO_ERROR";
}
}
""")
sys.stderr.write("NATIVE_LOGGING_START\n")
sys.stderr.flush()
for level, color, no_color in (
    ("INFO", "never", False),
    ("DEBUG", "never", False),
    ("DEBUG", "always", False),
    ("DEBUG", "always", True),
    ("DEBUG", "auto", True),
    ("DEBUG", "auto", False),
):
    os.environ["PYSYSTEMC_LOG_LEVEL"] = level
    os.environ["PYSYSTEMC_COLOR"] = color
    if no_color:
        os.environ["NO_COLOR"] = "1"
    else:
        os.environ.pop("NO_COLOR", None)
    cppyy.gbl.pysystemc.log(False, "debug %s {} café")
    cppyy.gbl.pysystemc.log(True, "error %s {} café")
os.environ["PYSYSTEMC_COLOR"] = "never"
cppyy.gbl.native_logging_probe.bytes()
sys.stderr.write("NATIVE_LOGGING_END\n")
sys.stderr.flush()
saved = os.dup(2)
try:
    with open(__file__, "rb", buffering=0) as readonly:
        os.dup2(readonly.fileno(), 2)
    error = str(cppyy.gbl.native_logging_probe.write_failure())
finally:
    os.dup2(saved, 2)
    os.close(saved)
assert "cannot write to file" in error, error
print("NATIVE_LOGGING_RESULT=" + json.dumps({"write_error": error}))
