"""Validate installed console arguments and tutorials outside a source checkout."""

from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import amal.eda.systemc as systemc


def main() -> None:
    """Run each installed-package simulation in a separate bounded process."""
    root = Path(__file__).resolve().parents[1]
    libraries = list((Path(systemc.__file__).parent / "_native/lib").glob("libsystemc.so*"))
    assert [path.name for path in libraries] == ["libsystemc.so"], libraries
    executable = shutil.which("pysystemc-cpp")
    assert executable, "installed console entry is missing"
    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory)
        source = work / "entry.cpp"
        source.write_text(
            "#include <systemc>\n#include <string>\n"
            'int sc_main(int n, char** v) { return n == 3 && std::string(v[1]) == "café" '
            '&& std::string(v[2]) == "--flag" ? 7 : 9; }\n'
        )
        result = subprocess.run(
            [executable, str(source), "--", "café", "--flag"],
            cwd=work,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert result.returncode == 7, result.stdout + result.stderr
        for example, marker in (
            ("helloworld/py/helloworld.py", "Simulation finished"),
            ("counter/py/counter_tb.py", "@74 ns :: Incremented Counter 0"),
            ("simple_fifo/py/simple_fifo.py", "Visit www.accellera.org"),
        ):
            result = subprocess.run(
                [sys.executable, str(root / "examples" / example)],
                cwd=work,
                capture_output=True,
                text=True,
                timeout=60,
            )
            assert result.returncode == 0, result.stdout + result.stderr
            assert marker in result.stdout, result.stdout
    print("Installed console arguments/status and tutorials passed")


if __name__ == "__main__":
    main()
