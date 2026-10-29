"""Validate SystemC runner in isolated subprocesses."""

import json
import re
import os
import subprocess
import sys
from importlib.metadata import version as distribution_version

import pytest

from .support import ROOT


def test_cpp_runner_kernel_banner(tmp_path):
    """Native startup prints the kernel banner and honors its suppression flag."""
    command = [
        sys.executable,
        str(ROOT / "examples/run_sysc_cpp.py"),
        str(ROOT / "examples/helloworld/cpp/helloworld.cpp"),
    ]
    env = dict(os.environ)
    env.pop("SYSTEMC_DISABLE_COPYRIGHT_MESSAGE", None)
    env.pop("SC_COPYRIGHT_MESSAGE", None)
    result = subprocess.run(
        command, cwd=tmp_path, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout
    assert "[pysystemc:C++:info]" in result.stdout
    assert "Reading " in result.stdout
    assert "\n\n[pysystemc:C++:info] Reading " in result.stdout
    assert result.stdout.index("SystemC 2.3.4-Accellera") < result.stdout.index("Hello world using approach 1")
    env["SC_COPYRIGHT_MESSAGE"] = "DISABLE"
    result = subprocess.run(
        command, cwd=tmp_path, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout
    assert "SystemC 2.3.4-Accellera" not in result.stdout
    assert "Hello world using approach 2" in result.stdout


def _run_blocked_entrypoint(
    entry_name: str,
    arguments: list[str],
    *,
    environment: dict[str, str] | None = None,
) -> subprocess.CompletedProcess[str]:
    """Invoke an installed console entry in a fresh process with native imports blocked.

    Args:
        entry_name: Installed console script name.
        arguments: Arguments passed to that script.
        environment: Optional child environment.

    Returns:
        Captured child output and exit status.
    """
    script = r"""
import importlib.metadata
import json
import sys
from importlib.abc import MetaPathFinder


class NativeImportBlocker(MetaPathFinder):
    "Fail if a lightweight CLI path imports native modules."

    def find_spec(self, fullname, path=None, target=None):
        "Block SystemC package and cppyy imports."
        if fullname == "amal.eda.systemc" or fullname.startswith("amal.eda.systemc."):
            raise AssertionError(f"native package imported during CLI parsing: {fullname}")
        if fullname == "cppyy" or fullname.startswith("cppyy."):
            raise AssertionError(f"cppyy imported during CLI parsing: {fullname}")
        return None


sys.meta_path.insert(0, NativeImportBlocker())
scenario = json.loads(sys.argv[1])
distribution = importlib.metadata.distribution("pysystemc")
entry_point = next(
    entry
    for entry in distribution.entry_points
    if entry.group == "console_scripts" and entry.name == scenario["entry_name"]
)
sys.argv = [scenario["entry_name"], *scenario["arguments"]]
try:
    status = entry_point.load()()
except SystemExit as error:
    status = error.code
sys.exit(status)
"""
    child_environment = dict(os.environ)
    if environment is not None:
        child_environment.update(environment)
    scenario = json.dumps({"entry_name": entry_name, "arguments": arguments})
    return subprocess.run(
        [sys.executable, "-c", script, scenario],
        capture_output=True,
        text=True,
        env=child_environment,
        timeout=30,
    )


@pytest.mark.parametrize(
    ("arguments", "expected_status"),
    [
        ([], 0),
        (["-h"], 0),
        (["--help"], 0),
        (["-V"], 0),
        (["--version"], 0),
        (["version"], 0),
        (["version", "-h"], 0),
        (["help", "-h"], 0),
        (["help", "run"], 0),
        (["help", "help"], 0),
        (["help", "version"], 0),
        (["-I"], 2),
        (["--invalid-option", "model.cpp"], 2),
    ],
)
def test_installed_cpp_entrypoint_metadata_paths_stay_lightweight(arguments, expected_status):
    """Load the installed console entry point and reject eager native imports."""
    environment = dict(os.environ)
    environment["PYSYSTEMC_COLOR"] = "never"
    environment.pop("NO_COLOR", None)
    result = _run_blocked_entrypoint("systemc-cpp", arguments, environment=environment)
    output = result.stdout + result.stderr
    assert result.returncode == expected_status, output
    if arguments in (["-h"], ["--help"]) or not arguments:
        assert "Usage: systemc-cpp" in output
        assert "Run native SystemC C++ models" in output
    elif arguments in (["-V"], ["--version"]):
        assert output.startswith("systemc-cpp ")
    elif arguments == ["-I"]:
        assert "argument -I/--include: expected one argument" in output
    elif arguments == ["version"]:
        assert output.startswith("systemc-cpp v")
    elif arguments == ["version", "-h"]:
        assert "Display the installed SystemC C++ runner version and exit." in output
        assert "Usage: systemc-cpp version [OPTIONS]" in output
    elif arguments == ["help", "-h"]:
        assert "Show help for systemc-cpp or one of its commands." in output
        assert "Optional command (run, help, version)" in output
        assert "Usage: systemc-cpp help [OPTIONS] [<COMMAND>]" in output
    elif arguments[:1] == ["help"]:
        assert "Usage: systemc-cpp" in output
    else:
        assert "systemc-cpp: error:" in output

    assert "SystemC 2.3.4" not in output
    assert "Accellera" not in output
    assert "Copyright" not in output
    assert "OSCI/SystemC" not in output
    assert "Simulation stopped" not in output


@pytest.mark.parametrize(
    ("color_mode", "no_color", "expect_ansi"),
    [("always", True, True), ("never", False, False), ("auto", True, False)],
)
def test_installed_cpp_version_color_modes_stay_lightweight(color_mode, no_color, expect_ansi):
    """Render version colors according to environment without native startup."""
    environment = dict(os.environ)
    environment.pop("NO_COLOR", None)
    environment["PYSYSTEMC_COLOR"] = color_mode
    if no_color:
        environment["NO_COLOR"] = "1"
    result = _run_blocked_entrypoint("systemc-cpp", ["--version"], environment=environment)
    assert result.returncode == 0, result.stdout + result.stderr
    plain_output = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout)
    installed_version = distribution_version("pysystemc")
    assert plain_output == f"systemc-cpp v{installed_version}\n"
    if expect_ansi:
        assert re.search(r"\x1b\[35msystemc-cpp ?\x1b\[[0-9;]*m", result.stdout)
        assert re.search(r"\x1b\[36mv" + re.escape(installed_version) + r"\x1b\[[0-9;]*m", result.stdout)
    else:
        assert "\x1b[" not in result.stdout


def test_installed_cpp_help_uses_shared_usage_tokens_without_native_imports():
    """Preserve wrapped help wording and render each C++ usage token by syntax."""
    environment = dict(os.environ)
    environment["PYSYSTEMC_COLOR"] = "always"
    environment["NO_COLOR"] = "1"
    environment["COLUMNS"] = "80"
    result = _run_blocked_entrypoint("systemc-cpp", ["--help"], environment=environment)
    assert result.returncode == 0, result.stdout + result.stderr
    plain_output = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout)
    collapsed_output = " ".join(plain_output.split())
    assert plain_output.index("Run native SystemC C++ models") < plain_output.index("Usage:")
    assert plain_output.startswith("Run native SystemC C++ models")
    assert "Positional arguments:" in plain_output
    assert "Global options:" in plain_output
    assert "Show this message and exit" in collapsed_output
    assert "Show version and exit" in collapsed_output
    assert re.search(r"(?m)^Usage: systemc-cpp \[OPTIONS\] <COMMAND>$", plain_output)
    assert re.search(r"\x1b\[34mUsage:", result.stdout)
    assert re.search(r"\x1b\[35msystemc-cpp", result.stdout)
    assert re.search(r"\x1b\[2;36m\[OPTIONS\]", result.stdout)
    assert re.search(r"\x1b\[96m<COMMAND>", result.stdout)
    assert len(re.findall(r"\x1b\[96mrun", result.stdout)) >= 2
    assert re.search(r"\x1b\[2;38;5;247mRun native SystemC C\+\+ models", result.stdout)


def test_installed_cpp_run_help_is_lightweight_and_styles_options():
    """Render native run help without importing SystemC or cppyy."""
    environment = dict(os.environ)
    environment["PYSYSTEMC_COLOR"] = "always"
    environment["NO_COLOR"] = "1"
    result = _run_blocked_entrypoint("systemc-cpp", ["run", "--help"], environment=environment)
    assert result.returncode == 0, result.stdout + result.stderr
    plain_output = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout)
    assert "Usage: systemc-cpp run [OPTIONS] source [source ...]" in plain_output
    assert "-I, --include <INCLUDE>" in plain_output
    assert re.search(r"\x1b\[35msystemc-cpp", result.stdout)
    assert re.search(r"\x1b\[96mrun", result.stdout)
    assert re.search(r"\x1b\[2;36m\[OPTIONS\]", result.stdout)
    assert len(re.findall(r"\x1b\[2;36m-I", result.stdout)) >= 1
    assert len(re.findall(r"\x1b\[2;36m--include", result.stdout)) >= 1
    assert re.search(r"\x1b\[33m<INCLUDE>\x1b\[0m", result.stdout)
    assert re.search(r"\x1b\[32msource", result.stdout)


def test_installed_python_help_styles_command_rows_and_placeholders():
    """Style Python command choices in usage and detail rows without native imports."""
    environment = dict(os.environ)
    environment["PYSYSTEMC_COLOR"] = "always"
    environment["NO_COLOR"] = "1"
    environment["COLUMNS"] = "42"
    result = _run_blocked_entrypoint("systemc-py", ["--help"], environment=environment)
    assert result.returncode == 0, result.stdout + result.stderr
    plain_output = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout)
    assert plain_output.index("Explore native SystemC") < plain_output.index("Usage:")
    assert "Usage: systemc-py" in plain_output
    assert "<COMMAND>" in plain_output
    assert re.search(r"\x1b\[35msystemc-py", result.stdout)
    assert len(re.findall(r"\x1b\[96mrepl", result.stdout)) >= 2
    assert len(re.findall(r"\x1b\[96mhelp", result.stdout)) >= 2

    placeholder = _run_blocked_entrypoint("systemc-py", ["help", "--help"], environment=environment)
    assert placeholder.returncode == 0, placeholder.stdout + placeholder.stderr
    placeholder_plain = re.sub(r"\x1b\[[0-9;]*m", "", placeholder.stdout)
    assert "Usage: systemc-py help" in placeholder_plain
    assert "<COMMAND>" in placeholder_plain
    assert re.search(r"\x1b\[96m<COMMAND>\x1b\[0m", placeholder.stdout)


@pytest.mark.parametrize("explicit_run", [True, False])
def test_packaged_cpp_runner_arguments_and_status(tmp_path, explicit_run):
    """Native runner forwards '--' arguments and propagates nonzero native status."""
    source = tmp_path / "entry.cpp"
    source.write_text("""#include <systemc>
#include <iostream>
int sc_main(int argc, char** argv) {
  std::cout << "RUNNER_ARGS " << argc << "|" << argv[1] << "|" << argv[2] << std::endl;
  return 7;
}
""")
    script = """
import importlib.metadata
import sys

entry_point = next(
    entry
    for entry in importlib.metadata.distribution("pysystemc").entry_points
    if entry.group == "console_scripts" and entry.name == "systemc-cpp"
)
sys.exit(entry_point.load()())
"""
    result = subprocess.run(
        [sys.executable, "-c", script, *(["run"] if explicit_run else []), str(source), "--", "café", "--flag"],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 7, result.stdout + result.stderr
    assert "RUNNER_ARGS 3|café|--flag" in result.stdout


def test_cpp_runner_multiple_sources_include_and_errors(tmp_path):
    """Runner supports ordered compilation/includes and reports compile/runtime errors."""
    includes = tmp_path / "headers"
    includes.mkdir()
    (includes / "value.h").write_text("int result_value();\n")
    helper = tmp_path / "helper.cpp"
    helper.write_text("int result_value() { return 0; }\n")
    source = tmp_path / "main.cpp"
    source.write_text("""#include <systemc>
#include <value.h>
#include <cstring>
int sc_main(int argc, char** argv) {
  if (sc_core::sc_argc() != argc) return 2;
  if (std::strcmp(sc_core::sc_argv()[1], "native") != 0) return 3;
  argv[1][0] = 'X';
  if (std::strcmp(sc_core::sc_argv()[1], "native") != 0) return 4;
  return result_value();
}
""")
    entrypoint = """
import importlib.metadata
import sys
entry_point = next(
    entry for entry in importlib.metadata.distribution("pysystemc").entry_points
    if entry.group == "console_scripts" and entry.name == "systemc-cpp"
)
sys.exit(entry_point.load()())
"""
    command = [sys.executable, "-c", entrypoint, "run"]
    result = subprocess.run(
        [*command, "-I", str(includes), str(helper), str(source), "--", "native"],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    source.write_text("this is invalid C++\n")
    result = subprocess.run([*command, str(source)], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert result.returncode == 1
    assert "systemc-cpp:" in result.stderr
    source.write_text(
        '#include <systemc>\n#include <stdexcept>\nint sc_main(int, char**) { throw std::runtime_error("runner_failure_marker"); }\n'
    )
    result = subprocess.run([*command, str(source)], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert result.returncode == 1, result.stdout + result.stderr
    assert "runner_failure_marker" in result.stdout + result.stderr


def test_runner_invalid_inputs_and_repeated_invocation(tmp_path):
    """Diagnose invalid sources/includes and reject a second Cling model invocation."""
    source = tmp_path / "model.cpp"
    source.write_text("#include <systemc>\nint sc_main(int, char**) { return 0; }\n")
    probe = tmp_path / "probe.py"
    probe.write_text(
        "from amal.eda.systemc._runner import run_sources\n"
        "import sys\n"
        "for sources, includes in (([], []), ([sys.argv[1]], [sys.argv[1]])):\n"
        "    try: run_sources(sources, includes=includes)\n"
        "    except ValueError: pass\n"
        "    else: raise AssertionError('invalid input accepted')\n"
        "assert run_sources([sys.argv[1]]) == 0\n"
        "try: run_sources([sys.argv[1]])\n"
        "except RuntimeError as e: assert 'fresh interpreter' in str(e)\n"
        "else: raise AssertionError('repeated invocation accepted')\n"
    )
    result = subprocess.run(
        [sys.executable, str(probe), str(source)],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    result = subprocess.run(
        [sys.executable, "-m", "amal.eda.systemc._runner", str(tmp_path / "missing.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 1
    assert "missing.cpp" in result.stderr
