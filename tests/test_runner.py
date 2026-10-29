"""Validate SystemC runner in isolated subprocesses."""

import json
import os
import subprocess
import sys

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
    assert result.stdout.index("SystemC 2.3.4-Accellera") < result.stdout.index("Hello world using approach 1")
    env["SC_COPYRIGHT_MESSAGE"] = "DISABLE"
    result = subprocess.run(
        command, cwd=tmp_path, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout
    assert "SystemC 2.3.4-Accellera" not in result.stdout
    assert "Hello world using approach 2" in result.stdout


@pytest.mark.parametrize(
    ("arguments", "expected_status"),
    [([], 2), (["-h"], 0), (["--help"], 0), (["--version"], 0), (["--invalid-option", "model.cpp"], 2)],
)
def test_installed_cpp_entrypoint_metadata_paths_stay_lightweight(arguments, expected_status):
    """Load the installed console entry point and reject eager native imports."""
    script = r"""
import importlib.metadata
import json
import sys
from importlib.abc import MetaPathFinder


class NativeImportBlocker(MetaPathFinder):
    "Fail immediately if CLI dispatch attempts to load native modules."

    def find_spec(self, fullname, path=None, target=None):
        "Reject native module loading while the dispatcher parses metadata-only commands."
        if fullname == "amal.eda.systemc" or fullname.startswith("amal.eda.systemc."):
            raise AssertionError(f"native package imported during CLI parsing: {fullname}")
        if fullname == "cppyy" or fullname.startswith("cppyy."):
            raise AssertionError(f"cppyy imported during CLI parsing: {fullname}")
        return None


sys.meta_path.insert(0, NativeImportBlocker())
distribution = importlib.metadata.distribution("pysystemc")
entry_point = next(
    entry
    for entry in distribution.entry_points
    if entry.group == "console_scripts" and entry.name == "pysystemc-cpp"
)
scenario = json.loads(sys.argv[1])
sys.argv = ["pysystemc-cpp", *scenario["arguments"]]
try:
    status = entry_point.load()()
except SystemExit as error:
    status = error.code
assert status == scenario["expected_status"], (status, sys.argv)
assert not any(
    name == "cppyy"
    or name.startswith("cppyy.")
    or name == "amal.eda.systemc"
    or name.startswith("amal.eda.systemc.")
    for name in sys.modules
), "native modules remained loaded after CLI parsing"
"""
    scenario = json.dumps({"arguments": arguments, "expected_status": expected_status})
    result = subprocess.run(
        [sys.executable, "-c", script, scenario],
        capture_output=True,
        text=True,
        timeout=30,
    )
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    if arguments in (["-h"], ["--help"]):
        assert "usage: pysystemc-cpp" in output
        assert "Compile native SystemC sources" in output
    elif arguments == ["--version"]:
        assert output.startswith("pysystemc-cpp ")
    elif not arguments:
        assert "usage: pysystemc-cpp" in output
        assert "the following arguments are required: source" in output
    else:
        assert "unrecognized arguments: --invalid-option" in output

    assert "SystemC 2.3.4" not in output
    assert "Accellera" not in output
    assert "Copyright" not in output
    assert "OSCI/SystemC" not in output
    assert "Simulation stopped" not in output


def test_packaged_cpp_runner_arguments_and_status(tmp_path):
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
    if entry.group == "console_scripts" and entry.name == "pysystemc-cpp"
)
sys.exit(entry_point.load()())
"""
    result = subprocess.run(
        [sys.executable, "-c", script, str(source), "--", "café", "--flag"],
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
    command = [sys.executable, "-m", "amal.eda.systemc._runner"]
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
    assert "pysystemc-cpp:" in result.stderr
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
