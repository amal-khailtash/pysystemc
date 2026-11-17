"""Cover public native method help and instance completion at their boundaries."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]


def test_valid_overloaded_method_help_and_instance_completion(tmp_path: Path) -> None:
    """Keep native overload docs and complete valid methods on native objects."""
    probe = r"""
import amal.eda.systemc as systemc
from amal.eda.systemc import sc_core
from amal.eda.systemc.repl import PythonNamespaceCompleter
from prompt_toolkit.completion import CompleteEvent
from prompt_toolkit.document import Document

import cppyy

native_core = cppyy.gbl.sc_core
native_event_type = sc_core.sc_event
assert native_event_type is native_core.sc_event
finder_type = sc_core.sc_event_finder
finder_init = finder_type.__init__
finder_init_doc = finder_init.__doc__
assert finder_type is native_core.sc_event_finder
finder_help = systemc.format_native_help(finder_type)
finder_init_help = systemc.format_native_help(finder_init)
assert "Native help: sc_core::sc_event_finder" in finder_help
assert "sc_event_finder" in finder_init_help, finder_init_help
assert finder_type.__init__.__doc__ == finder_init_doc
time_before = native_core.sc_time_stamp()
status_before = native_core.sc_get_status()

event = sc_core.sc_event("presentation_help_event")
notify = event.notify
notify_doc = notify.__doc__
notify_cpp_name = getattr(notify, "__cpp_name__", None)
rendered = systemc.format_native_help(notify)
signatures = [line for line in rendered.splitlines() if "sc_event::notify(" in line]
assert len(signatures) == 3, rendered
assert any("sc_event::notify()" in line for line in signatures), rendered
assert any("sc_event::notify(const sc_core::sc_time&)" in line for line in signatures), rendered
assert any("sc_event::notify(double, sc_core::sc_time_unit)" in line for line in signatures), rendered
assert "python_" not in "\n".join(signatures), rendered
assert notify.__doc__ == notify_doc
assert getattr(notify, "__cpp_name__", None) == notify_cpp_name
assert event.notify is not None

duration = sc_core.sc_time(2, sc_core.SC_NS)
completer = PythonNamespaceCompleter({"duration": duration})
completions = {
    item.text
    for item in completer.get_completions(Document("duration.to_sec"), CompleteEvent())
}
assert "duration.to_seconds(" in completions or "duration.to_seconds" in completions, completions
assert callable(duration.to_seconds)
assert native_core.sc_time_stamp() == time_before
assert native_core.sc_get_status() == status_before
print("SECTION1_HELP_COMPLETION_OK")
"""
    env = dict(os.environ)
    env.setdefault("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))
    env["COVERAGE_FILE"] = str(tmp_path.resolve() / "coverage.section1_help_completion")
    result = subprocess.run(
        [sys.executable, "-c", probe],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=60,
        env=env,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "SECTION1_HELP_COMPLETION_OK" in result.stdout
